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

#include <ll/i386/string.h>

FILE(strstr);

#if 0
char *strstr(const char *haystack, const char *needle)
{
	int hlen;
	int nlen;

	hlen = strlen((char *)haystack);
	nlen = strlen((char *)needle);
	while (hlen >= nlen)
	{
		if (!memcmp(haystack, needle, nlen))
			return (char *)haystack;

		haystack++;
		hlen--;
	}
	return 0;
}
#else

/* The following was copied from Linux 2.6.12 by Nils Labugt Jul 28 2005 */
/* see string.c */

/* Nils: FIXME: arch dependent should go somewhere else? */
/* string.h is big enough as it is (compile time) */

/**
 * strstr - Find the first substring in a %NUL terminated string
 * @haystack: The string to be searched
 * @needle: The string to search for
 */
char * strstr(const char *haystack,const char *needle)
{
int	d0, d1;
register char * __res;
__asm__ __volatile__(
	"movl %6,%%edi\n\t"
	"repne\n\t"
	"scasb\n\t"
	"notl %%ecx\n\t"
	"decl %%ecx\n\t"	/* NOTE! This also sets Z if searchstring='' */
	"movl %%ecx,%%edx\n"
	"1:\tmovl %6,%%edi\n\t"
	"movl %%esi,%%eax\n\t"
	"movl %%edx,%%ecx\n\t"
	"repe\n\t"
	"cmpsb\n\t"
	"je 2f\n\t"		/* also works for empty string, see above */
	"xchgl %%eax,%%esi\n\t"
	"incl %%esi\n\t"
	"cmpb $0,-1(%%eax)\n\t"
	"jne 1b\n\t"
	"xorl %%eax,%%eax\n\t"
	"2:"
	:"=a" (__res), "=&c" (d0), "=&S" (d1)
	:"0" (0), "1" (0xffffffff), "2" (haystack), "g" (needle)
	:"dx", "di");
return __res;
}
#endif
