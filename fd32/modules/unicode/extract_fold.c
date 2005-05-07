/* Unicode case folding extraction utility
 * by Salvo Isaja
 */

/** \file
 *  \brief Unicode case folding extraction utility.
 *
 *  This program parses the CaseFolding.txt file from the Unicode
 *  Database and generates C code containing lookup tables for
 *  simple case folding.
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>


static unsigned ltrim(const char *s)
{
	unsigned res = 0;
	while (*(s++) == ' ') res++;
	return res;
}


int main()
{
	int page_present[256];
	uint16_t page[256];
	char s[256], *p, *e;
	long code, mapping;
	char status;
	unsigned k;
	unsigned last_page = (unsigned) -1;
	FILE *fin = fopen("CaseFolding.txt", "rt");
	for (k = 0; k < 256; k++) page_present[k] = 0;
	while (fgets(s, sizeof(s), fin))
	{
		p = s + ltrim(s);
		if ((*p == '#') || (*p == '\n')) continue;
		code = strtol(p, &e, 16);
		e++;
		p = e + ltrim(e);
		status = *p;
		if ((status != 'C') && (status != 'S')) continue;
		p += 2;
		p += ltrim(p);
		mapping = strtol(p, &e, 16);
		//printf("%X; %c; %X\n", code, status, mapping);
		unsigned p = code >> 8;
		unsigned i = code & 0xFF;
		if (last_page != p)
		{
			page_present[p] = 1;
			if (last_page != (unsigned) -1)
			{
				unsigned x;
				printf("\nstatic const uint16_t page_%02X[256] = \n{\n\t", last_page);
				for (k = 0, x = 0; k < 256; k++, x++)
				{
					if (x == 8)
					{
						x = 0;
						printf("\n\t");
					}
					printf("0x%04X", page[k]);
					if (k < 255) printf(", ");
				}
				printf("\n};\n");
			}
			for (k = 0; k < 256; k++) page[k] = (p << 8) + k;
		}
		page[i] = mapping;
		last_page = p;
	}
	fclose(fin);
	unsigned x;
	printf("\nstatic const uint16_t *pages[256] = \n{\n\t");
	for (k = 0, x = 0; k < 256; k++, x++)
	{
		if (x == 8)
		{
			x = 0;
			printf("\n\t");
		}
		if (page_present[k])
		{
			printf("page_%02X", k);
			if (k < 255) printf(", ");
		}
		else
		{
			printf("NULL");
			if (k < 255) printf(",    ");
		}
	}
	printf("\n};\n");
	return 0;
}
