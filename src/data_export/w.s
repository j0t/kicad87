	.bss
	.align 16
	.globl __imp_Table
__imp_Table:
	.space 16
	
	.text
    .globl setupTable
setupTable:
    # extern "C" void setupTable( FARPROC )
	movq %rcx, __imp_Table(%rip)
    ret
	