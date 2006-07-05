/***************************************************************************
 *   Copyright (C) 2005 by Nils Labugt                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

/*  TODO:    1) Improve error handling/messages                        */
/*           2) Make dirsrch.c more general (for use in other apps)    */
/*           3) Add options? (/D and /@)                               */
/*           4) Try to make the executable smaller                     */
          

#include <dr-env.h>
#include <filesys.h>
#include "dirsrch.h"

//#define _FAKE_CON_

#ifdef _FAKE_CON_

#define ERR_PRINT fd32_message
#define LIST_PRINT fd32_message
#define MESSAGE_PRINT fd32_message

#else

#define stderr 2
#define stdout 1
/* COMMAND.EXE implements redirection in *it's* Jft, therefore we have
   to switch back to the previous psp */
void old_psp_puts(int handle, char* s)
{
	struct process_info *ppi = fd32_get_current_pi();
    fd32_set_previous_pi(ppi);
    if(s)
        fd32_write(handle, s, strlen(s));
    fd32_set_current_pi(ppi);
}

#define ERR_PRINT(string) old_psp_puts(stderr, string)
#define LIST_PRINT(string) old_psp_puts(stdout, string)
#define MESSAGE_PRINT(string) old_psp_puts(stdout, string)

#endif

#define STRINGIZE(s) #s

static fd32_fs_lfnfind_t find_data;
static fd32_fs_attr_t m_attr;
static search_data sdat;



static void inval_parm(char* param/* not string! */)
{
    char* p = param;
    while(*p != ' ' && *p != '\0')
        p++;
    *p = '\0';
    ERR_PRINT("Invalid parameter: ");
    ERR_PRINT(param);
    ERR_PRINT("\n");
}

static void print_help()
{
    MESSAGE_PRINT("attrib v0.1\n"
                  "Displays or changes file attributes.\n"
                  "Copyright (c) 2005, licensed under the GPL v2.\n\n"
                  "Syntax: attrib [+r|-r][+s|-s|][+h|-h][+a|-a] [[drive:][path]filename] [/s]\n"
                  "Options:\n"
                  " +/-r  sets/clears the \"read only\" attribute\n"
                  " +/-s  sets/clears the \"system\" attribute\n");
    MESSAGE_PRINT(" +/-h  sets/clears the \"hidden\" attribute\n"
                  " +/-a  sets/clears the \"archive\" attribute\n"
                  "With none of the above options, attributes will be displayed.\n"
                  " /s    process files in subdirectories\n"
                  "Example: attrib -a -h +r mydir\\*.* /s\n");
}

static void syntax_error(char* arg /* not string! */, char* string)
{
    char* p = arg;
    while(*p != ' ' && *p != '\0')
        p++;
    *p = '\0';
    ERR_PRINT("Syntax error: ");
    ERR_PRINT(arg);
    ERR_PRINT(string);
}

static void int_error()
{
    ERR_PRINT("Internal error, aborting...\n");
}


static int attr_param(char* prm, int* flags)
{
    if(!(prm[2] == '\0' || prm[2] == ' '))
    {
        inval_parm(prm);
        return -1;
    }
    switch(prm[1])
    {
    case 'r':
    case 'R':
        *flags |= FD32_ARDONLY;
        break;
    case 's':
    case 'S':
        *flags |= FD32_ASYSTEM;
        break;
    case 'h':
    case 'H':
        *flags |= FD32_AHIDDEN;
        break;
    case 'a':
    case 'A':
        *flags |= FD32_AARCHIV;
        break;
    default:
        inval_parm(prm);
        return -1;
    }
    return 0;
}

static void err_access_denied(char* p)
{
    ERR_PRINT("Access denied: ");
    ERR_PRINT(sdat.path);
    ERR_PRINT("\n");
}

static int process(char* args)
{
    int clear_attr = FD32_ANONE;
    int set_attr = FD32_ANONE;
    char* start_fullpath = NULL;
    char* p;
    char str[16];
    int fhandle, Res, i, n = 0;

    sdat.search_subdir = FALSE;
    /* Parse command line */
    while(*args)
    {
        if(args[0] == '-')
        {
            if(args[1] == '-')
            {
                if(args[2] == 'h' && args[3] == 'e' && args[4] == 'l' && args[5] == 'p')
                {
                    print_help();
                    return 0;
                }
                inval_parm(args);
                return 1;
            }
            if(attr_param(args, &clear_attr) != 0)
                return 1;
        }
        else if(args[0] == '+')
        {
            if(attr_param(args, &set_attr) != 0)
                return 1;
        }
        else if(args[0] == '/')
        {
            if(!(args[2] == '\0' || args[2] == ' '))
            {
                inval_parm(args);
                return 1;
            }
            switch(args[1])
            {
            case 's':
            case 'S':
                sdat.search_subdir = TRUE;
                break;
            case '?':
                print_help();
                return 0;
            default:
                inval_parm(args);
                return 1;
            }
        }
        else
        {
            if(start_fullpath != NULL)
            {
                syntax_error(args, "\n");
                return 1;
            }
            start_fullpath = args;
        }
        while(*args != ' ' && *args != '\0')
            args++;
        while(*args == ' ')
            args++;
    }
    if( (set_attr & clear_attr) != FD32_ANONE)
    {
        syntax_error(NULL, "Same attribute both set and cleared!\n");
        return 1;
    }

    /* Initialize search */
    if(start_fullpath == NULL )
    {
        sdat.f_path = "*.*";
    }
    else
    {
        p = start_fullpath;
        while( *p != ' ' && *p != '\0' )
            p++;
        *p = '\0';
        sdat.f_path = start_fullpath;
    }
    if( (Res = init_dir_search(&sdat)) )
    {
        switch(Res)
        {
        case ERR_TRUENAME:
            syntax_error(start_fullpath, "Invalid path\n");
            return 1;
        case ERR_NONAME:
            syntax_error(start_fullpath, "Path must contain name. Example: dir1\\dir2\\*.*\n");
            return 1;
        case PATH_NOT_FOUND:
            ERR_PRINT("Path not found: ");
            ERR_PRINT(sdat.path);
            ERR_PRINT("\n");
            return 1;
        }
        return 1;
    }
    m_attr.Size = sizeof(fd32_fs_attr_t);
    /* One file is processed for each iteration in this loop */
    while(1)
    {
        Res = nextfind(&sdat, &find_data);
        if(Res > ALL_FINISHED)
        {
            int_error();
            return 1;
        }
        if(Res == ALL_FINISHED)
        {
            if(n == 0)
            {
                MESSAGE_PRINT("File(s) not found:  ");
                sdat.path[sdat.first_dir] = '\0';
                MESSAGE_PRINT(sdat.path);
                MESSAGE_PRINT(sdat.s_fname);
                MESSAGE_PRINT("\n");
            }
            return 0;
        }
        if(Res < 0)
        {
            switch(Res)
            {
            case ERR_DEPTH:
                ERR_PRINT("Error: Directory tree deeper than " STRINGIZE(MAX_DEPTH) " levels\n");
                return 1;
            case ERR_INT:
                int_error();
                err_close_dirs(&sdat);
                return 1;
            }
            return 1;
        }
        n++;
        if( set_attr == FD32_ANONE && clear_attr == FD32_ANONE)
        {
            str[0] = str[1] = str[2] = str[3] = str[4] = str[5] = str[6] = str[7] = ' ';
            if(find_data.Attr & FD32_AARCHIV)
                str[1] = 'a';
            if(find_data.Attr & FD32_ASYSTEM)
                str[2] = 's';
            if(find_data.Attr & FD32_AHIDDEN)
                str[3] = 'h';
            if(find_data.Attr & FD32_ARDONLY)
                str[4] = 'r';
            str[8] = '\0';
            LIST_PRINT(str);
            i = 0;
            for(; find_data.ShortName[i] != '\0'; i++)
                str[i] = find_data.ShortName[i];
            /* Allign vertically */
            for(; i<15; i++)
                str[i] = ' ';
            str[15] = '\0';
            LIST_PRINT(str);
            LIST_PRINT(sdat.path);
            LIST_PRINT("\n");
        }
        else
        {
            fhandle = fd32_open(sdat.path, O_RDONLY, FD32_ANONE, 0, NULL);
            if(fhandle < 0)
            {
                err_access_denied(sdat.path);
                continue;
            }
            Res = fd32_get_attributes(fhandle, &m_attr);
            if(Res)
            {
                fd32_close(fhandle);
                err_access_denied(sdat.path);
                continue;
            }
            m_attr.Attr |= set_attr;
            m_attr.Attr &= ~clear_attr;
            Res = fd32_set_attributes(fhandle, &m_attr);
            if(Res)
            {
                fd32_close(fhandle);
                err_access_denied(sdat.path);
                continue;
            }
            Res = fd32_close(fhandle);
            if(Res)
            {
                int_error();
                return 1;
            }
        }
    }
}

void attrib_init(struct process_info *pi)
{
    char *args;
    int res;
    args = pi-> args;
    /* This is a hack to compensate for a bug in the kernel */
    char c = '\0';
    if((DWORD)args <100)
        args = &c;

#if 0
    while(*args != ' ' && *args != 0)
        args++;
#endif
    while(*args == ' ')
        args++;
    pi->jft_size = MAX_DEPTH + 12;
    pi->jft = fd32_init_jft(MAX_DEPTH + 12);
    res = process(args);
#ifndef _FAKE_CON_
    fd32_fflush(stdout);
    fd32_fflush(stderr);
#endif
    fd32_free_jft(pi->jft, pi->jft_size);
}

