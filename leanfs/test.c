/*  The LEAN File System Driver version 0.1
 *  Copyright (C) 2004  Salvatore ISAJA
 *
 *  LEAN (Lean yet Effective Allocation and Naming) is a free, simple,
 *  portable, personal, full featured file system for embedded tasks
 *  designed by Salvatore Isaja, and somewhat inspired from the EXT2 fs.
 *  This driver provides read and write access to media formatted with LEAN.
 *
 *  This file is part of the LEAN File System Driver (the Program).
 *
 *  The Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  The Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with the Program; see the file COPYING; if not, write to
 *  the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* This file is "test.c".
 * Some routines to test the functionality of the driver.
 */

#include "leanfs.h"
//#define NDEBUG
#include <assert.h>
#include <error.h>
#include <stdlib.h>


static int readdir_callback(void *dirent, const char *d_name, int d_namlen, off_t f_pos, ino_t d_fileno, unsigned d_type)
{
	unsigned k;
	printf("--readdir_callback--\n");
	printf("d_name  : %s\n", d_name);
	printf("d_namlen: %i\n", d_namlen);
	printf("f_pos   : %u\n", f_pos);
	printf("d_fileno: %u\n", d_fileno);
	printf("d_type  : %u\n", d_type);
	return 0;
}


static void readdir_test(struct File *d)
{
	struct DirEntry de;
	int res;
	printf("readdir_test\n");
	leanfs_lseek(d, 0, SEEK_SET);
	while ((res = leanfs_readdir(d, &de, readdir_callback)) == 0);
	error(0, -res, "Exiting readdir loop with %i", res);
}


static void open_test(struct File *parent, struct File **file, const char *name)
{
	printf("open_test: %s\n", name);
	int res = leanfs_open(parent, file, name, O_RDWR);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		abort();
	}
}


static void create_test(struct File *parent, struct File **file, const char *name)
{
	printf("create_test: %s\n", name);
	int res = leanfs_open(parent, file, name, O_RDWR | O_CREAT, LEANFS_IRUSR | LEANFS_IWUSR | LEANFS_IFREG);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		abort();
	}
}


static void close_test(struct File *f)
{
	printf("close_test\n");
	int res = leanfs_close(f);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		abort();
	}
}


static void read_test(struct File *f, unsigned count)
{
	unsigned k;
	printf("read_test\n");
	for (k = 0; k < count; k++)
	{
		unsigned x;
		int res = leanfs_read(f, &x, sizeof(k));
		if (res < 0)
		{
			error(0, -res, "Frame %u, res=%i", k, res);
			printf("Closing the file after write error\n");
			res = leanfs_close(f);
			if (res < 0) error(0, -res, "%i", res);
			abort();
		}
		if (res == 0) break;
		if (x != k) printf("x=%u != k=%u\n", x, k);
	}
	printf("%u elements read\n", k);
}


static void write_test(struct File *f, unsigned count)
{
	unsigned k;
	printf("write_test\n");
	for (k = 0; k < count; k++)
	{
		int res = leanfs_write(f, &k, sizeof(k));
		if ((res < 0) || (res != sizeof(k)))
		{
			error(0, -res, "Frame %u, res=%i", k, res);
			printf("Closing the file after write error\n");
			res = leanfs_close(f);
			if (res < 0) error(0, -res, "%i", res);
			abort();
		}
	}
}


static void ftruncate_test(struct File *f, off_t length)
{
	printf("ftruncate_test");
	int res = leanfs_ftruncate(f, length);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		abort();
	}
}


static void unlink_test(struct File *d, const char *name)
{
	printf("unlink_test: %s\n", name);
	int res = leanfs_unlink(d, name);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		abort();
	}
}


static void mkdir_test(struct File *parent, const char *name)
{
	printf("mkdir_test: %s\n", name);
	int res = leanfs_mkdir(parent, name, LEANFS_IRUSR | LEANFS_IWUSR | LEANFS_IFDIR);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		abort();
	}
}


int main()
{
	struct Volume *v;
	struct File   *d, *f;
	int res;

	printf("Mounting volume...\n");
	res = leanfs_mount("leantest.img", &v);
//	res = leanfs_mount("/dev/fd0", &v);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		return res;
	}
	printf("Opening root directory...\n");
	//res = fs_open(v, "/testdir/dir1", O_RDWR | O_DIRECTORY, &d);
	res = fs_open(v, "/testdir/dir1/test.txt", O_RDONLY, &d);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		return res;
	}
	/* PUT TESTS HERE */
	read_test(d, 120000);
	//create_test(d, &f, "test.txt");
	//write_test(f, 100000);
	//close_test(f);
	//mkdir_test(d, "dir1");
	//readdir_test(d);
	/* PUT TESTS HERE */
	printf("Closing the root directory\n");
	res = leanfs_close(d);
	if (res < 0)
	{
		error(0, -res, "%i", res);
		return res;
	}
	return 0;
}
