/* Dynalynk
 * Portable dynamic linker for object files
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#define T_NULL		0
#define T_VOID		1	/* function argument (only used by compiler) */
#define T_CHAR		2	/* character		*/
#define T_SHORT		3	/* short integer	*/
#define T_INT		4	/* integer		*/
#define T_LONG		5	/* long integer		*/
#define T_FLOAT		6	/* floating point	*/
#define T_DOUBLE	7	/* double word		*/
#define T_STRUCT	8	/* structure 		*/
#define T_UNION		9	/* union 		*/
#define T_ENUM		10	/* enumeration 		*/
#define T_MOE		11	/* member of enumeration*/
#define T_UCHAR		12	/* unsigned character	*/
#define T_USHORT	13	/* unsigned short	*/
#define T_UINT		14	/* unsigned integer	*/
#define T_ULONG		15	/* unsigned long	*/
#define T_LNGDBL	16	/* long double		*/


#define DT_NON		(0)	/* no derived type */
#define DT_PTR		(1)	/* pointer */
#define DT_FCN		(2)	/* function */
#define DT_ARY		(3)	/* array */

#define BTYPE(x)	((x) & 0x0F)

#define ISNON(x)	(((x) & 0xF0) == (DT_NON << 4))
#define ISPTR(x)	(((x) & 0xF0) == (DT_PTR << 4))
#define ISFCN(x)	(((x) & 0xF0) == (DT_FCN << 4))
#define ISARY(x)	(((x) & 0xF0) == (DT_ARY << 4))
#define ISTAG(x)	((x)==C_STRTAG||(x)==C_UNTAG||(x)==C_ENTAG)
#define DECREF(x) ((((x)>>N_TSHIFT)&~N_BTMASK)|((x)&N_BTMASK))

char *type[] = {
  "No Symbol",
  "void function argument",
  "character",
  "short integer",
  "integer",
  "long integer",
  "floating point",
  "double precision float",
  "structure",
  "union",
  "enumeration",
  "member of enumeration",
  "unsigned character",
  "unsigned short",
  "unsigned integer",
  "unsigned long"
};
