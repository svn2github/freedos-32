/* DPMI Driver for FD32: Interrupt Handlers & similar stuff
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#include <ll/i386/linkage.h>

#define HANDLER(n) \
.globl SYMBOL_NAME(int##n)              ; \
SYMBOL_NAME_LABEL(int##n)               ; \
        pushl $##n                      ; \
        jmp handler
.text
.extern _chandler
        
HANDLER(0x21)
HANDLER(0x31)

handler:
        /* Let's just push something... */
        pushal
        pushl   %ds
        pushl   %es
        pushl   %fs
        pushl   %gs
        pushf
        movw    $0x30, %ax
        movw    %ax, %ds
        movw    %ax, %es
        movw    %ax, %fs
        movw    %ax, %gs
        call    SYMBOL_NAME(chandler)
        popl    %eax            /* Pops Flag Register in EAX */
        movl    %eax, 60(%esp)
        popl    %gs
        popl    %fs
        popl    %es
        popl    %ds
        popal
        addl    $4, %esp
        iret
