/* Project:     OSLib
 * Description: The OS Construction Kit
 * Date:                1.6.2000
 * Idea by:             Luca Abeni & Gerardo Lamastra
 *
 * OSLib is an SO project aimed at developing a common, easy-to-use
 * low-level infrastructure for developing OS kernels and Embedded
 * Applications; it partially derives from the HARTIK project but it
 * currently is independently developed.
 *
 * OSLib is distributed under GPL License, and some of its code has
 * been derived from the Linux kernel source; also some important
 * ideas come from studying the DJGPP go32 extender.
 *
 * We acknowledge the Linux Community, Free Software Foundation,
 * D.J. Delorie and all the other developers who believe in the
 * freedom of software and ideas.
 *
 * For legalese, check out the included GPL license.
 */

/*	Console output functions - part 2	*/

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/cons.h>
/* #include <xsys.h>*/
#include <ll/i386/string.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/stdio.h>
#include <ll/stdarg.h>

FILE(Cons2);

#define PAGE_SIZE		2048
#define PAGE_MAX		8

/* CGA compatible registers */

#define CGA_INDEX_REG	0x3D4
#define CGA_DATA_REG	0x3D5

#define VIDEO_ADDRESS_MSB	0x0C
#define VIDEO_ADDRESS_LSB	0x0D


extern int active_page;
extern int visual_page;
static int curs_x[PAGE_MAX];
static int curs_y[PAGE_MAX];
extern BYTE bios_x, bios_y, bios_attr;

void set_visual_page(int page)
{
    unsigned short page_offset;
    
    page_offset = page * PAGE_SIZE;
    visual_page = page;
    outp(CGA_INDEX_REG, VIDEO_ADDRESS_LSB);
    outp(CGA_DATA_REG, page_offset & 0xFF);
    outp(CGA_INDEX_REG, VIDEO_ADDRESS_MSB);
    outp(CGA_DATA_REG, (page_offset >> 8) & 0xFF);
}

void set_active_page(int page)
{
    curs_x[active_page] = bios_x;
    curs_y[active_page] = bios_y;
    bios_x = curs_x[page];
    bios_y = curs_y[page];
    active_page = page;
}

int get_visual_page(void)
{
    return(visual_page);
}

int get_active_page(void)
{
    return(active_page);
}

void _clear(char c,char attr,int x1,int y1,int x2,int y2)
{
    register int i,j;
    WORD w = attr;
    
    w <<= 8; w |= c;
    for (i = x1; i <= x2; i++)
    	for (j = y1; j <= y2; j++) 
            lmempokew((LIN_ADDR)(0xB8000 + 2*i+160*j + 2*active_page*PAGE_SIZE),w);
    place(x1,y1);
    bios_y = y1;
    bios_x = x1;
}

void clear()
{
    _clear(' ',bios_attr,0,0,79,24);
}

void puts_xy(int x,int y,char attr,char *s)
{
    LIN_ADDR v = (LIN_ADDR)(0xB8000 + (80*y+x)*2 + active_page*(2*PAGE_SIZE));
    while (*s != 0) {
	/* REMEMBER! This is a macro! v++ is out to prevent side-effects */
	lmempokeb(v,*s); s++; v++;
	lmempokeb(v,attr); v++;
    }
}

void putc_xy(int x,int y,char attr,char c)
{
    LIN_ADDR v = (LIN_ADDR)(0xB8000 + (80*y+x)*2 + active_page*(2*PAGE_SIZE));
    /* REMEMBER! This is a macro! v++ is out to prevent side-effects */
    lmempokeb(v,c); v++;
    lmempokeb(v,attr);
}

char getc_xy(int x,int y,char *attr,char *c)
{
    LIN_ADDR v = (LIN_ADDR)(0xB8000 + (80*y+x)*2 + active_page*(2*PAGE_SIZE));
    char r;
    r = lmempeekb(v); v++;
    if (c != NULL) *c = r;
    r = lmempeekb(v);
    if (attr != NULL) *attr = r;
    return(r);
}
