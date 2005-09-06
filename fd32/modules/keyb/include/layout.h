#ifndef __LAYOUT_H__
#define __LAYOUT_H__

#include <dr-env.h>

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
	DWORD size; /* The whole size of CONTROL BLOCK */

	struct {
	BYTE submap_num;
	BYTE plane_num;
	BYTE dec_sep;
	BYTE cur_submap;
	WORD mcbid;
	DWORD next;
	WORD id;
	char name[8];
	} *ref; /* The CONTROL BLOCK info */

	SubMapping *submaps;
	Plane *planes;
} __attribute__ ((packed)) KeybCB;
#define SIZEOF_CB (20)


int keyb_layout_choose(const char *filename, char *layout_name);
int keyb_layout_free(void);
int keyb_layout_decode(BYTE c, int flags, int lock);

#endif
