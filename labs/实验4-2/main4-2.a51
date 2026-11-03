                ORG     0000H
                LJMP    MAIN
;=================================================
; 位定义、变量定义
;=================================================
S1_PIN          BIT     P3.2            ; S1按键，低电平有效
S2_PIN          BIT     P3.3            ; S2按键，低电平有效
COUNT           EQU     30H             ; 当前计数值，范围 00~99
;=================================================
; 主程序
;=================================================
MAIN:           
                LCALL   INIT_SYSTEM
MAIN_LOOP:      
                LCALL   CHECK_S1        ; 检查S1，加计数
                LCALL   CHECK_S2        ; 检查S2，发送数据
                SJMP    MAIN_LOOP
;=================================================
; 系统初始化
;=================================================
INIT_SYSTEM:    
                SETB    S1_PIN          ; P3.2置1，作为输入
                SETB    S2_PIN          ; P3.3置1，作为输入
                MOV     COUNT, #00H     ; 初始显示00
                MOV     P0, #0FFH       ; 关闭/初始化P0数码管
                MOV     P2, #0FFH       ; 关闭/初始化P2数码管
                LCALL   INIT_UART       ; 初始化串口
                LCALL   DISPLAY         ; 显示初始值00
                RET
;=================================================
; 串口初始化
; 11.0592MHz，9600bps，串口方式1
;=================================================
INIT_UART:      
                ANL     PCON, #7FH      ; SMOD=0，波特率不倍速
                ANL     8EH, #0BFH      ; 定时器1时钟为Fosc/12
                ANL     8EH, #0FEH      ; 串口1选择定时器1为波特率发生器
                MOV     TMOD, #20H      ; 定时器1方式2，8位自动重装
                MOV     TH1, #0FDH      ; 9600bps @ 11.0592MHz
                MOV     TL1, #0FDH
                MOV     SCON, #50H      ; 串口方式1，允许接收
                SETB    TR1             ; 启动定时器1
                CLR     TI
                CLR     RI
                RET
;=================================================
; 检查S1
; 功能：按下S1，COUNT加1，00~99循环显示
;=================================================
CHECK_S1:       
                JB      S1_PIN, S1_EXIT         ; 未按下则退出
                LCALL   DELAY10MS               ; 消抖
                JB      S1_PIN, S1_EXIT         ; 再次判断，仍未按下则退出
                LCALL   COUNT_ADD_ONE           ; 计数加1
                LCALL   DISPLAY                 ; 更新显示
S1_WAIT_UP:     
                JNB     S1_PIN, S1_WAIT_UP      ; 等待按键释放
                LCALL   DELAY10MS               ; 释放消抖
S1_EXIT:        
                RET
;=================================================
; 检查S2
; 功能：按下S2，把当前COUNT以ASCII格式发送到串口
;=================================================
CHECK_S2:       
                JB      S2_PIN, S2_EXIT         ; 未按下则退出
                LCALL   DELAY10MS               ; 消抖
                JB      S2_PIN, S2_EXIT         ; 再次判断，仍未按下则退出
                LCALL   SEND_NUMBER             ; 发送当前显示值
S2_WAIT_UP:     
                JNB     S2_PIN, S2_WAIT_UP      ; 等待按键释放
                LCALL   DELAY10MS               ; 释放消抖
S2_EXIT:        
                RET
;=================================================
; COUNT加1
; 范围：00~99，超过99回到00
;=================================================
COUNT_ADD_ONE:  
                MOV     A, COUNT
                INC     A
                CJNE    A, #64H, COUNT_SAVE     ; 64H = 十进制100
                MOV     A, #00H
COUNT_SAVE:     
                MOV     COUNT, A
                RET
;=================================================
; 显示子程序
; COUNT = 00~99
; P0显示十位，P2显示个位
;=================================================
DISPLAY:        
                MOV     A, COUNT
                MOV     B, #10
                DIV     AB                      ; A=十位，B=个位
                MOV     R2, A                   ; R2保存十位
                MOV     R1, B                   ; R1保存个位
                ;---------- 显示个位到P2 ----------
                MOV     DPTR, #SEG_P2
                MOV     A, R1
                MOVC    A, @A+DPTR
                MOV     P2, A
                ;---------- 显示十位到P0 ----------
                MOV     DPTR, #SEG_P0
                MOV     A, R2
                MOVC    A, @A+DPTR
                MOV     P0, A
                RET
;=================================================
; 发送当前数字
; 例如 COUNT=25，则发送 ASCII字符 '2'、'5'、回车、换行
; 串口助手普通字符模式显示：25
;=================================================
SEND_NUMBER:    
                MOV     A, COUNT
                MOV     B, #10
                DIV     AB                      ; A=十位，B=个位
                MOV     R2, A                   ; 保存十位
                MOV     R1, B                   ; 保存个位
                ;---------- 发送十位 ----------
                MOV     A, R2
                ADD     A, #30H                 ; 转ASCII
                LCALL   SEND_CHAR
                ;---------- 发送个位 ----------
                MOV     A, R1
                ADD     A, #30H                 ; 转ASCII
                LCALL   SEND_CHAR
                ;---------- 发送回车换行 ----------
                MOV     A, #0DH                 ; CR
                LCALL   SEND_CHAR
                MOV     A, #0AH                 ; LF
                LCALL   SEND_CHAR
                RET
;=================================================
; 串口发送单个字符
; 入口：A = 要发送的数据
;=================================================
SEND_CHAR:      
                MOV     SBUF, A
SEND_WAIT:      
                JNB     TI, SEND_WAIT
                CLR     TI
                RET
;=================================================
; 约10ms延时
;=================================================
DELAY10MS:      
                MOV     R7, #20
DLY1:           
                MOV     R6, #250
DLY2:           
                DJNZ    R6, DLY2
                DJNZ    R7, DLY1
                RET
;=================================================
; 数码管段码表
; 注意：
; SEG_P2 对应 P2 口数码管
; SEG_P0 对应 P0 口数码管
;=================================================
SEG_P2:         
                DB      88H, 0EBH, 4CH, 49H, 2BH
                DB      19H, 18H, 0CBH, 08H, 09H
SEG_P0:         
                DB      11H, 0D7H, 32H, 92H, 0D4H
                DB      98H, 18H, 0D3H, 10H, 90H
                END