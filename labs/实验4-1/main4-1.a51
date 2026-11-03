      ORG 0000H
	  MOV P0,#11H
	  MOV P2,#88H
	  MOV R0,#12H
	  MOV R1,#0
	  MOV R2,#0
	  ANL PCON,#7FH		;波特率不倍速
	  ANL 8EH,#0BFH	;定时器1时钟为Fosc/12,即12T
 	  ANL 8EH,#0FEH	;串口1选择定时器1为波特率发生器
	  MOV TMOD,#20H
      MOV TL1,#0FDH		;设定定时初值
	  MOV TH1,#0FDH		;设定定时器重装值
      SETB TR1
      MOV SCON,#50H ;串行口工作方式1			 
 RECE:JNB RI,$
      CLR RI
      MOV R0,SBUF
	  MOV A,R0
	  ANL A,#0FH
	  MOV R1,A
	  MOV A,R0		  
	  ANL A,#0F0H
	  SWAP A
	  MOV R2,A
      MOV A,R1  
      MOV DPTR,#TAB
      MOVC A,@A+DPTR
      MOV P2,A 
      MOV A,R2 
	  MOV DPTR,#TABA
      MOVC A,@A+DPTR
      MOV P0,A	      
	  SJMP RECE 
 TAB: DB 88H,0EBH,4CH,49H,2BH,19H,18H,0CBH,08H,09H	 ;P2
 TABA:DB 11H,0D7H,32H,92H,0D4H,98H,18H,0D3H,10H,90H	 ;P0
END