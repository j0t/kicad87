	.bss
	.align 16
	.globl __imp_Table
__imp_Table:
	.space 16
	
	.text
    .globl setup
setup:
	movq __imp_Table(%rip), %rax
    ret
	