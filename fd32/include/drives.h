#ifndef __KFS_H__
#define __KFS_H__
char *get_name_without_drive(char *FileName);
void *get_drive(char *file_name);

int  fd32_set_default_drive(char Drive);
char fd32_get_default_drive();

#endif	/* __KFS_H__ */
