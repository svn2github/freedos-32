/*********************************************************************
 * FreeDOS 32 Event Management Code for DJGPP                        *
 * by Salvo Isaja                                                    *
 *                                                                   *
 * Copyright (C) 2003, Salvatore Isaja                               *
 *                                                                   *
 * This program is free software; you can redistribute it and/or     *
 * modify it under the terms of the GNU General Public License       *
 * as published by the Free Software Foundation; either version 2    *
 * of the License, or (at your option) any later version.            *
 *                                                                   *
 * This program is distributed in the hope that it will be useful,   *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 * GNU General Public License for more details.                      *
 *                                                                   *
 * You should have received a copy of the GNU General Public License *
 * along with this program; see the file COPYING.txt;                *
 * if not, write to the Free Software Foundation, Inc.,              *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA           *
 *********************************************************************/
#ifndef __FD32_DJGPP_EVENTS_H
#define __FD32_DJGPP_EVENTS_H


typedef void Fd32EventCallback(void *params);
typedef int  Fd32Event;

#define FD32_EVENT_NULL ((Fd32Event) -1)

Fd32Event fd32_event_post  (unsigned milliseconds, Fd32EventCallback *handler, void *params);
void      fd32_event_delete(Fd32Event event);
void      fd32_event_init  (void);
void      fd32_event_done  (void);


#endif /* #ifndef __FD32_DJGPP_EVENTS_H */

