#include <STC15F2K60S2.H>
#include <intrins.h>
#include "led.h"

#pragma NOAREGS

/*
 * 模块：CH452 显示与键盘底层驱动
 * 总线格式：
 *   写操作：LOAD 为低期间发送 12 位数据，低位先发，LOAD 拉高后锁存；
 *   读操作：发送 4 位读键命令 0x07 后，从 DOUT 采集 7 位键盘事件码。
 */

/*
 * CH452 串行接口引脚分配：
 *   P2.4 -> DCLK，串行时钟输出；
 *   P2.5 -> DIN ，串行数据输出；
 *   P2.6 -> LOAD，加载控制输出；
 *   P2.7 -> DOUT，串行数据输入，同时作为按键事件提示输入。
 */
sbit DCLK = P2^4;
sbit DIN  = P2^5;
sbit LOAD = P2^6;
sbit DOUT = P2^7;

/* CH452 显示寄存器地址，分别对应 4 位数码管。 */
#define DIG0 0x0c00
#define DIG1 0x0d00
#define DIG2 0x0e00
#define DIG3 0x0f00

/*
 * 段码表：数组下标 0~15 对应显示字符 0~F。
 * 数组元素为写入 CH452 的 8 位段选数据，段位编码由板载数码管连线决定。
 */
static uchar seg_code[16] = {
    0xbd, 0x18, 0xd5, 0xd9,
    0x78, 0xe9, 0xed, 0x98,
    0xfd, 0xf9, 0xfc, 0x6d,
    0xa5, 0x5d, 0xe5, 0xe4
};

/*
 * 配置 CH452 总线口线。
 * P2.4~P2.6 配置为推挽输出；P2.7 配置为准双向输入。
 * 寄存器操作只修改 P2.4~P2.7，保留 P2.0~P2.3 的原状态。
 */
static void CH452PortInit(void)
{
    P2M1 &= 0x0f;      /* P2.4~P2.7：M1=0 */
    P2M0 &= 0x0f;      /* 清除 P2.4~P2.7 的 M0 位 */
    P2M0 |= 0x70;      /* P2.4~P2.6：推挽输出；P2.7：准双向 */

    DCLK = 1;          /* 总线空闲电平 */
    DIN  = 0;
    LOAD = 1;          /* LOAD 空闲为高 */
    DOUT = 1;          /* 准双向输入口写 1，释放 DOUT */
}

/*
 * CH452 位时序间隔。
 * 该延时用于 DCLK、LOAD 翻转后的建立和保持时间裕量。
 */
static void Delay1us(void)
{
    _nop_();
    _nop_();
    _nop_();
    _nop_();
}

/*
 * 向 CH452 写入 12 位命令或显示数据。
 * 输入参数 cmd 的最低位首先送出，循环结束后由 LOAD 上升沿完成锁存。
 */
static void CH452Write(uint cmd)
{
    uchar i;

    LOAD = 0;          /* 开始写帧 */
    DOUT = 1;          /* 写帧期间释放 DOUT 输入口 */

    for (i = 0; i < 12; i++)
    {
        DIN = (uchar)(cmd & 0x01);  /* 输出当前最低位 */
        DCLK = 0;
        Delay1us();
        cmd >>= 1;
        DCLK = 1;                   /* CH452 在时钟边沿采样 DIN */
        Delay1us();
    }

    LOAD = 1;          /* 锁存 12 位写入数据 */

    Delay1us();
    Delay1us();
    Delay1us();
    Delay1us();
}

/*
 * 从 CH452 读取键盘事件码。
 * 读帧由 4 位命令 0x07 和 7 位返回数据组成，返回数据由 DOUT 输出。
 */
static uchar CH452Read(void)
{
    uchar i;
    uchar key = 0;
    uchar cmd = 0x07;

    LOAD = 0;

    /* 发送读键命令，低位先发。 */
    for (i = 0; i < 4; i++)
    {
        DIN = (uchar)(cmd & 0x01);
        DCLK = 0;
        Delay1us();
        cmd >>= 1;
        DCLK = 1;
        Delay1us();
    }

    LOAD = 1;
    Delay1us();

    /* 采集 7 位键盘事件码。 */
    for (i = 0; i < 7; i++)
    {
        key <<= 1;

        if (DOUT)
        {
            key++;
        }

        DCLK = 0;
        Delay1us();
        DCLK = 1;
        Delay1us();
    }

    return key;
}

void ChipInit(void)
{
    CH452PortInit();

    /* CH452 工作模式控制字，启用板卡所需的显示和键盘扫描状态。 */
    CH452Write(0x0403);
}

void ShowNums(uint num)
{
    /* 4 位数码管只显示 0~9999。 */
    num %= 10000;

    CH452Write(DIG0 | seg_code[num / 1000]);
    CH452Write(DIG1 | seg_code[(num % 1000) / 100]);
    CH452Write(DIG2 | seg_code[(num % 100) / 10]);
    CH452Write(DIG3 | seg_code[num % 10]);
}

void ShowHex(uint num)
{
    CH452Write(DIG0 | seg_code[num >> 12]);
    CH452Write(DIG1 | seg_code[(num >> 8) & 0x0f]);
    CH452Write(DIG2 | seg_code[(num >> 4) & 0x0f]);
    CH452Write(DIG3 | seg_code[num & 0x0f]);
}

void ShowString(uchar a, uchar b, uchar c, uchar d)
{
    CH452Write(DIG0 | a);
    CH452Write(DIG1 | b);
    CH452Write(DIG2 | c);
    CH452Write(DIG3 | d);
}

uchar KeyRead(void)
{
    return CH452Read();
}