# MCU Course Projects

基于 8051 架构单片机的课程设计与课程实验代码，包含 C51 和 8051 汇编程序，以及可直接使用 Keil µVision 打开的工程文件。

This repository contains an 8051-based microcontroller course project and laboratory exercises written in C51 and 8051 Assembly.

## 项目内容

### 课程设计：四档倒计时器

`course-project/` 中为基于 **STC15F2K60S2** 的倒计时器项目，主要功能包括：

- 通过 CH452 驱动 4 位数码管和矩阵按键；
- 支持 `0009`、`0099`、`0999`、`9999` 秒四个倒计时档位；
- 支持开始、暂停和复位操作；
- 支持通过 UART 串口查询状态和控制倒计时；
- 串口默认参数为 `9600 bps, 8N1`。

主要文件：

- `main.c`：倒计时逻辑、定时器和 UART 通信；
- `led.c`：CH452 显示与键盘底层驱动；
- `led.h`：CH452 驱动接口和按键事件定义；
- `mct5.uvproj`：Keil µVision 工程文件；
- `STARTUP.A51`：Keil C51 启动文件。

### 课程实验

`labs/` 中包含 8 个 8051 汇编语言实验：

| 实验    | 内容                                |
| ------- | ----------------------------------- |
| 实验1-1 | 基于软件延时的 LED 流水灯           |
| 实验1-2 | 按键计数与一位数码管显示            |
| 实验2-1 | 定时器中断控制 LED 往返移位         |
| 实验2-2 | 外部中断控制两位数码管加减计数      |
| 实验3-1 | 查询定时器溢出标志控制 LED 循环移位 |
| 实验3-2 | 定时器中断实现 `00–59` 秒计数显示   |
| 实验4-1 | 串口接收数据并在两位数码管上显示    |
| 实验4-2 | 按键计数、数码管显示及串口发送      |

## 仓库结构

```text
mcu-course-projects/
├── course-project/
│   ├── led.c
│   ├── led.h
│   ├── main.c
│   ├── mct5.uvproj
│   └── STARTUP.A51
├── labs/
│   ├── 实验1-1/
│   ├── 实验1-2/
│   ├── 实验2-1/
│   ├── 实验2-2/
│   ├── 实验3-1/
│   ├── 实验3-2/
│   ├── 实验4-1/
│   └── 实验4-2/
├── .gitignore
├── LICENSE
└── README.md
```

每个实验目录通常包含：

- `.a51` 汇编源文件；
- `.uvproj` Keil 工程文件；
- `STARTUP.A51` 启动文件。

## 开发环境

- Keil µVision
- Keil C51 / A51 工具链
- STC15F2K60S2
- C51
- 8051 Assembly

## 使用方法

1. 克隆或下载本仓库；
2. 使用 Keil µVision 打开对应目录中的 `.uvproj` 文件；
3. 检查工程所选单片机型号和系统时钟；
4. 根据实际开发板确认 LED、数码管、按键和串口的引脚连接；
5. 编译工程并生成 HEX 文件；
6. 使用相应下载工具将程序烧录到目标单片机。

> 不同开发板的外围电路和端口连接可能不同。直接烧录前，请先核对原理图和程序中的端口定义。

## 源文件编码

源码统一使用 **UTF-8** 编码，以便中文注释能够在 GitHub 和现代代码编辑器中正常显示。

部分旧版本 Keil 对 UTF-8 中文显示的支持可能有限，但这不会影响只包含 ASCII 指令、标识符和常量的程序编译。若编辑器显示异常，请检查文件编码设置。

## 许可证

除另有说明外，本仓库中由项目作者编写的代码采用 [MIT License](LICENSE)。

`STARTUP.A51` 来源于 Keil C51 工具链，并保留其原始版权声明；该文件不属于本仓库原创代码。

Unless otherwise noted, the original code in this repository is licensed under the MIT License. The `STARTUP.A51` files originate from the Keil C51 toolchain and retain their original copyright notices.

## 说明

本仓库用于记录单片机课程设计和实验过程，主要面向学习、交流与参考。实际使用时请根据自己的硬件平台修改相关配置。
