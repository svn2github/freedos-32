.data
.text
.globl start
.globl _start

start:
_start:
	call _main
	movl $0x4c00, %eax
	movl $2, %ebx
	movl $3, %ecx
	movl $4, %edx
	movl $8, %esi
	movl $9, %edi
	int $0x21
	ret
	movl $0xb8A00, %ebx
loop:
	movb $' ', %ah
	movb $'H', %al
	movw %ax, (%ebx)
	addl $2, %ebx
	movb $'E', %al
	movw %ax, (%ebx)
	addl $2, %ebx
	movb $'L', %al
	movw %ax, (%ebx)
	addl $2, %ebx
	movb $'L', %al
	movw %ax, (%ebx)
	addl $2, %ebx
	movb $'O', %al
	movw %ax, (%ebx)
	ret

