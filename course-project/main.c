#include <STC15F2K60S2.H>
#include <intrins.h>
#include "led.h"

#pragma NOAREGS

/*
 * 应用模块：四档倒计时器
 * 硬件资源分配：
 *   Timer0：5 ms 周期中断，累计形成 1 s 倒计时基准；
 *   Timer2：UART1 波特率发生器；
 *   UART1 ：P3.0/RXD、P3.1/TXD，ASCII 文本通信；
 *   CH452 ：4 位数码管显示和 K1~K8 键盘扫描。
 *
 * 本地按键映射：
 *   K1：选择 0009 s 档；
 *   K2：选择 0099 s 档；
 *   K3：选择 0999 s 档；
 *   K4：选择 9999 s 档；
 *   K5：开始或暂停；
 *   K6：复位当前档位；
 *   K7、K8：保留功能键，仅上报键盘事件码。
 *
 * UART 命令集，串口参数为 9600 bps、8N1：
 *   '1'/'2'/'3'/'4'：选择 0009/0099/0999/9999 s 档；
 *   '5'、'S'、's'  ：开始或暂停；
 *   '6'、'R'、'r'  ：复位当前档位；
 *   '?'             ：返回当前状态；
 *   'H'、'h'        ：返回命令提示。
 */

/* 系统时钟频率，单位 Hz；该值应与 STC-ISP 配置的运行时 IRC 频率一致。 */
#define FOSC             11059200UL

/* UART1 通信速率，单位 bps。 */
#define UART_BAUD        9600UL

/*
 * Timer0 定时参数。
 * Timer0 使用 12T 模式，每 5 ms 产生一次中断，200 次中断累计为 1 s。
 */
#define T0_TICK_MS       5UL
#define TICKS_PER_SECOND (1000UL / T0_TICK_MS)
#define T0_RELOAD        (65536UL - (FOSC / 12UL / TICKS_PER_SECOND))
#define T0_TH            ((uchar)(T0_RELOAD >> 8))
#define T0_TL            ((uchar)(T0_RELOAD & 0xff))

/* UART1 由 Timer2 提供波特率时钟，Timer2 使用 1T 模式。 */
#define UART_T2_RELOAD   (65536UL - (FOSC / 4UL / UART_BAUD))
#define UART_T2H         ((uchar)(UART_T2_RELOAD >> 8))
#define UART_T2L         ((uchar)(UART_T2_RELOAD & 0xff))

/* CH452 的 DOUT 作为按键事件提示信号，低电平表示存在待读取事件。 */
sbit KEY_INT = P2^7;

/*
 * 倒计时状态变量。
 * 这些变量在 Timer0 中断和主循环之间共享，使用 volatile 防止编译器优化缓存。
 */
static volatile uint  left_time      = 9;  /* 剩余时间，单位 s */
static volatile uint  count_tick     = 0;  /* 5 ms 节拍计数器，达到 1 s 后清零 */
static volatile uint  system_tick    = 0;  /* 系统节拍计数器，单位 5 ms */
static volatile uchar mode           = 0;  /* 档位：0=0009，1=0099，2=0999，3=9999 */
static volatile uchar running        = 0;  /* 运行状态：0=停止，1=计时中 */
static volatile uchar display_update = 1;  /* 显示刷新请求标志 */

/* UART1 收发状态变量。 */
static volatile uchar uart_busy    = 0;    /* 发送忙标志，TI 中断置 0 */
static volatile uchar uart_rx_flag = 0;    /* 接收完成标志，主循环处理后置 0 */
static volatile uchar uart_rx_data = 0;    /* 最近一次接收到的字节 */

/* 档位编号转换为倒计时初值，返回值单位为 s。 */
static uint PresetTime(uchar m)
{
    switch (m)
    {
        case 0:
            return 9;

        case 1:
            return 99;

        case 2:
            return 999;

        default:
            return 9999;
    }
}

/*
 * 读取 16 位系统节拍。
 * 8051 内核访问 16 位变量需要两次 8 位读操作，关中断可避免读数跨中断更新。
 */
static uint GetSystemTick(void)
{
    uint value;
    uchar ea_bak;

    ea_bak = EA;
    EA = 0;
    value = system_tick;
    EA = ea_bak;

    return value;
}

/*
 * 毫秒级阻塞延时。
 * 延时基准来自 Timer0 的 5 ms 系统节拍，适用于按键滤波和非精密等待场景。
 */
static void DelayMs(uint ms)
{
    uint start;
    uint wait_tick;

    if (ms == 0)
    {
        return;
    }

    wait_tick = (uint)((ms + T0_TICK_MS - 1) / T0_TICK_MS);
    start = GetSystemTick();

    while ((uint)(GetSystemTick() - start) < wait_tick)
    {
        ;
    }
}

/* 获取倒计时状态快照，保证时间、档位和运行标志来自同一临界区。 */
static void GetStateSnapshot(uint *time, uchar *cur_mode, uchar *is_running)
{
    uchar ea_bak;

    ea_bak = EA;
    EA = 0;
    *time = left_time;
    *cur_mode = mode;
    *is_running = running;
    EA = ea_bak;
}

/* 切换倒计时档位，并装载对应初值；切换后计时状态固定为停止。 */
static void SelectMode(uchar new_mode)
{
    uint value;
    uchar ea_bak;

    value = PresetTime(new_mode);

    ea_bak = EA;
    EA = 0;

    mode = new_mode;
    running = 0;
    count_tick = 0;
    left_time = value;
    display_update = 1;

    EA = ea_bak;
}

/* 翻转运行状态；若剩余时间为 0，则先重装当前档位初值再启动。 */
static void StartOrPause(void)
{
    uchar ea_bak;

    ea_bak = EA;
    EA = 0;

    if (running)
    {
        running = 0;
    }
    else
    {
        if (left_time == 0)
        {
            left_time = PresetTime(mode);
            display_update = 1;
        }

        running = 1;
    }

    count_tick = 0;

    EA = ea_bak;
}

/* 停止计时，并恢复当前档位的倒计时初值。 */
static void ResetCurrentMode(void)
{
    uchar ea_bak;

    ea_bak = EA;
    EA = 0;

    running = 0;
    count_tick = 0;
    left_time = PresetTime(mode);
    display_update = 1;

    EA = ea_bak;
}

/* 阻塞发送 1 字节，发送完成由 UART1 的 TI 中断确认。 */
static void UartSendByte(uchar dat)
{
    while (uart_busy)
    {
        ;
    }

    uart_busy = 1;
    SBUF = dat;
}

/* 发送以 '\0' 结尾的 ASCII 字符串。 */
static void UartSendString(char *str)
{
    while (*str)
    {
        UartSendByte((uchar)(*str));
        str++;
    }
}

/* 发送 0~65535 的十进制文本，避免引入 printf 库开销。 */
static void UartSendUInt(uint value)
{
    uchar buf[5];
    uchar i = 0;

    if (value == 0)
    {
        UartSendByte('0');
        return;
    }

    while ((value > 0) && (i < 5))
    {
        buf[i] = (uchar)(value % 10 + '0');
        value /= 10;
        i++;
    }

    while (i > 0)
    {
        i--;
        UartSendByte(buf[i]);
    }
}

/* 发送 4 位档位文本，与数码管显示格式一致。 */
static void UartSendModeText(uchar m)
{
    switch (m)
    {
        case 0:
            UartSendString("0009");
            break;

        case 1:
            UartSendString("0099");
            break;

        case 2:
            UartSendString("0999");
            break;

        default:
            UartSendString("9999");
            break;
    }
}

/* 输出状态帧：STATUS mode=<档位> left=<剩余秒数>s run=<0/1>。 */
static void UartSendStatus(void)
{
    uint time;
    uchar cur_mode;
    uchar is_running;

    GetStateSnapshot(&time, &cur_mode, &is_running);

    UartSendString("STATUS mode=");
    UartSendModeText(cur_mode);
    UartSendString(" left=");
    UartSendUInt(time);
    UartSendString("s run=");
    UartSendByte(is_running ? '1' : '0');
    UartSendString("\r\n");
}

/* 输出 UART 命令提示帧。 */
static void UartSendHelp(void)
{
    UartSendString("CMD: 1=0009 2=0099 3=0999 4=9999 5/S=start_pause 6/R=reset ?=status\r\n");
}

/*
 * 初始化 UART1。
 * 引脚：P3.0/RXD、P3.1/TXD；格式：8 位数据、无校验、1 位停止位；波特率：9600 bps。
 */
static void UartInit(void)
{
    uart_busy = 0;
    uart_rx_flag = 0;
    uart_rx_data = 0;

    /* UART1 使用第一组引脚。 */
    P_SW1 &= 0x3f;

    /* RXD 保持准双向输入；TXD 配置为推挽输出。 */
    P3M1 &= 0xfc;
    P3M0 &= 0xfc;
    P3M0 |= 0x02;
    P3 |= 0x01;

    SCON = 0x50;       /* 模式 1，允许接收 */
    RI = 0;
    TI = 0;

    /* Timer2 使用内部时钟和 1T 模式，并作为 UART1 波特率发生器。 */
    AUXR &= ~0x18;     /* T2R=0，T2_CT=0 */
    AUXR |= 0x04;      /* T2x12=1 */
    AUXR |= 0x01;      /* S1ST2=1 */

    T2L = UART_T2L;
    T2H = UART_T2H;

    AUXR |= 0x10;      /* T2R=1，启动 Timer2 */

    ES = 1;            /* 允许 UART1 中断 */
}

/* 将 CH452 事件码映射为按键编号，未知事件返回 0。 */
static uchar KeyNumber(uchar key)
{
    switch (key)
    {
        case b1d:
        case b1u:
            return 1;

        case b2d:
        case b2u:
            return 2;

        case b3d:
        case b3u:
            return 3;

        case b4d:
        case b4u:
            return 4;

        case b5d:
        case b5u:
            return 5;

        case b6d:
        case b6u:
            return 6;

        case b7d:
        case b7u:
            return 7;

        case b8d:
        case b8u:
            return 8;

        default:
            return 0;
    }
}

/* 判断事件码是否为按键释放事件。 */
static uchar IsReleaseKey(uchar key)
{
    switch (key)
    {
        case b1u:
        case b2u:
        case b3u:
        case b4u:
        case b5u:
        case b6u:
        case b7u:
        case b8u:
            return 1;

        default:
            return 0;
    }
}

/* 输出按键事件帧：KEY K<编号> <DOWN/UP> raw=<CH452事件码>。 */
static void UartSendKeyReport(uchar raw_key, uchar key_num, uchar is_release)
{
    UartSendString("KEY K");
    UartSendUInt(key_num);

    if (is_release)
    {
        UartSendString(" UP raw=");
    }
    else
    {
        UartSendString(" DOWN raw=");
    }

    UartSendUInt(raw_key);
    UartSendString("\r\n");
}

/* 执行本地按键控制动作，返回 1 表示倒计时状态发生变化。 */
static uchar DoKeyAction(uchar key_num)
{
    switch (key_num)
    {
        case 1:
            SelectMode(0);
            return 1;

        case 2:
            SelectMode(1);
            return 1;

        case 3:
            SelectMode(2);
            return 1;

        case 4:
            SelectMode(3);
            return 1;

        case 5:
            StartOrPause();
            return 1;

        case 6:
            ResetCurrentMode();
            return 1;

        default:
            /* K7、K8 为保留键，不改变倒计时状态。 */
            return 0;
    }
}

/*
 * 处理 CH452 按键事件。
 * 按下事件触发控制动作；释放事件只记录事件帧，避免一次按键产生两次控制动作。
 */
static void HandleKey(uchar key)
{
    uchar key_num;
    uchar is_release;

    key_num = KeyNumber(key);

    if (key_num == 0)
    {
        return;
    }

    is_release = IsReleaseKey(key);
    UartSendKeyReport(key, key_num, is_release);

    if (!is_release)
    {
        if (DoKeyAction(key_num))
        {
            UartSendStatus();
        }
    }
}

/* 根据显示刷新标志更新数码管，降低主循环对 CH452 的重复写入频率。 */
static void RefreshDisplay(void)
{
    uint value;
    uchar need_update;
    uchar ea_bak;

    ea_bak = EA;
    EA = 0;

    value = left_time;
    need_update = display_update;
    display_update = 0;

    EA = ea_bak;

    if (need_update)
    {
        ShowNums(value);
    }
}

/* Timer0 中断服务：维护 5 ms 系统节拍，并在运行状态下累计 1 s 扣减。 */
void Timer0Isr(void) interrupt 1
{
    TH0 = T0_TH;
    TL0 = T0_TL;

    system_tick++;

    if (!running)
    {
        count_tick = 0;
        return;
    }

    count_tick++;

    if (count_tick >= TICKS_PER_SECOND)
    {
        count_tick = 0;

        if (left_time > 0)
        {
            left_time--;
            display_update = 1;
        }

        if (left_time == 0)
        {
            running = 0;
        }
    }
}

/* 初始化 Timer0 为 16 位定时器模式，12T 计数，5 ms 周期中断。 */
static void Timer0Init(void)
{
    TR0 = 0;

    AUXR &= ~0x80;     /* T0x12=0，Timer0 使用 12T 模式 */

    TMOD &= 0xf0;
    TMOD |= 0x01;      /* Timer0 方式 1：16 位定时器 */

    TH0 = T0_TH;
    TL0 = T0_TL;

    TF0 = 0;
    ET0 = 1;
    TR0 = 1;
}

/* UART1 中断服务：接收命令字节，并在发送完成时释放发送忙标志。 */
void Uart1Isr(void) interrupt 4
{
    if (RI)
    {
        RI = 0;
        uart_rx_data = SBUF;
        uart_rx_flag = 1;
    }

    if (TI)
    {
        TI = 0;
        uart_busy = 0;
    }
}

/* 处理 UART 接收命令；每次调用最多处理 1 个接收字节。 */
static void ProcessUartCommand(void)
{
    uchar cmd = 0;
    uchar has_cmd;
    uchar ea_bak;

    ea_bak = EA;
    EA = 0;
    has_cmd = uart_rx_flag;
    if (has_cmd)
    {
        cmd = uart_rx_data;
        uart_rx_flag = 0;
    }
    EA = ea_bak;

    if (!has_cmd)
    {
        return;
    }

    /* 忽略文本终止符和空格，兼容串口工具自动追加回车换行的配置。 */
    if ((cmd == '\r') || (cmd == '\n') || (cmd == ' '))
    {
        return;
    }

    UartSendString("RX ");
    UartSendByte(cmd);
    UartSendString("\r\n");

    switch (cmd)
    {
        case '1':
            SelectMode(0);
            UartSendStatus();
            break;

        case '2':
            SelectMode(1);
            UartSendStatus();
            break;

        case '3':
            SelectMode(2);
            UartSendStatus();
            break;

        case '4':
            SelectMode(3);
            UartSendStatus();
            break;

        case '5':
        case 'S':
        case 's':
            StartOrPause();
            UartSendStatus();
            break;

        case '6':
        case 'R':
        case 'r':
            ResetCurrentMode();
            UartSendStatus();
            break;

        case '?':
            UartSendStatus();
            break;

        case 'H':
        case 'h':
            UartSendHelp();
            break;

        default:
            UartSendHelp();
            break;
    }
}

void main(void)
{
    uchar key;

    ChipInit();         /* 初始化 CH452 显示与键盘接口 */
    SelectMode(0);      /* 默认档位：0009 s */
    RefreshDisplay();   /* 上电显示初始倒计时值 */

    UartInit();         /* 初始化 UART1 通信接口 */
    Timer0Init();       /* 初始化倒计时和消抖共用的时间基准 */

    EA = 1;             /* 外设配置完成后打开总中断 */

    UartSendString("\r\nMCT5 countdown ready. UART=9600,8N1.\r\n");
    UartSendHelp();
    UartSendStatus();

    while (1)
    {
        RefreshDisplay();
        ProcessUartCommand();

        /* CH452 有按键事件时，DOUT/KEY_INT 为低电平。 */
        if (!KEY_INT)
        {
            DelayMs(5);     /* 前沿滤波 */

            if (!KEY_INT)
            {
                key = KeyRead();
                HandleKey(key);

                DelayMs(20);    /* 释放机械抖动窗口，降低重复事件概率 */
            }
        }
    }
}