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


#include <dr-env.h>
#include <filesys.h>
#include "dirsrch.h"


/* Try to close all open file handles if an error occures */
void err_close_dirs(search_data* sd)
{
    while(sd->current_dir >= 0)
    {
        fd32_lfn_findclose(sd->dir_s_handle[sd->current_dir]);
        sd->current_dir--;
    }
}

/* Initialize search with this function */
int init_dir_search( search_data* sd)
{
    char* p;
    int n = 0, handle;
    int Res;
    sd->err_no = 0;
    /* Cannonicalize path */
    if( (Res = fd32_truename(sd->path, sd->f_path, FD32_TNDOTS /* | FD32_TNSUBST */)) )
    {
        sd->err_no = Res;
        return ERR_TRUENAME;
    }
    /* Split name and path */
    sd->s_fname = sd->f_path;
    while(*(sd->s_fname))
        sd->s_fname++;
    p = sd->path;
    while(*p)
    {
        if(*p == '\\')
            n++;
        p++;
        sd->first_dir++;
    }
    if(n == 0)
        return ERR_TRUENAME;
    if(*(p-1) == '\\')
    {
        return ERR_NONAME;
    }
    while(*(p-1) != '\\')
    {
        p--;
        sd->first_dir--;
        sd->s_fname--;
    }
    *p = '\0';
    /* Try to open directory to check that path exist */
    handle = fd32_open(sd->path, O_RDONLY | O_DIRECTORY, FD32_ANONE, 0, NULL);
    if(handle < 0)
    {
        sd->err_no = handle;
        return PATH_NOT_FOUND;
    }
    if((n = fd32_close(handle)) < 0)
        return n;
    sd->end_path = sd->first_dir;
    sd->current_dir = -1;
    sd->file_s_handle = -1;
    return 0;
}


static int down_tree( search_data* sd, fd32_fs_lfnfind_t* fd)
{
    int Res;
    /* Search for directory */
    sd->current_dir++;
    if(sd->current_dir > MAX_DEPTH - 1)
        return ERR_DEPTH;
    strcpy(sd->path + sd->end_path, "*.*");
    Res = fd32_lfn_findfirst(sd->path, DIR_FIND_FLAGS, fd);
    if(Res < 0)
    {
        sd->current_dir--;
        return NO_SUBDIRS;
    }
    sd->dir_s_handle[sd->current_dir] = Res;
    /* Add directory name to end of path */
    strcpy(sd->path + sd->end_path, fd->LongName);
    while(sd->path[sd->end_path])
        sd->end_path++;
    sd->path[sd->end_path++] = '\\';
    sd->path[sd->end_path] = '\0';
    return 0;
}

static int first_file( search_data* sd, fd32_fs_lfnfind_t* fd)
{
    int Res;
    strcpy(sd->path + sd->end_path, sd->s_fname);
    Res = fd32_lfn_findfirst(sd->path, FILE_FIND_FLAGS, fd);
    if(Res < 0)
    {
        if(sd->first_dir < sd->end_path)
            return DIR_FINISHED;
        return ALL_FINISHED;
    }
    sd->file_s_handle = Res;
    strcpy(sd->path + sd->end_path, fd->LongName);
    return 0;
}


static int next_file(search_data* sd, fd32_fs_lfnfind_t* fd)
{
    int Res;
    if(sd->file_s_handle < 0)
        return FIRST_ROUND;
    Res = fd32_lfn_findnext(sd->file_s_handle, FILE_FIND_FLAGS, fd);
    if(Res == 0)
    {
        strcpy(sd->path + sd->end_path, fd->LongName);
        return 0;
    }
    Res = fd32_lfn_findclose(sd->file_s_handle);
    if(Res)
    {
        sd->err_no = Res;
        return Res;
    }
    if(sd->first_dir < sd->end_path)
    {
        return DIR_FINISHED;
    }
    return ALL_FINISHED;
}


static int next_dir(search_data* sd, fd32_fs_lfnfind_t* fd)
{
    int Res;
    if(sd->current_dir < 0)
        return FIRST_ROUND;
    /* Remove previous directory from end of path */
    sd->end_path--;
    while(sd->path[sd->end_path - 1] != '\\')
    {
        sd->end_path--;
        if(sd->end_path < sd->first_dir)
        {
            err_close_dirs(sd);
            return ERR_INT;
        }
    }
    Res = fd32_lfn_findnext(sd->dir_s_handle[sd->current_dir], DIR_FIND_FLAGS, fd);
    if(Res != 0)
    {
        Res = fd32_lfn_findclose(sd->dir_s_handle[sd->current_dir]);
        if(Res)
        {
            sd->err_no = Res;
            return Res;
        }
        sd->current_dir--;
        return NO_SUBDIRS;
    }
    /* Add directory name to end of path */
    strcpy(sd->path + sd->end_path, fd->LongName);
    while(sd->path[(sd->end_path)])
        sd->end_path++;
    sd->path[sd->end_path++] = '\\';
    sd->path[(sd->end_path)] = '\0';
    return 0;
}


/* The "search_data" structure should (generally) not be altered */
/* between successive calls to this function */
int nextfind(search_data* sd, fd32_fs_lfnfind_t* fd)
{
    int Res;
    Res = next_file(sd, fd);
    if(Res == 0)
        return 0;
    if(Res == ALL_FINISHED)
        return Res;
    /* If we arrive here, then either this is the first time this
       function is called, or we have found the last file in the
       subdirectory */
    while(1)
    {
        if(sd->search_subdir == TRUE)
        {
            Res = next_dir(sd, fd);
            if(Res < 0)
                return Res;
            if(Res == 0 || Res == FIRST_ROUND)
            {
                /* OK, there were another subdirectory. We procede down the
                   directory tree as far as we get */
                while(1)
                {
                    Res = down_tree(sd, fd);
                    if(Res < 0)
                        return Res;
                    if(Res > 0)
                        break;
                    /* If the directory name starts with a dot, then it
                       is not a subdirectory */
                    while(*(fd->LongName) == '.')
                    {
                        Res = next_dir(sd, fd);
                        if(Res < 0)
                            return Res;
                        if(Res > 0)
                            break;
                    }
                    if(Res > 0)
                        break;
                }
            }
        }
        Res = first_file(sd, fd);
        if(Res == 0)
            return 0;
        if(Res == ALL_FINISHED)
            return Res;
    }
}

