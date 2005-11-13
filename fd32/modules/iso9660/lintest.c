/* Test program for the FreeDOS-32 ISO 9660 file system driver
 * when compiled for the GNU/Linux target (test environment).
 * by Salvo Isaja
 */
#include "iso9660.h"
#include <error.h>
#include <filesys.h>

ssize_t iso9660_blockdev_read(Volume *v, void *data, size_t count, Sector from)
{
	fseek(v->blockdev, from * v->bytes_per_sector, SEEK_SET);
	return fread(data, v->bytes_per_sector, count, v->blockdev);
}

int main()
{
	Volume *v;
	File *f;
	Buffer *b = NULL;
	uint8_t buf[65536];
	int res = iso9660_mount("/dev/cdrom", &v);
	if (res < 0) goto theend;

	#if 0
	/* Directory listing */
	res = iso9660_open_pr(&v->root_dentry, "\\FOOBAR", strlen("\\FOOBAR"), O_RDONLY, 0, &f);
	if (res < 0) goto theend;
	for (;;)
	{
		res = iso9660_do_readdir(f, &b, NULL, NULL);
		if (res < 0) break;
	}
	iso9660_close(f);
	#elif 0
	/* File copy test */
	FILE *fout = fopen("foo.bar", "wb");
	res = iso9660_open_pr(&v->root_dentry, "\\dir1\\foo.bar", strlen("\\dir1\\foo.bar"), O_RDONLY, 0, &f);
	if (res < 0) goto theend;
	for (;;)
	{
		printf("%li\n", f->file_pointer);
		res = iso9660_read(f, buf, sizeof(buf));
		if (res < 0) goto theend;
		if (res == 0) break;
		fwrite(buf, res, 1, fout);
	}
	fclose(fout);
	iso9660_close(f);
	#elif 1
	/* Findfirst/Findnext test */
	fd32_fs_dosfind_t df;
	res = iso9660_findfirst_pr(&v->root_dentry, "\\dir1\\*.*", strlen("\\dir1\\*.*"), 0xFF, &df);
	while (res >= 0)
	{
		printf("%02x %9u \"%s\"\n", df.Attr, df.Size, df.Name);
		res = iso9660_findnext(v, &df);
	}
	#endif
	iso9660_unmount(v);
theend:
	error(0, -res, "res=%i", res);
	return 0;
}
