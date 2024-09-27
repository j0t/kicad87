	.file	"w.c"
	.text
	.p2align 4
	.globl	CallFoo
	.def	CallFoo;	.scl	2;	.type	32;	.endef
	.seh_proc	CallFoo
CallFoo:
.LFB0:
	.seh_endprologue
	rex.W jmp	*__imp__CallFoo(%rip)
	.seh_endproc
	.ident	"GCC: (Rev4, Built by MSYS2 project) 12.2.0"
	.section .drectve
	.ascii " -export:\"CallFoo\""
