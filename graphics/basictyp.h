#ifndef __BASICTYP_H
#define __BASICTYP_H

/* Signed integer types */
typedef signed   char      CHAR;     /* 8-bit  */
typedef signed   short     SHORT;    /* 16-bit */
typedef signed   long      LONG;     /* 32-bit */
typedef signed   long long LONGLONG; /* 64-bit */

/* Unsigned integer types */
typedef unsigned char      BYTE;     /* 8-bit  */
typedef unsigned short     WORD;     /* 16-bit */
typedef unsigned long      DWORD;    /* 32-bit */
typedef unsigned long long QWORD;    /* 64-bit */

/* Special data types */
typedef struct RECT
{
  LONG left;
  LONG top;
  LONG right;
  LONG bottom;
}
RECT;

#endif

