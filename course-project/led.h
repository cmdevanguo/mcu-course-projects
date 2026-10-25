#ifndef __LED_H__
#define __LED_H__

/*
 * 模块接口：CH452 显示与按键驱动
 * 硬件对象：STC15F2K60S2 外接 CH452
 * 对外功能：
 *   1. 初始化 CH452 串行控制口；
 *   2. 向 4 位数码管写入十进制或十六进制显示数据；
 *   3. 读取 CH452 键盘扫描事件码。
 */
typedef unsigned char uchar;    /* 8 位无符号数据类型 */
typedef unsigned int  uint;     /* 16 位无符号数据类型 */

/*
 * CH452 键盘事件码定义。
 * 后缀 u 表示按键释放事件，后缀 d 表示按键按下事件。
 * 上层按键处理通过宏名使用事件码，避免在业务逻辑中直接使用立即数。
 */
#define b1u  4       /* K1 释放 */
#define b1d  68      /* K1 按下 */
#define b2u  5       /* K2 释放 */
#define b2d  69      /* K2 按下 */
#define b3u  6       /* K3 释放 */
#define b3d  70      /* K3 按下 */
#define b4u  7       /* K4 释放 */
#define b4d  71      /* K4 按下 */
#define b5u  12      /* K5 释放 */
#define b5d  76      /* K5 按下 */
#define b6u  13      /* K6 释放 */
#define b6d  77      /* K6 按下 */
#define b7u  14      /* K7 释放 */
#define b7d  78      /* K7 按下 */
#define b8u  15      /* K8 释放 */
#define b8d  79      /* K8 按下 */

/* 初始化 CH452 总线口线，并写入 CH452 初始控制字。 */
void ChipInit(void);

/* 输出 4 位十进制数，显示范围为 0000~9999；超出范围时显示低 4 位十进制值。 */
void ShowNums(uint num);

/* 输出 4 位十六进制数，显示范围为 0000~FFFF。 */
void ShowHex(uint num);

/* 直接写入 4 个数码管的段码；参数必须为 CH452 显示段码。 */
void ShowString(uchar a, uchar b, uchar c, uchar d);

/* 读取 CH452 当前挂起的键盘事件码。 */
uchar KeyRead(void);

#endif