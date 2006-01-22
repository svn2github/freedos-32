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

.data
in_irq: .int 0
.extern pit_process
.text
.global pit_isr
pit_isr:
	pusha
	push %ds
	push %es
	push %fs
	push %gs

	mov $(X_FLATDATA_SEL), %ax
	mov %ax, %ds
	mov %ax, %es

/*	cmp $0, in_irq
	jnz exit
	incl in_irq*/
	call pit_process
/*	decl in_irq*/
exit:
	mov $0x20, %dx
	mov $0x20, %al
	out %al, %dx
	pop %gs
	pop %fs
	pop %es
	pop %ds
	popa
	iret
