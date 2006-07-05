/* FD/32 utility functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <ll/i386/hw-data.h>

/* Delay a specified nanoseconds
 * NOTE: This needs to be rewritten. Move into the proper place of kernel?
 */
#define CONFIG_PIT_MODULE /* disable the old delay */
#ifndef CONFIG_PIT_MODULE
#include <ll/sys/ll/event.h>
#include <ll/sys/ll/time.h>
static void imp_delay(unsigned int ns)
{
    TIME start, current;
    TIME us = ns / 1000;
    us +=4;
    current = start = ll_gettime(TIME_NEW, NULL);
    while(current - start < us)
        current = ll_gettime(TIME_NEW, NULL);
}
#endif
