/*
Segments of code to be concatenated into interrupt handler entries.
Unfortunately, I forgot about hardware stack switches when writing this, witch
makes things more complicated, and more segments needs to be added. Also,
there should be segments for saving and restoring more context.
*/
		
#define STARTSEG1(s) \
.global entseg_##s##_s;\
.byte	entseg_##s##_e - entseg_##s##_s;\
entseg_##s##_s:

#define STARTSEG2(s, d1) \
.global entseg_##s##_s;\
.byte	entparm_1_##s - entseg_##s##_s + (d1);\
.byte	entseg_##s##_e - entseg_##s##_s;\
entseg_##s##_s:

#define STARTSEG3(s, d1, d2) \
.global entseg_##s##_s;\
.byte	entparm_1_##s - entseg_##s##_s + (d1);\
.byte	entparm_2_##s - entseg_##s##_s + (d2);\
.byte	entseg_##s##_e - entseg_##s##_s;\
entseg_##s##_s:

#define STARTSEG4(s, d1, d2, d3) \
.global entseg_##s##_s;\
.byte	entparm_1_##s - entseg_##s##_s + (d1);\
.byte	entparm_2_##s - entseg_##s##_s + (d2);\
.byte	entparm_3_##s - entseg_##s##_s + (d3);\
.byte	entseg_##s##_e - entseg_##s##_s;\
entseg_##s##_s:

#define ENDSEG(s) \
entseg_##s##_e:

#define SEGPARM1(s) \
entparm_1_##s##:

#define SEGPARM2(s) \
entparm_2_##s##:

#define SEGPARM3(s) \
entparm_3_##s##:

dummy_const=0
/* FIXME */
#define X_FLATDATA_SEL 0 


.extern GDT_base
.extern irq_level
.extern _old_stack_store


.text

.global dummy_address
dummy_address:

vector=0
.global irq_entries_master
irq_entries_master:
.rept 8
.align 8
1:	pushal
	movb $vector,%cl
	jmp dummy_target
vector=vector+1
.endr

vector=0
.global irq_entries_slave
irq_entries_slave:
.rept 8
.align 8
1:	pushal
	movb $vector,%cl
	jmp dummy_target
vector=vector+1
.endr


STARTSEG1(saveES_DS)
	pushl %es
	pushl %ds
ENDSEG(saveES_DS)


/* Hardware irq handlers will typically not use FS and GS regs */
STARTSEG1(save_newFS)
	pushl %fs
	movw %si, %fs
ENDSEG(save_newFS)


STARTSEG1(save_newGS)
	pushl %gs
	movw %si, %gs
ENDSEG(save_newGS)


STARTSEG1(restES_DS)
	popl %ds
	popl %es
ENDSEG(restES_DS)


STARTSEG1(restFS)
	popl %fs
ENDSEG(restFS)


STARTSEG1(restGS)
	popl %gs
ENDSEG(restGS)


/* Loading flat address space int DS and ES and saving SS */
STARTSEG1(new_seg)
	movw $(X_FLATDATA_SEL),%si
	movw %si, %es
	movw %si, %ds
	movw %ss, %bx
ENDSEG(new_seg)


/* Access the GDT to get base address and add to ESP, simmilar to OSLib */
STARTSEG1(same_stack)
	movl %ebx, %edi
	andl $0xfff8, %edi
	movb GDT_base+4(,%edi,), %al
	movb GDT_base+7(,%edi,), %ah
	shl $16, %eax
	movw GDT_base+2(%edi), %ax
	movw %si, %ss
	addl %eax, %esp
	pushl %ebx
	pushl %eax
ENDSEG(same_stack)


/* irq_level should be initialized to -1 */
STARTSEG2(if_first_ent, -1)
	incl irq_level
	jne 1f
SEGPARM1(if_first_ent)
1:
ENDSEG(if_first_ent)


STARTSEG2(if_first_leave, -1)
	subl $-1, irq_level
	jnc 1f
SEGPARM1(if_first_leave)
1:
ENDSEG(if_first_leave)


STARTSEG2(new_stack_1, -4)
	movl %esp, %edi
	movw %si, %ss
	movl $(dummy_const), %esp
SEGPARM1(new_stack_1)
ENDSEG(new_stack_1)


STARTSEG2(new_stack_2, -4)
	movl %esp, %edi
	movl $(dummy_const), %esp
SEGPARM1(new_stack_2)
ENDSEG(new_stack_2)


STARTSEG1(save_stack_1)
	pushl %ebx
	pushl %edi
ENDSEG(save_stack_1)


STARTSEG1(rest_stack_1)
	popl %edi
	popl %ebx
	movw %bx, %ss
	movl %edi, %esp
ENDSEG(rest_stack_1)


STARTSEG1(save_stack_2)
	movl %edi, _old_stack_store
	movl %ebx, _old_stack_store+4
ENDSEG(save_stack_2)


STARTSEG1(rest_stack_2)
#if 0
	lss _old_stack_store, %esp
#else
	movl _old_stack_store+4, %ebx
	mov %bx, %ss
	movl _old_stack_store, %esp
#endif
ENDSEG(rest_stack_2)


STARTSEG1(save_stack_3)
	pushl %edi
ENDSEG(save_stack_3)


STARTSEG1(rest_stack_3)
	popl %esp
no_stack_switch5:
ENDSEG(rest_stack_3)


STARTSEG1(save_stack_4)
	movl %esp, _old_stack_store
ENDSEG(save_stack_4)


STARTSEG1(rest_stack_4)
	movl _old_stack_store, %esp
ENDSEG(rest_stack_4)
	

/* Master only */
STARTSEG1(nonspecific_EOI_1)
	movb $0x20, %al
	outb %al, $0x20
ENDSEG(nonspecific_EOI_1)


/* Master and slave */
STARTSEG1(nonspecific_EOI_2)
	movb $0x20, %al
	outb %al, $0xA0
	outb %al, $0x20
ENDSEG(nonspecific_EOI_2)


/* Master only */
STARTSEG1(specific_EOI_1)
	movb $0x20, %al
	movl $0x20, %edx
	outb %al, %dx
ENDSEG(specific_EOI_1)


/* Master and slave */
STARTSEG1(specific_EOI_2)
	movb %cl, %al
	add $0x60, %al
	outb %al, $0xA0
	movb $2, %al
	outb %al, $0x20 
ENDSEG(specific_EOI_2)


STARTSEG1(disable_slave)
	movb %bl, %al
	mov $1, %al
	shl %cl, %al
	orb curr_slave_mask, %al
	outb %al, $0xa1
	movb %al, curr_slave_mask
ENDSEG(disable_slave)

	
STARTSEG1(disable_master)
	movb %bl, %al
	mov $1, %al
	shl %cl, %al
	orb curr_master_mask, %al
	outb %al, $0x21
	movb %al, curr_master_mask
ENDSEG(disable_master)


STARTSEG1(enable_slave)
	movb %bl, %al
	mov $1, %al
	shl %cl, %al
	notb %al
	andb curr_slave_mask, %al
	outb %al, $0xa1
	movb %al, curr_slave_mask
ENDSEG(enable_slave)

	
STARTSEG1(enable_master)
	movb %bl, %al
	mov $1, %al
	shl %cl, %al
	notb %al
	and curr_master_mask, %al
	outb %al, $0x21
	movb %al, curr_master_mask
ENDSEG(enable_master)

	
STARTSEG1(delay)
	jmp ed1
ed1:	jmp ed2
ed2:	jmp ed3
ed3:
ENDSEG(delay)


STARTSEG1(cli)
	cli
ENDSEG(cli)


STARTSEG1(sti)
	sti
ENDSEG(sti)


STARTSEG2(call_direct, -4)
	cld
	call dummy_address
SEGPARM1(call_direct)
ENDSEG(call_direct)


STARTSEG2(call_indir_1, -4)
	andl $0xFF, %ecx
	cld
	call *dummy_address(,%ecx,4)
SEGPARM1(call_indir_1)
ENDSEG(call_indir_1)


/* 32 bit counter */
STARTSEG2(irq_count_1, -4)
	andl $0xFF, %ecx
	incl dummy_address(,%ecx,4)
SEGPARM1(irq_count_1)
ENDSEG(irq_count_1)


/* 64 bit counter */
STARTSEG3(irq_count_2, -4, -4)
	andl $0xFF, %ecx
	addl $1, dummy_address(,%ecx,4)
SEGPARM1(irq_count_2)
	adcl $0, dummy_address+4(,%ecx,4)
SEGPARM2(irq_count_2)
ENDSEG(irq_count_2)


/* 32 bit counter */
STARTSEG2(inc_1, -4)
	incl dummy_address(,%ecx,4)
SEGPARM1(inc_1)
ENDSEG(inc_1)


/* 64 bit counter */
STARTSEG3(inc_2, -4, -4)
	addl $1, dummy_address
SEGPARM1(inc_2)
	adcl $0, dummy_address+4
SEGPARM2(inc_2)
ENDSEG(inc_2)


STARTSEG3(or_mask, -8, -4)
	orl $dummy_const, dummy_address
SEGPARM1(or_mask)   /* address */
SEGPARM2(or_mask)   /* mask */
ENDSEG(or_mask)


/* Tiesting the irr may be useful as a timer overrun test */
/* dummy_const is mask for testing */
/* Warning: this may be slow */
STARTSEG3(if_irr_set, -1, -1)
	movb $0x0A, %al
	outb %al, $0x20
	inb $0x20, %al
	testb $dummy_const, %al /* dummy_const is mask for testing */
SEGPARM2(if_irr_set)
	jz 1f
SEGPARM1(if_irr_set)
1:
ENDSEG(if_irr_set)


/* Put this here to make ensure 32 bit jump instruction */
.global dummy_target
dummy_target:
