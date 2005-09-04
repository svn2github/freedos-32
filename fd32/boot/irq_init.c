/* FreeDOS-32 Runtime ISR Creation
 * Copyright (C) 2005  Nils Labugt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


/*
ESTOPPEL:
  I, Nils Labugt, find strong reason to believe that a copyright holder in
  an operating system do not get any rights to applications that use its
  Applications Programmable Interface trough copyright law, but to remove
  any doubt, I hereby waiver any such rights. If any court should find
  otherwise, then this notice should additionally count as an exception to
  the GPL for such use of this file. Note that this waiver or license
  modification only cover my and my successor' in interest rights, and not
  those of other copyright holders of any work in which this file is
  included. Any future copyright holder in this file, who do not want to be
  bound by this waiver, should remove this notice.
*/

/* Concatenating segments of code into handlers as specified in a string. */

typedef unsigned char uint8_t;
typedef long long int64_t;

#include "entry.h"
#include <ll/i386/mem.h>
#include <ll/limits.h>
#include <malloc.h>
#include <ll/stdlib.h>


static int process_character(void);
static int parse_seg_params(int index, char* seg_start );

#define IRQ_SF_NONE 0
#define IRQ_SF_IF   1
#define IRQ_SF_PARM 2

#define NOSTACK 1
#define NO_ALLOC 2

static void* dummy_address;

#define SEGINFO(s,ch,f) \
{ .segm_pt = &(entseg_##s##_s), .c = ch, .flags = f }

/* A pity there is so few letters in the alphabet...
	maybe a more verbose syntax needed? */
	
struct handler_segment segm_table[] =
{
	SEGINFO(saveES_DS, 'd', IRQ_SF_NONE),		/* 0 */
	SEGINFO(save_newFS, 'f', IRQ_SF_NONE),
	SEGINFO(save_newGS, 'g', IRQ_SF_NONE),
	SEGINFO(restES_DS, 'D', IRQ_SF_NONE),
	SEGINFO(restFS, 'F', IRQ_SF_NONE),
	SEGINFO(restGS, 'G', IRQ_SF_NONE),		/* 5 */
	SEGINFO(new_seg, 'n', IRQ_SF_NONE),
	SEGINFO(same_stack, 'k', IRQ_SF_NONE),
	SEGINFO(if_first_ent, 'i', IRQ_SF_IF),
	SEGINFO(if_first_leave, 'I', IRQ_SF_IF),
	SEGINFO(new_stack_1, 'T', IRQ_SF_PARM),		/* 10 */
	SEGINFO(new_stack_2, 't', IRQ_SF_PARM),
	SEGINFO(save_stack_1, 'u', IRQ_SF_NONE),
	SEGINFO(rest_stack_1, 'U', IRQ_SF_NONE),
	SEGINFO(save_stack_2, 'o', IRQ_SF_NONE),
	SEGINFO(rest_stack_2, 'O', IRQ_SF_NONE),	/* 15 */
	SEGINFO(save_stack_3, 'v', IRQ_SF_NONE),
	SEGINFO(rest_stack_3, 'V', IRQ_SF_NONE),
	SEGINFO(save_stack_4, 'w', IRQ_SF_NONE),
	SEGINFO(rest_stack_4, 'W', IRQ_SF_NONE),
	SEGINFO(nonspecific_EOI_1, 'e', IRQ_SF_NONE),	/* 20 */
	SEGINFO(nonspecific_EOI_2, 'E', IRQ_SF_NONE),
	SEGINFO(specific_EOI_1, 'p', IRQ_SF_NONE),
	SEGINFO(specific_EOI_2, 'P', IRQ_SF_NONE),
	SEGINFO(disable_slave, 'l', IRQ_SF_NONE),
	SEGINFO(disable_master, 'm', IRQ_SF_NONE),	/* 25 */
	SEGINFO(enable_slave, 'L', IRQ_SF_NONE),
	SEGINFO(enable_master, 'M', IRQ_SF_NONE),
	SEGINFO(delay, 'y', IRQ_SF_NONE),
	SEGINFO(cli, 'c', IRQ_SF_NONE),
	SEGINFO(sti, 's', IRQ_SF_NONE),			/* 30 */
	SEGINFO(call_direct, 'a', IRQ_SF_PARM),
	SEGINFO(call_indir_1, 'A', IRQ_SF_PARM),
	SEGINFO(irq_count_1, 'b', IRQ_SF_PARM),
	SEGINFO(irq_count_2, 'B', IRQ_SF_PARM),
	SEGINFO(inc_1, 'q', IRQ_SF_PARM),		/* 35 */
	SEGINFO(inc_2, 'Q', IRQ_SF_PARM),
	SEGINFO(or_mask, 'r', IRQ_SF_PARM),
	SEGINFO(if_irr_set, 'f', IRQ_SF_IF | IRQ_SF_PARM),
	{ .segm_pt = NULL, .c ='\0', .flags = IRQ_SF_NONE }
};

static uint8_t curr_slave_mask;
static uint8_t curr_master_mask;
static int irq_level;
static int64_t _old_stack_store;


static char* current_seg_end;
static char* str_ptr;

static void skip_whitespace(void)
{
	while(*str_ptr == ' '
			  || *str_ptr == '\t'
			  || *str_ptr == '\n'
			  || *str_ptr == '\r')
	{
		str_ptr++;
	}
}

int parse_handler_str( void )
{
	int res;

	while(1)
	{
		skip_whitespace();
		if(*str_ptr == '\0' || *str_ptr == '}')
			return 0;
		res = process_character();
		if(res<0)
			return res;
	}
}

static int process_character(void)
{
	int L, L2;
	int res;
	char* tmp;
	char* seg_start;
	char c;
	int offs, i = 0;

	skip_whitespace();
	c = *str_ptr;
	while(1)
	{
		if(segm_table[i].c == '\0')
			return -1;
		if(segm_table[i].c == c)
			break;
	}
	L = (int)(*(((char*)segm_table[i].segm_pt) - 1));
	seg_start = current_seg_end;
	if(current_seg_end != NULL)
	{
		memcpy(current_seg_end, segm_table[i].segm_pt, L);
		current_seg_end += L;
	}
	str_ptr++;
	if(segm_table[i].flags | IRQ_SF_PARM)
	{
		res = parse_seg_params( i, seg_start );
		if (res < 0)
			return res;
	}
	if(segm_table[i].flags | IRQ_SF_IF)
	{
		skip_whitespace();
		if(*str_ptr != '{')
			return -1;
		str_ptr++;
		tmp = current_seg_end;
		res = parse_handler_str();
		if(res<0)
			return res;
		skip_whitespace();
		if(*str_ptr != '}')
			return -1;
		str_ptr++;
		L2 = current_seg_end - tmp;
		offs = (int)(*(((char*)segm_table[i].segm_pt) - 2));
		if(L2 + *(((char*)segm_table[i].segm_pt) + offs) > 127)
			return -1;
		if(current_seg_end != NULL)
			*(seg_start + offs) += L2;
	}
	return 0;
}


int parse_next_param(int* result, int flags)
{
	int i = 0, digits = 0, negative = FALSE;
	unsigned n = 0;
	size_t size;
	char* stack;
	char c;
	int res;

	skip_whitespace();
	switch(*str_ptr)
	{
		case '0':
			str_ptr++;
			if(*str_ptr != 'x')
			{
				*result = 0;
				return 0;
			}
			str_ptr++;
			while(1)
			{
				c = *str_ptr;
				if(c >= '0' && c <= '9')
					c -= '0';
				else if(c >= 'a' && c <= 'f')
					c -= 'a' - 16;
				else if(c >= 'A' && c <= 'F')
					c -= 'A' - 16;
				else
					break;
				digits++;
				i <<= 4;
				i |= c;
			}
			if( digits > 8 || digits == 0)
				return -1;
			*result = i;
			return 0;
		case 's':
			if(flags & NOSTACK)
				return -1;
			str_ptr++;
			skip_whitespace();
			if(*str_ptr != '[')
				return -1;
			res = parse_next_param((int*)&size, NOSTACK);
			if(res<0)
				return res;
			if(size > 0x40000000)
				return -1;
			skip_whitespace();
			if(*str_ptr != ']')
				return -1;
			str_ptr++;
			/* FIXME: alignement? */
			/* FIXME: poitner to stack will be lost, impossible to free stack memory */
			if(!(flags & NO_ALLOC))
				stack = kmalloc(size);
			else
				return 0;
			if(stack == NULL)
				return -1;
			*result = (int)((size_t)stack + size - 4);
			return 0;
		case '-':
			negative = TRUE;
			str_ptr++;
		default:
			if( !(*str_ptr >= '0' && *str_ptr <= '9') )
				return -1;
			while(1)
			{
				c = *str_ptr;
				if(c >= '0' && c <= '9')
					c -= '0';
				else
					break;
				if( n > UINT_MAX / 10 )
					return -1;
				if( n == UINT_MAX / 10 &&  c > UINT_MAX % 10)
					return -1;
				n *= 10;
				n += c;
			}
			if(negative)
			{
				if(n > -(long long)INT_MIN)
					return -1;
				*result = -n;
				return 0;
			}
			*result = n;
			return 0;
	}
}

static void seg_param_write32( int n, int index, char* seg_start, int p)
{
	int offs1, offs2;

	offs1 = -2 - n;
	if(segm_table[index].flags & IRQ_SF_IF)
		offs1--;
	offs2 = (int)(*(((char*)segm_table[index].segm_pt) + offs1));
	*((int*)(seg_start + offs2)) = p;
}

static void seg_param_write8( int n, int index, char* seg_start, char p)
{
	int offs1, offs2;

	offs1 = -2 - n;
	if(segm_table[index].flags & IRQ_SF_IF)
		offs1--;
	offs2 = (int)(*(((char*)segm_table[index].segm_pt) + offs1));
	*((char*)(seg_start + offs2)) = p;
}

static void seg_param_adjust( int n, int index, char* seg_start, int p)
{
	int offs1, offs2;

	offs1 = -2 - n;
	if(segm_table[index].flags & IRQ_SF_IF)
		offs1--;
	offs2 = (int)(*(((char*)segm_table[index].segm_pt) + offs1));
	*((int*)(seg_start + offs2)) += p;
}

static int parse_seg_params(int index, char* seg_start )
{
	char c;
	int param[3] = {0,0,0};
	int n = 0;
	int res;
	
	skip_whitespace();
	if( *str_ptr != '(' )
		return -1;
	str_ptr++;
	while(1)
	{
		if( seg_start == NULL )
			res = parse_next_param(&param[n], NO_ALLOC);
		else
			res = parse_next_param(&param[n], 0);
		if(res<0)
			return res;
		skip_whitespace();
		c = *str_ptr;
		if( c == ')' )
			break;
		if( c == ',' )
		{
			n++;
			continue;
		}
		return -1;
	}
	switch(index)
	{
		case 10:
		case 11:
		case 32:
		case 33:
		case 35:
			if(n != 1)
				return -1;
			seg_param_write32( 0, index, seg_start, param[0]);
			return 0;
		case 31:
			if(n != 1)
				return -1;
			seg_param_adjust( 0, 31, seg_start, param[0]-
					(size_t)&dummy_address);
			return 0;
		case 34:
		case 36:
			if(n != 1)
				return -1;
			seg_param_write32( 0, index, seg_start, param[0]);
			seg_param_write32( 1, index, seg_start, param[0] + 4 );
			return 0;
		case 37:
			if(n != 2)
				return -1;
			seg_param_write32( 0, index, seg_start, param[0]); /* address */
			seg_param_write32( 1, index, seg_start, param[1]); /* mask */
			return 0;
		case 38:
			if(n != 1)
				return -1;
			if(param[0] | 0xFFFFFF00)
				return -1;
			seg_param_write8( 0, index, seg_start, param[0]);
			return 0;
	}
	return 0; /* We never get here, just to avoid warning */
}	
		


