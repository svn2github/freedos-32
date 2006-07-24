/* The FreeDOS-32 Unicode Support Library version 2.1
 * Copyright (C) 2001-2006  Salvatore ISAJA
 *
 * This file "init.c" is part of the FreeDOS-32 Unicode
 * Support Library (the Program).
 *
 * The Program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with the Program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "unicode.h"
#include <kernel.h>
#include <ll/i386/error.h>

static struct { char *name; void *address; } symbols[] =
{
	{ "unicode_utf8_len",         unicode_utf8_len         },
	{ "unicode_utf8_to_wchar",    unicode_utf8_to_wchar    },
	{ "unicode_wchar_to_utf8",    unicode_wchar_to_utf8    },
	{ "unicode_utf16le_len",      unicode_utf16le_len      },
	{ "unicode_utf16le_to_wchar", unicode_utf16le_to_wchar },
	{ "unicode_wchar_to_utf16le", unicode_wchar_to_utf16le },
	{ "unicode_utf16be_len",      unicode_utf16be_len      },
	{ "unicode_utf16be_to_wchar", unicode_utf16be_to_wchar },
	{ "unicode_wchar_to_utf16be", unicode_wchar_to_utf16be },
	{ "unicode_simple_fold",      unicode_simple_fold      },
	{ 0, 0 }
};


void unicode_init(void)
{
	int k;
	message("Going to install the Unicode support library... ");
	for (k = 0; symbols[k].name; k++)
		if (fd32_add_call(symbols[k].name, symbols[k].address, ADD) == -1)
			message("Cannot add %s to the symbol table\n", symbols[k].name);
	message("Done\n");
}
