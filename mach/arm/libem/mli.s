.define	.mli

	.sect .text

.mli:	LDMFD R12<,{R1,R2}
	MOV R3,#0
	MOV R0,#0
	CMP R1,#0
	ADD.MI R3,R3,#1
	RSB.MI R1,R1,#0
	CMP R2,#0
	ADD.MI R3,R3,#1
	RSB.MI R2,R2,#0
	CMP R1,R2
	BLT 1F
	MOV R4,R1
	MOV R1,R2
	MOV R2,R4
1:
	TST R1,#1
	ADD.NE R0,R0,R2
	MOV R2,R2,LSL #1
	MOV.S R1,R1,LSR #1
	BNE 1B
	TST R3,#1
	RSB.NE R0,R0,#0
	MOV R15,R14
