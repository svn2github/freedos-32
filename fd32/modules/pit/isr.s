/* FreeDOS-32 PIT-driven Event Management version 0.1
 * Copyright (C) 2006  Salvatore ISAJA
 *
 * This file "isr.s" is part of the
 * FreeDOS-32 PIT-driven Event Management (the Program).
 *
 * The Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <ll/i386/sel.h>
#include <ll/i386/linkage.h>

.data
in_irq: .int 0
.extern SYMBOL_NAME(pit_process)
.extern SYMBOL_NAME(ticks)

.text
.global SYMBOL_NAME(pit_isr)
SYMBOL_NAME_LABEL(pit_isr)
	pusha
	push %ds
	mov $0x20, %dx
	mov $0x20, %al
	out %al, %dx

	mov $(X_FLATDATA_SEL), %ax
	mov %ax, %ds

	incl 0x046c /* BIOS timer */
/*	incl SYMBOL_NAME(ticks) */
	addl $0x10000, SYMBOL_NAME(ticks)
/*	jnz 1f */
	jnc 1f 
	incl SYMBOL_NAME(ticks)+4
1:
	cmp $0, in_irq
	jnz 2f
	incl in_irq
	sti
	cld
	push %es
	mov %ax, %es
	push %fs
	push %gs
	call SYMBOL_NAME(pit_process)
	pop %gs
	pop %fs
	pop %es
	cli
	decl in_irq
2:
	pop %ds
	popa
	iret



.global SYMBOL_NAME(pit_isr2)
SYMBOL_NAME_LABEL(pit_isr2)
	pusha
	push %ds
	mov $0x20, %dx
	mov $0x20, %al
	out %al, %dx
	
	mov $(X_FLATDATA_SEL), %ax
	mov %ax, %ds
	
	movl SYMBOL_NAME(pit_ticks), %ebx
	movl SYMBOL_NAME(ticks), %ecx
	movl %ecx, %edx
	addl %ebx, %ecx
	movl %ecx, SYMBOL_NAME(ticks)
	jnc 1f
	incl SYMBOL_NAME(ticks)+4
1:
	addw %bx, %dx
	jnc 1f
	incl 0x046c /* BIOS timer */
1:
	cmp $0, in_irq
	jnz 2f
	incl in_irq
	sti
	cld
	push %es
	mov %ax, %es
	push %fs
	push %gs
	call SYMBOL_NAME(pit_process)
	pop %gs
	pop %fs
	pop %es
	cli
	decl in_irq
2:
	pop %ds
	popa
	iret

