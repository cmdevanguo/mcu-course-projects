;==================================================
; 题目：外部中断控制数码管显示 00~99
; S1：加计数，接 P3.2 / INT0
; S2：减计数，接 P3.3 / INT1
;==================================================
P0      DATA    080H
P2      DATA    0A0H
IT0     BIT     088H
IT1     BIT     08AH
EX0     BIT     0A8H
EX1     BIT     0AAH
EA      BIT     0AFH
S1      BIT     0B2H        ; P3.2 / INT0
S2      BIT     0B3H        ; P3.3 / INT1
COUNT   DATA    30H         ; 计数值：00~99
ORG 0000H
LJMP START
ORG 0003H                  ; 外部中断0入口
LJMP INT0_PLUS
ORG 0013H                  ; 外部中断1入口
LJMP INT1_MINUS
ORG 0030H
; 数码管段码表
SEG_SHI:
DB 011H,0D7H,032H,092H,0D4H,098H,018H,0D3H,010H,090H
SEG_GE:
DB 088H,0EBH,04CH,049H,02BH,019H,018H,0CBH,008H,009H
;==================================================
; 主程序
;==================================================
START:
    MOV SP,#60H             ; 设置堆栈，避免与变量冲突
    MOV COUNT,#00H          ; 初始显示 00
    SETB IT0                ; INT0 下降沿触发
    SETB IT1                ; INT1 下降沿触发
    SETB EX0                ; 允许外部中断0
    SETB EX1                ; 允许外部中断1
    SETB EA                 ; 开总中断
MAIN:
    ACALL DISPLAY
    SJMP MAIN
;==================================================
; 显示子程序
; COUNT = 0~99
; A = 十位，B = 个位
;==================================================
DISPLAY:
    MOV A,COUNT
    MOV B,#10
    DIV AB                  ; A=十位，B=个位
    MOV DPTR,#SEG_SHI
    MOVC A,@A+DPTR
    MOV P0,A                ; 十位送 P0
    MOV A,B
    MOV DPTR,#SEG_GE
    MOVC A,@A+DPTR
    MOV P2,A                ; 个位送 P2
    RET
;==================================================
; 约 10ms 消抖延时，12MHz 晶振
;==================================================
DELAY_10MS:
    MOV R6,#20
DLY1:
    MOV R7,#248
DLY2:
    DJNZ R7,DLY2
    DJNZ R6,DLY1
    RET
;==================================================
; 外部中断0：S1 加计数
; 00 -> 01 -> ... -> 99 -> 00
;==================================================
INT0_PLUS:
    PUSH ACC
    ACALL DELAY_10MS
    JB S1,PLUS_EXIT         ; 抖动或已经松开，退出
    MOV A,COUNT
    INC A
    CJNE A,#064H,PLUS_SAVE  ; 100 = 64H
    CLR A                   ; 超过 99 回到 00
PLUS_SAVE:
    MOV COUNT,A
WAIT_S1_RELEASE:
    JNB S1,WAIT_S1_RELEASE  ; 等待按键释放
    ACALL DELAY_10MS
PLUS_EXIT:
    POP ACC
    RETI
;==================================================
; 外部中断1：S2 减计数
; 00 -> 99 -> 98 -> ... -> 00
;==================================================
INT1_MINUS:
    PUSH ACC
    ACALL DELAY_10MS
    JB S2,MINUS_EXIT        ; 抖动或已经松开，退出
    MOV A,COUNT
    JNZ DO_MINUS
    MOV A,#063H             ; 99 = 63H
    SJMP MINUS_SAVE
DO_MINUS:
    DEC A
MINUS_SAVE:
    MOV COUNT,A
WAIT_S2_RELEASE:
    JNB S2,WAIT_S2_RELEASE  ; 等待按键释放
    ACALL DELAY_10MS
MINUS_EXIT:
    POP ACC
    RETI
END