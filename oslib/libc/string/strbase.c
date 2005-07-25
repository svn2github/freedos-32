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

FILE(strbase);

int isalnum(char c)
{
	return (isalpha(c) || isdigit(c));
}

int isalpha(char c)
{
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z'))
       return(1);
    else return(0);
}

int iscntrl(char c)
{
  if (c < ' ') {
    return 1;
  }
  if (c == 0x7F) {
    return 1;
  }

  return 0;
}


int islower(char c)
{
    if ((c >= 'a' && c <= 'z'))
       return(1);
    else return(0);
}

int isspace(char c)
{
    if ((c >= 0x09 && c <= 0x0d) || (c == 0x20)) return(1);
    return(0);
}

int isupper(char c)
{
    if ((c >= 'A' && c <= 'Z'))
       return(1);
    else return(0);
}

char toupper(char c)
{
    if (c >= 'a' && c <= 'z') return(c-'a'+'A');
    else return(c);
}

char tolower(char c)
{
    if (c >= 'A' && c <= 'Z') return(c-'A'+'a');
    else return(c);
}
