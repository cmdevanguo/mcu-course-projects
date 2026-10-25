ORG 0000H
LJMP MAIN
; 共阳数码管段码表 0~9
; 低电平点亮
ORG 0100H
SEG_TAB:
    DB 11H,0D7H,32H,92H,0D4H   ; 0 1 2 3 4
    DB 98H,18H,0D3H,10H,90H    ; 5 6 7 8 9
ORG 0200H
MAIN:
    MOV P0, #11H       ; 上电显示0
    MOV R2, #00H       ; R2=计数器，初值0
LOOP_MAIN:
    JB P3.2, LOOP_MAIN ; 没按键则等待，P3.2=1表示未按下
    LCALL DELAY20MS    ; 消抖
    JB P3.2, LOOP_MAIN ; 再次判断，确认是否真的按下
    INC R2             ; 按键次数+1
    CJNE R2,#10,SHOW   ; 如果不到10，直接显示
    MOV R2,#00H        ; 如果到10次，清零
SHOW:
    MOV A, R2          ; A=当前计数值
    MOV DPTR,#SEG_TAB  ; DPTR指向段码表
    MOVC A,@A+DPTR     ; 查表取段码
    MOV P0,A           ; 输出到数码管显示
WAIT_UP:
    JNB P3.2, WAIT_UP  ; 等待按键松开，P3.2=0表示还在按下
    LJMP LOOP_MAIN     ; 返回继续检测按键
; 20ms 消抖延时
DELAY20MS:
    MOV R5,#40
D1:
    MOV R6,#250
D2:
    DJNZ R6,D2
    DJNZ R5,D1
    RET
END