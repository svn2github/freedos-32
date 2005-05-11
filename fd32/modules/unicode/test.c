/* A simple test program for the FreeDOS-32 Unicode Support Library
 * by Salvo Isaja, 2005
 */
#include "unicode.h"
//#define wchar_t old_wchar_t
#include <stdio.h>
//#undef wchar_t

int main()
{
	wchar_t wc = 0x10A0; //GEORGIAN CAPITAL LETTER AN
	char buf[10];
	int res = unicode_wctoutf8(buf, wc, sizeof(buf));
	if (res > 0)
	{
		unsigned k;
		printf("%X ->", wc);
		for (k = 0; k < res; k++) printf(" %02X", (unsigned char) buf[k]);
		printf("\nres=%i\n", res);
		res = unicode_utf8towc(&wc, buf, sizeof(buf));
		if (res > 0) printf("%X\n", wc);
		wc = unicode_simple_fold(wc);
		printf("%X\n", wc);
	}
	printf("res=%i\n", res);
	return 0;
}
