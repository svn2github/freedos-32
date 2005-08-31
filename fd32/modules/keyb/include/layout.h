#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include <windows.h>


/*
000  3 BYTES   Signature:  "KLF"
			   Signature for the KLF file

003	WORD	Version of the KL structure (in this case 0100h)

005	BYTE	Length n of the following namestring

006  n BYTES   namestring (see table 2)
 */

/* KL compacted library header */
typedef struct KHeader
{
	char Sig[3];
	BYTE version[2];
	BYTE closed;
	BYTE length;
	BYTE *description;
} KHeader;
#define SIZEOF_KHEADER (7)

typedef struct KLHeader
{
	WORD klsize;
	BYTE length;
	BYTE *id;
} KLHeader;
#define SIZEOF_KLHEADER (3)

typedef struct KeyType
{
	BYTE scancode;
	struct {
		BYTE data_length: 3;
		BYTE reserved:    1;
		BYTE lock:        1;
		BYTE numlock_af:  1;
		BYTE capslock_af: 1;
		BYTE s_flag:      1;
	} flags;
	BYTE cmdbits;
	BYTE data[1];	/* a variable data */
} KeyType;

typedef struct SubMapping
{
	WORD codepage;  /* codepage of the submapping */
	WORD keysofs;   /* pointer to KEYS section (index in object list) */
	WORD diacofs;   /* pointer to COMBI section (index in object list) */
	WORD strofs;    /* pointer to String section (index in object list) */
} SubMapping;

typedef struct Plane
{
	WORD wtStd;	 /* Wanted standard flags */
	WORD exStd;	 /* Excluded standard flags */
	WORD wtUsr;	 /* Wanted user-defined flags */
	WORD exUsr;	 /* Excluded user-defined flags */
} Plane;

typedef struct KeybCB
{
	struct {
	BYTE submap_num;
	BYTE plane_num;
	BYTE dec_sep;
	BYTE cur_submap;
	WORD mcbid;
	DWORD next;
	WORD id;
	char name[8];
	} *ref;
	SubMapping *submaps;
	Plane *planes;
} __attribute__ ((packed)) KeybCB;
#define SIZEOF_CB (20)

/*
000  BYTE      Number of submappings (n+1) described in the block
               General submapping is counted, and is usually numbered as 0.
001  BYTE      Number of additional planes (m) that are globally defined for
               the whole keyb control block. The total number of planes is
               m+2 (there are two implicit ones, see TABLE 3)
               The maximum number of planes is 10 (that is, m=8)
002  BYTE      Character to be used as decimal separator character by
               the driver when using this layout.
               0 when the default character (usually .) should be used.
003  BYTE      Current particular submapping in use (1 to n)
004  WORD      MCB-ID of the KeybCB, only if the KeybCB resides in a MCB
               that must be deallocated if the KeybCB will no longer be
               used.
006  DWORD     PTR=> Next KeybCB
               KeybCBs may be organized in a linked list, if the keyboard
               driver supports more than one.
               Set to 00:00 if there are no more KeybCBs.
010  WORD      ID Word of the current KeybCB (see ofs 012)
012  8 BYTES   0-padded string of the name with which the KeybCB has been
               loaded. It is usually a two-letter identifier, such as UK,
               GR or SP.
               KeybCBs are usually loaded as single "keyboard layouts"
               identified by a short string and an identifier word in case
               there are several models appliable to the same string ID.
               (for the identifier word, see ofs 010)

     ---Submappings---
020  n+1 QWORDS Each QWORD describes a submapping (see TABLE 2)
                The general submappings comes first, the others are the
                particular submappings.

     ---Planes---
020 + (n+1)*8
     m   QWORDS Each QWORD describes an additional plane (see TABLE 3)

     ---Data---
020 + (n+1)*8 + m*8
                Data blocks start here
*/

int keyb_choose_layout(const char *filename, char *layout_name);

#endif
