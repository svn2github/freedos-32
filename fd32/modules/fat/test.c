#include "fat.h"
void nls_init(void);


#define BYTES_PER_SECTOR 512

ssize_t fat_blockdev_read(BlockDev *bd, void *data, size_t count, Sector from)
{
	fseek(*bd, from * BYTES_PER_SECTOR, SEEK_SET);
	return fread(data, BYTES_PER_SECTOR, count, *bd);
}


ssize_t fat_blockdev_write(BlockDev *bd, const void *data, size_t count, Sector from)
{
	fseek(*bd, from * BYTES_PER_SECTOR, SEEK_SET);
	return fwrite(data, BYTES_PER_SECTOR, count, *bd);
}


int fat_blockdev_mediachange(BlockDev *bd)
{
	return -ENOTSUP; /* not removable */
}


static void unlink_test(Volume *v, const char *pn)
{
	int res = fat_unlink_pr(&v->root_dentry, pn, strlen(pn));
	printf("Unlink: %i (%s)\n", res, strerror(-res));
}


static void mkdir_test(Volume *v, const char *pn)
{
	int res = fat_mkdir_pr(&v->root_dentry, pn, strlen(pn), 0);
	printf("Mkdir: %i (%s)\n", res, strerror(-res));
}


static void rmdir_test(Volume *v, const char *pn)
{
	int res = fat_rmdir_pr(&v->root_dentry, pn, strlen(pn));
	printf("Rmdir: %i (%s)\n", res, strerror(-res));
}


static void rename_test(Volume *v, const char *on, const char *nn)
{
	int res = fat_rename_pr(&v->root_dentry, on, strlen(on), &v->root_dentry, nn, strlen(nn));
	printf("Rename: %i (%s)\n", res, strerror(-res));
}


static void read_test(Volume *v, const char *pn)
{
	Channel *c;
	char buf[2048];
	int k, res;
	res = fat_open_pr(&v->root_dentry, pn, strlen(pn), O_RDONLY, 0, &c);
	printf("Open: %i (%s)\n", res, strerror(-res));
	res = fat_read(c, buf, sizeof(buf));
	printf("Read: %i (%s)\n", res, strerror(-res));
	for (k = 0; k < res; k++) fputc(buf[k], stdout);
	res = fat_close(c);
	printf("Close: %i (%s)\n", res, strerror(-res));
}


static void lfnfind_test(Volume *v, const char *parent, const char *fn)
{
	fd32_fs_lfnfind_t lfnfind;
	Channel *c;
	int res = fat_open_pr(&v->root_dentry, parent, strlen(parent), O_RDONLY | O_DIRECTORY, 0, &c);
	printf("Open: %i (%s)\n", res, strerror(-res));
	for (;;)
	{
		res = fat_findfile(c, fn, strlen(fn), (FAT_ANONE << 16) | FAT_ANOVOLID, &lfnfind);
		printf("findfile: '%s' \"%s\" %i (%s)\n", lfnfind.ShortName, lfnfind.LongName, res, strerror(-res));
		if (res < 0) break;
	}
	res = fat_close(c);
	printf("Close: %i (%s)\n", res, strerror(-res));
}


static void dosfind_test(Volume *v, const char *pn)
{
	fd32_fs_dosfind_t df;
	int res = fat_findfirst_pr(&v->root_dentry, pn, strlen(pn), FAT_ANOVOLID, &df);
	if (res >= 0) printf("'%s'\n", df.Name);
	do
	{
		res = fat_findnext(v, &df);
		if (res >= 0) printf("'%s'\n", df.Name);
	}
	while (res >= 0);
}


static void create_test(Volume *v, const char *pn)
{
	Channel *c;
	char buf[2048];
	int res;
	res = fat_open_pr(&v->root_dentry, pn, strlen(pn), O_RDWR | O_CREAT | O_EXCL, 0, &c);
	printf("Open: %i (%s)\n", res, strerror(-res));
	for (res = 0; res < sizeof(buf); res++) buf[res] = (res % 10) + '0';
	res = fat_write(c, buf, sizeof(buf));
	printf("Write: %i (%s)\n", res, strerror(-res));
	res = fat_close(c);
	printf("Close: %i (%s)\n", res, strerror(-res));
}


int main()
{
	int res;
	Volume *v;
	nls_init();
	res = fat_mount("../img/floppy.img", &v);
	printf("Mount: %i (%s)\n", res, strerror(-res));
	//for (res = 0; res < 100000; res++)
	//unlink_test(v, "\\grub\\menu.lst");
	//rmdir_test(v, "\\faccio un provolone");
	rename_test(v, "\\grub\\rentest", "\\puppo");
		//read_test(v, "\\grub\\menu.lst");
		//lfnfind_test(v, "\\grub", "*.*");
		//create_test(v, "\\faccio una prova.txt");
		//mkdir_test(v, "\\faccio un provolone");
		//dosfind_test(v, "\\autoexec.bat\\*.*");
	res = fat_unmount(v);
	printf("Unmount: %i (%s)\n", res, strerror(-res));
	#if 0
	//res = fat_open(v, "\\autoexec.bat", NULL, O_RDWR, 0, &c);
	res = fat_open(v, "\\faccio una prova.txt", NULL, O_RDWR | O_CREAT | O_EXCL, 0, &c);
	printf("Open: %i (%s)\n", res, strerror(-res));
	#if 1 /* write */
	res = fat_lseek(c, 0, SEEK_SET);
	printf("Seek: %i (%s)\n", res, strerror(-res));
	for (k = 0; k < sizeof(buf); k++) buf[k] = (k % 10) + '0';
	res = fat_write(c, buf, sizeof(buf));
	printf("Write: %i (%s)\n", res, strerror(-res));
	#elif 0 /* read */
	res = fat_read(c, buf, sizeof(buf));
	printf("Read: %i (%s)\n", res, strerror(-res));
	for (k = 0; k < res; k++) fputc(buf[k], stdout);
	#elif 0 /* truncate */
	res = fat_ftruncate(c, 65432);
	printf("Truncate: %i (%s)\n", res, strerror(-res));
	#endif
	res = fat_close(c);
	printf("Close: %i (%s)\n", res, strerror(-res));
	res = fat_unmount(v);
	printf("Unmount: %i (%s)\n", res, strerror(-res));
	#endif
	return 0;
}
