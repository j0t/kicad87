	.macro WRAP Pname
	.p2align 4
	.globl	"\Pname"
	.seh_proc	"\Pname"
"\Pname":
	.seh_endprologue
	rex.W jmp	*"__imp__\Pname"(%rip)
	.seh_endproc
	.endm
