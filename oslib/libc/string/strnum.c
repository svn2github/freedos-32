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

FILE(strnum);

int isnumber(char c, int base)
{
    char max;

    if (base > 9) {
      max = '9';
    } else {
      max = '0' + base - 1;
    }
    if (c >= '0' && c <= max) {
        return 1;
    }
   
    if (base < 11) {
      return 0;
    }

    max = 'A' + (base - 11);
    if (toupper(c) >= 'A' && toupper(c) <= max) {
        return 1;
    }
   
    return 0;
}

int tonumber(char c)
{
    if (c >= '0' && c <= '9') return(c - '0');
    else if (c >= 'A' && c <= 'F') return(c - 'A' + 10);
    else if (c >= 'a' && c <= 'f') return(c - 'a' + 10);
    else return(c);
}

char todigit(int c)
{
    if (c >= 0 && c <= 9) return(c+'0');
    else if (c >= 10 && c <= 15) return(c + 'A' - 10);
    else return(c);    
}

int isxdigit(char c)
{
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'))
       return(1);
    else return(0);
}

int isdigit(char c)
{
    if ((c >= '0' && c <= '9'))
       return(1);
    else return(0);
}
