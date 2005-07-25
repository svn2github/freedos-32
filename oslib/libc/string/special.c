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

#include <ll/ctype.h>
#include <ll/i386/string.h>

FILE(IsSpecial);

int isspecial(double d, char *bufp)
{
    /* IEEE standard number rapresentation */
    register struct IEEEdp {
	unsigned manl:32;
	unsigned manh:20;
	unsigned exp:11;
	unsigned sign:1;
    } *ip = (struct IEEEdp *) &d;

    if (ip->exp != 0x7ff)
	return (0);
    if (ip->manh || ip->manl) {
	if (bufp != NULL)
	    strcpy(bufp, "NaN");
	return (1);
    } else if (ip->sign) {
	if (bufp != NULL)
	    strcpy(bufp, "+Inf");
	return (2);
    } else {
	if (bufp != NULL)
	    strcpy(bufp, "-Inf");
	return (3);
    }
}
