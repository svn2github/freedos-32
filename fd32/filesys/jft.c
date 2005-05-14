/* The FreeDOS-32 JFT Manager version 0.1
 * Copyright (C) 2002-2005  Salvatore ISAJA
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
 * along with this program; see the file COPYING; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <ll/i386/hw-data.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/mem.h>
#include <ll/i386/error.h>
#include <ll/i386/string.h>

#include <kmem.h>
#include <kernel.h>
#include <errno.h>
#include <devices.h>
#include <filesys.h>


/* Define the __DEBUG__ symbol in order to activate log output */
//#define __DEBUG__
#ifdef __DEBUG__
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif


/* Define this to enable console output on file descriptors
 * 1 (stdout) and 2 (stderr) even when they are closed.
 */
#define SAFE_CON_OUT 1


struct JftEntry
{
	fd32_request_t *request; /* free entry if NULL */
	void           *file;
	struct Search  *search; /* NULL for regular entries */
};


/* Defined by the FD32 kernel (PSP) */
void *fd32_get_jft(int *jft_size);


/* Given a handle, checks if it's valid for operations on files (in
 * valid range, open and with no associated search).
 * Returns a pointer to the JFT entry on success, or NULL on failure.
 */
static inline struct JftEntry *validate_file_handle(int fd)
{
	int jft_size;
	struct JftEntry *j = (struct JftEntry *) fd32_get_jft(&jft_size);
	if ((fd < 0) || (fd >= jft_size)) return NULL;
	if (!j[fd].request) return NULL;
	if (j[fd].search) return NULL;
	return &j[fd];
}


int fd32_get_dev_info(int fd)
{
	int res;
	struct JftEntry *j = validate_file_handle(fd);
	if (!j) return -EBADF;
	res = j->request(FD32_GET_DEV_INFO, NULL);
	if (res > 0) return res;
	return 0;
}


/* We want to do some output even if the console driver does not implement it... */
int fake_console_write(void *buffer, int size)
{
	char string[255];
	int  n, count;

	count = size;
	while (count > 0)
	{
		n = 254;
		if (n > count) n = count;
		memcpy(string, buffer, n);
		string[n] = 0;
		cputs(string);
		count -= n;
		buffer += n;
	}
	return size;
}


/* The fake console request function is used for a fake console device
 * open with handles 0, 1, 2 when no console driver is installed.
 */
static fd32_request_t fake_console_request;
static int fake_console_request(DWORD function, void *params)
{
	switch (function)
	{
		case FD32_WRITE:
		{
			fd32_write_t *p = (fd32_write_t *) params;
			if (p->Size < sizeof(fd32_write_t)) return -EINVAL;
			return fake_console_write(p->Buffer, p->BufferBytes);
		}
		case FD32_CLOSE:
		{
			fd32_close_t *p = (fd32_close_t *) params;
			if (p->Size < sizeof(fd32_close_t)) return -EINVAL;
			return 0;
		}
	}
	return -ENOTSUP;
}


/* The dummy request function is used for fake AUX and PRN devices
 * that are to be open with handles 3, 4.
 */
static fd32_request_t dummy_request;
static int dummy_request(DWORD function, void *params)
{
	switch (function)
	{
		case FD32_CLOSE:
		{
			fd32_close_t *p = (fd32_close_t *) params;
			if (p->Size < sizeof(fd32_close_t)) return -EINVAL;
			return 0;
		}
	}
	return -ENOTSUP;
}


#if 0
static fd32_request_t con_filter_request;
static int con_filter_request(DWORD Function, void *Param)
{
  if (Function != FD32_READ) return
  fd32_dev_file_t *console_ops = (fd32_dev_file_t *) P;
  return console_ops->read(NULL, Buffer, 1);
}
int fake_console_read(void *P, void *Buffer, int Size)
{
}
#endif


/* Allocates and initializes a new Job File Table.                  */
/* Handles 0, 1, 2, 3 and 4 are reserved for stdin, stdout, stderr, */
/* stdaux and stdprn special files respectively.                    */
/* Thus, JftSize must be at least 5.                                */
/* Returns a pointer to the new JFT on success, or NULL on failure. */
void *fd32_init_jft(int jft_size)
{
	int hDev;
	struct JftEntry *j;

	LOG_PRINTF(("Initializing JFT...\n"));
	if (jft_size < 5) return NULL;
	
	j = (struct JftEntry *) mem_get(sizeof(struct JftEntry) * jft_size);
	if (!j) return NULL;
	memset(j, 0, sizeof(struct JftEntry) * jft_size);

	/* Prepare JFT entries 0, 1 and 2 with the "con" device */
	hDev = fd32_dev_search("con");
	if (hDev < 0)
	{
		LOG_PRINTF(("\"con\" device not present. Using fake cosole output only.\n"));
		j[0].request = fake_console_request;
	}
	else fd32_dev_get(hDev, &j[0].request, &j[0].file, NULL, 0);
	/* TODO: Re-enable the blocking con read filter */
	#if 0
	else if ((Ops = fd32_dev_query(Dev, FD32_DEV_FILE)))
	{
		if (!(Ops->ioctl(0, FD32_SET_NONBLOCKING_IO, 0) > 0))
		{
			fd32_message("\"con\" device performs blocking input. Installing an input filter.\n");
			FakeConOps      = *Ops;
			FakeConOps.P    = Ops;
			FakeConOps.read = fake_console_read;
			Ops             = &FakeConOps;
		}
	}
	else
	{
		fd32_message("\"con\" is not a character device. Using fake cosole output only.\n");
		request = fake_console_request;
	}
	#endif
	memcpy(&j[1], &j[0], sizeof(struct JftEntry));
	memcpy(&j[2], &j[0], sizeof(struct JftEntry));

	/* Prepare JFT entry 3 with the "aux" device */
	hDev = fd32_dev_search("aux");
	if (hDev < 0)
	{
		LOG_PRINTF(("\"aux\" device not present. Initializing handle 3 with dummy ops.\n"));
		j[3].request = dummy_request;
	}
	else fd32_dev_get(hDev, &j[3].request, &j[3].file, NULL, 0);

	/* Prepare JFT entry 4 with the "prn" device */
	hDev = fd32_dev_search("prn");
	if (hDev < 0)
	{
		LOG_PRINTF(("\"prn\" device not present. Initializing handle 4 with dummy ops.\n"));
		j[4].request = dummy_request;
	}
	else fd32_dev_get(hDev, &j[4].request, &j[4].file, NULL, 0);

	LOG_PRINTF(("JFT initialized.\n"));
	return j;
}


void fd32_free_jft(void *p, int jft_size)
{
	int res = mem_free((DWORD) p, sizeof(struct JftEntry) * jft_size);
	if (res != 1)
	{
		error("Restore PSP panicing while freeing the JFT\n");
		fd32_abort();
	}
}


/* The LSEEK system call.                                     */
/* Calls the "lseek" function of the device hosting the file. */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_lseek(int fd, long long int offset, int whence, long long int *result)
{
	off_t res;
	struct JftEntry *j = validate_file_handle(fd);
	if (!j) return -EBADF;
	res = fs_lseek(j->request, j->file, offset, whence);
	if (res < 0) return (int) res;
	if (result) *result = res;
	return 0;
}


/* The READ system call.                                      */
/* Calls the "read" function of the device hosting the file.  */
/* On success, returns the not negative number of bytes read. */
/* On failure, returns a negative error code.                 */
int fd32_read(int fd, void *buffer, int size)
{
	struct JftEntry *j = validate_file_handle(fd);
	if (!j) return -EBADF;
	return fs_read(j->request, j->file, buffer, size);
}


/* The WRITE system call.                                        */
/* Calls the "write" function of the device hosting the file.    */
/* On success, returns the not negative number of bytes written. */
/* On failure, returns a negative error code.                    */
int fd32_write(int fd, void *buffer, int size)
{
	struct JftEntry *j = validate_file_handle(fd);
	if (!j)
	{
		#if SAFE_CON_OUT
		if ((fd == 1) || (fd == 2)) return fake_console_write(buffer, size);
		else
		#endif
		return -EBADF;
	}
	return fs_write(j->request, j->file, buffer, size);
}


/* The CHMOD or SET ATTRIBUTES system call.                   */
/* Sets attributes and time stamps of an open file.           */
/* Returns 0 on success, or a negative error code on failure. */
/* Note: the Size field of the attributes structure must be   */
/* set before calling this function.                          */
int fd32_set_attributes(int fd, fd32_fs_attr_t *p)
{
	struct JftEntry *j = validate_file_handle(fd);
	if (!j) return -EBADF;
	return fs_set_attributes(j->request, j->file, p);
}


/* The GET ATTRIBUTES system call.                                     */
/* Gets attributes and time stamps of an open file.                    */
/* On success, returns 0 and fills the passed attributes structure.    */
/* On failure, returns a negative error code.                          */
/* Note: the Size field of the attributes structure must be set before */
/* calling this function.                                              */
int fd32_get_attributes(int fd, fd32_fs_attr_t *p)
{
	struct JftEntry *j = validate_file_handle(fd);
	if (!j) return -EBADF;
	return fs_get_attributes(j->request, j->file, p);
}


/* The FFLUSH system call.                                     */
/* Calls the "fflush" function of the device hosting the file. */
/* Returns 0 on success, or a negative error code on failure.  */
int fd32_fflush(int fd)
{
	struct JftEntry *j = validate_file_handle(fd);
	if (!j) return -EBADF;
	return fs_fflush(j->request, j->file);
}


/* The CLOSE system call.                                     */
/* Calls the "close" function of the device hosting the file, */
/* and marks the JFT entry as free.                           */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_close(int fd)
{
	int res;
	struct JftEntry *j = validate_file_handle(fd);
	if (!j) return -EBADF;
	res = fs_close(j->request, j->file);
	if (res >= 0) j->request = NULL;
	return res;
}


/* The DUP system call.                                                    */
/* Allocates a JFT entry referring to the same file identified by the      */
/* source handle.                                                          */
/* Returns the new handle on success, or a negative error code on failure. */
int fd32_dup(int oldfd)
{
	fd32_open_t p;
	int h;
	int jft_size;
	struct JftEntry *j = (struct JftEntry *) fd32_get_jft(&jft_size);

	if ((oldfd < 0) || (oldfd >= jft_size)) return -EBADF;
	if (!j[oldfd].request) return -EBADF;

	for (h = 0; h < jft_size; h++)
		if (!j[h].request)
		{
			memcpy(&j[h], &j[oldfd], sizeof(struct JftEntry));
			/* Increment the references counter for the file */
			p.Size     = sizeof(fd32_open_t);
			p.DeviceId = j[oldfd].file;
			j[oldfd].request(FD32_OPEN, &p);
			return h;
		}
	return -EMFILE;
}


/* The DUP2 or FORCEDUP system call.                                    */
/* Causes a specified JFT entry to refer to the same file identified by */
/* the source handle. If the new handle is an open file, it is closed.  */
/* Returns 0, or a negative error code on failure.                      */
int fd32_forcedup(int oldfd, int newfd)
{
	fd32_open_t p;
	int res;
	int jft_size;
	struct JftEntry *j = (struct JftEntry *) fd32_get_jft(&jft_size);

	if ((oldfd < 0) || (oldfd >= jft_size)) return -EBADF;
	if (!j[oldfd].request) return -EBADF;
	if (newfd == oldfd) return 0;
	if ((newfd < 0) || (newfd >= jft_size)) return -EBADF;

	/* Close the file associated to "newfd" if currently open */
	if (j[newfd].request)
		if ((res = fd32_close(newfd)) < 0) return res;
	/* Assign the JFT entry to the source SFT entry */
	memcpy(&j[newfd], &j[oldfd], sizeof(struct JftEntry));
	/* Increment the references counter for the file */
	p.Size     = sizeof(fd32_open_t);
	p.DeviceId = j[oldfd].file;
	j[oldfd].request(FD32_OPEN, &p);
	return 0;
}


/* The OPEN system call.                                                 */
/* Calls the "open" function of the file system driver related to the    */
/* specified file name, allocates a JFT entry and returns a not negative */
/* file handle (the JFT index) to identify the open file.                */
/* On failure, returns a negative error code.                            */
int fd32_open(char *file_name, DWORD mode, WORD attr, WORD alias_hint, int *result)
{
	int jft_size;
	int fd;
	struct JftEntry *j = fd32_get_jft(&jft_size);
	for (fd = 0; fd < jft_size; fd++)
		if (!j[fd].request)
		{
			fd32_request_t *request;
			void *file;
			int res = fs_open(file_name, mode, attr, alias_hint, &request, &file);
			if (res < 0) return res;
			if (result) *result = res;
			j[fd].request = request;
			j[fd].file    = file;
			j[fd].search  = NULL;
			return fd;
		}
	return -EMFILE;
}


/******************************************************************************
 *
 * Short File Name search facilities
 *
 ******************************************************************************/


/* Splits a full valid path name (path + file name) into */
/* its path and file name components.                    */
/* Called by fd32_dos_findfirst, fd32_lfn_findfirst.     */
static void split_path(const char *full_path, char *path, char *name)
{
	const char *name_start;
	const char *s;

	/* Move to the end of the "full_path" string */
	for (s = full_path; *s != 0; s++);
	/* Now move backward until '\' is found. The name starts after the '\' */
	for (; (*s != '\\') && (s != full_path); s--);
	if (*s == '\\') s++;
	/* And copy the string from this point until the end into "name" */
	for (name_start = s; *s != 0; s++, name++) *name = *s;
	*name = 0;
	/* Finally we copy from the beginning to the filename
	 * into "path", with the trailing slash. */
	for (s = full_path; s < name_start; s++, path++) *path = *s;
	*path = 0;
}


/* The DOS style FINDFIRST system call.                                    */
/* Finds the first file or device that matches with the specified file     */
/* name. No handles are kept open by this call, and the search status for  */
/* subsequent FINXNEXT calls is stored in a user buffer, the Disk Transfer */
/* Area (DTA) in the real-mode DOS world. Thus, invoking another system    */
/* call that writes into the DTA overwrites the search status with         */
/* unpredictable results.                                                  */
/* Returns 0 on success, or a negative error code on failure.              */
int fd32_dos_findfirst(char *file_spec, BYTE attrib, fd32_fs_dosfind_t *find_data)
{
	struct fd32_findfirst p;
	fd32_request_t *request;
	void *volume;
	char  aux[FD32_LFNPMAX];
	char *path;
	int   res;

	/* TODO: About character devices, the RBIL says:
	this call also returns successfully if given the name of a
	character device without wildcards.  DOS 2.x returns attribute
	00h, size 0, and the current date and time.  DOS 3.0+ returns
	attribute 40h and the current date and time. */
	res = fd32_truename(aux, file_spec, FD32_TNSUBST);
	if (res < 0) return res;
	for (;;)
	{
		res = fd32_get_drive(aux, &request, &volume, &path);
		if (res < 0) return res;
		/* DOS find functions work only on drives identified by a drive letter */
		if (res == 0) return -ENODEV;
		find_data->SearchDrive = res - 'A' + 1;
		/* Open the directory and search for the specified template */
		p.volume    = volume;
		p.path      = path;
		p.attrib    = attrib;
		p.find_data = find_data;
		res = request(FD32_FINDFIRST, &p);
		if (res != -ENOTMOUNT) return res;
	}
}


/* The DOS style FINDNEXT system call.                                    */
/* Finds the next file (or device?) that matches with file name specified */
/* by a previous DOS style FINDFIRST system call.                         */
int fd32_dos_findnext(fd32_fs_dosfind_t *find_data)
{
	struct fd32_findnext p;
	fd32_request_t *request;
	void *volume;
	char  drive[2] = " :";
	int   res;

	for (;;)
	{
		drive[0] = find_data->SearchDrive + 'A' - 1;
		res = fd32_get_drive(drive, &request, &volume, NULL);
		if (res < 0) return res;
		p.volume    = volume;
		p.find_data = find_data;
		res = request(FD32_FINDNEXT, &p);
		if (res != -ENOTMOUNT) return res;
	}
}


/******************************************************************************
 *
 * Long File Name search facilities and search handles
 *
 ******************************************************************************/


struct Search
{
	DWORD flags;
	char name[FD32_LFNMAX];
	struct Search *next;
};


/* Organize Search structs in a linked list of page-sized tables */
#ifndef PAGE_SIZE
 #define PAGE_SIZE 4096
#endif
#define SEARCHES_PER_TABLE ((PAGE_SIZE - sizeof(struct SearchTable *)) / sizeof(struct Search))
struct SearchTable
{
	struct Search s[SEARCHES_PER_TABLE];
	struct SearchTable *next;
};
struct SearchTable *search_table = NULL;
struct Search *search_used = NULL;
struct Search *search_free = NULL;


static struct Search *search_get(void)
{
	struct Search *p = search_free;
	if (!p)
	{
		unsigned k;
		struct SearchTable *table = (struct SearchTable *) mem_get(PAGE_SIZE);
		if (!table) return NULL;
		memset(table, 0, PAGE_SIZE);
		for (k = 0; k < SEARCHES_PER_TABLE - 1; k++)
			table->s[k].next = &table->s[k + 1];
		search_free = &table->s[0];
		table->next = search_table;
		search_table = table;
		p = search_free;
	}
	search_free = p->next;
	p->next = search_used;
	search_used = p;
	return p;
}


static void search_put(struct Search *p)
{
	//assert(p->refs);
	//if (--p->refs == 0)
	//{
		search_used = p->next;
		p->next = search_free;
		search_free = p;
	//}
}


/* Given a handle, checks if it's valid for directory searches (in    */
/* valid range, open and with a search name defined.                  */
/* Returns a pointer to the JFT entry on success, or NULL on failure. */
static inline struct JftEntry *validate_search_handle(int handle)
{
	int jft_size;
	struct JftEntry *j = (struct JftEntry *) fd32_get_jft(&jft_size);
	if ((handle < 0) || (handle >= jft_size)) return NULL;
	if (!j[handle].request) return NULL;
	if (!j[handle].search) return NULL;
	return &j[handle];
}


/* The Long File Name style FINDFIRST system call.                         */
/* Finds the first file (or device?) that matches with the specified name, */
/* returning a file handle to continue the search.                         */
/* On failure, returns a negative error code.                              */
int fd32_lfn_findfirst(/*const*/ char *file_spec, DWORD flags, fd32_fs_lfnfind_t *find_data)
{
	int fd;
	int jft_size;
	struct JftEntry *j = (struct JftEntry *) fd32_get_jft(&jft_size);
	for (fd = 0; fd < jft_size; fd++)
		if (!j[fd].request)
		{
			char path[FD32_LFNPMAX];
			char name[FD32_LFNPMAX];
			struct fd32_findfile p;
			fd32_request_t *request;
			void *file;
			int res;
			struct Search *search = search_get();
			if (!search) return -ENOMEM;
			split_path(file_spec, path, name);
			res = fs_open(path, FD32_OREAD | FD32_OEXIST | FD32_ODIR, FD32_ANONE, 0, &request, &file);
			if (res < 0)
			{
				search_put(search);
				return res;
			}
			p.dir       = file;
			p.name      = name;
			p.find_data = (void *) find_data;
			LOG_PRINTF(("fd32_lfn_findfirst: Searching for \"%s\"\n", p.name));
			res = request(FD32_FINDFILE, &p);
			if (res < 0)
			{
				search_put(search);
				fs_close(request, file);
				return res;
			}
			search->flags = flags;
			strcpy(search->name, name);
			j[fd].request = request;
			j[fd].file    = file;
			j[fd].search  = search;
			return fd;
		}
	return -EMFILE;
}


/* The Long File Name style FINDNEXT system call.                        */
/* Finds the next file (or device?) that matches with the name specified */
/* by a previous LFN style FINDFIRST system call.                        */
/* Returns 0 on success, or a negative error code on failure.            */
int fd32_lfn_findnext(int fd, DWORD flags, fd32_fs_lfnfind_t *find_data)
{
	int res = -EBADF;
	struct JftEntry *j = validate_search_handle(fd);
	if (j)
	{
		struct fd32_findfile p;
		p.dir       = j->file;
		p.name      = j->search->name;
		p.find_data = (void *) find_data;
		res = j->request(FD32_FINDFILE, &p);
	}
	return res;
}


/* The Long File Name style FINDCLOSE system call.                     */
/* Terminates a search started by the LFN style FINDFIRST system call, */
/* closing the file handle used for the search.                        */
/* Returns 0 on success, or a negative error code on failure.          */
int fd32_lfn_findclose(int fd)
{
	int res = -EBADF;
	struct JftEntry *j = validate_search_handle(fd);
	if (j)
	{
		res = fs_close(j->request, j->file);
		if (res >= 0)
		{
			search_put(j->search);
			j->request = NULL;
			j->search = NULL;
		}
	}
	return res;
}
