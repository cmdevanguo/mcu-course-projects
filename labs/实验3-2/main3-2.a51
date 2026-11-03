        ORG     0000H
        LJMP    MAIN
        ORG     001BH              ; 定时器1中断入口
        LJMP    TIMER1_ISR
        ORG     0030H
; ==================================================
; 常量定义
; ==================================================
RELOAD_H    EQU     0E3H           ; 定时器1初值高字节
RELOAD_L    EQU     0E0H           ; 定时器1初值低字节
CNT_1S      EQU     128            ; 128次中断约等于1秒
MAX_SEC     EQU     60             ; 秒数范围 0~59
; ==================================================
; 内部RAM变量定义
; ==================================================
TICK_CNT    EQU     30H            ; 中断计数器 0~127
SEC_VAL     EQU     31H            ; 当前秒数 0~59
TEN_VAL     EQU     32H            ; 十位
ONE_VAL     EQU     33H            ; 个位
; ==================================================
; 段码表
; ==================================================
TABA:   DB  11H, 0D7H, 32H, 92H, 0D4H
        DB  98H, 18H, 0D3H, 10H, 90H     ; P0 十位段码

TAB:    DB  88H, 0EBH, 4CH, 49H, 2BH
        DB  19H, 18H, 0CBH, 08H, 09H     ; P2 个位段码
; ==================================================
; 主程序
; ==================================================
MAIN:
        LCALL   TIMER1_INIT
        LCALL   VAR_INIT
        LCALL   UPDATE_BUF
MAIN_LOOP:
        LCALL   DISPLAY
        SJMP    MAIN_LOOP
; ==================================================
; 定时器1初始化
; 11.0592MHz，方式1，初值 E3E0H
; 每次中断约 7.8125ms
; ==================================================
TIMER1_INIT:
        MOV     TMOD, #10H         ; 定时器1，方式1
        MOV     TH1, #RELOAD_H
        MOV     TL1, #RELOAD_L
        SETB    ET1                ; 允许定时器1中断
        SETB    EA                 ; 开总中断
        SETB    TR1                ; 启动定时器1
        RET
; ==================================================
; 变量初始化
; ==================================================
VAR_INIT:
        MOV     TICK_CNT, #0
        MOV     SEC_VAL,  #0
        MOV     TEN_VAL,  #0
        MOV     ONE_VAL,  #0
        RET
; ==================================================
; 定时器1中断服务程序
; ==================================================
TIMER1_ISR:
        PUSH    ACC
        PUSH    B
        PUSH    DPL
        PUSH    DPH
        PUSH    PSW
        MOV     TH1, #RELOAD_H     ; 重装初值
        MOV     TL1, #RELOAD_L
        INC     TICK_CNT
        MOV     A, TICK_CNT
        CJNE    A, #CNT_1S, ISR_EXIT
        MOV     TICK_CNT, #0       ; 满1秒清零
        INC     SEC_VAL            ; 秒数加1
        MOV     A, SEC_VAL
        CJNE    A, #MAX_SEC, UPDATE_TIME
        MOV     SEC_VAL, #0        ; 到60秒归零
UPDATE_TIME:
        LCALL   UPDATE_BUF         ; 更新十位和个位缓冲
ISR_EXIT:
        POP     PSW
        POP     DPH
        POP     DPL
        POP     B
        POP     ACC
        RETI
; ==================================================
; 更新显示缓冲
; 将 SEC_VAL 拆分为十位 TEN_VAL 和个位 ONE_VAL
; ==================================================
UPDATE_BUF:
        MOV     A, SEC_VAL
        MOV     B, #10
        DIV     AB                 ; A = 十位，B = 个位
        MOV     TEN_VAL, A
        MOV     ONE_VAL, B
        RET
; ==================================================
; 静态显示
; P0 输出十位段码
; P2 输出个位段码
; ==================================================
DISPLAY:
        MOV     DPTR, #TABA
        MOV     A, TEN_VAL
        MOVC    A, @A+DPTR
        MOV     P0, A
        MOV     DPTR, #TAB
        MOV     A, ONE_VAL
        MOVC    A, @A+DPTR
        MOV     P2, A
        RET
        END