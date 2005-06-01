/* FreeDOS-32 open/fcntl constants
 * by Salvo Isaja, May 2005
 */
#ifndef __FD32_FCNTL_H
#define __FD32_FCNTL_H

/* The following are derived rrom the DOS API (see RBIL table 01782) */
#define O_ACCMODE   (3 << 0)
#define O_RDONLY    (0 << 0)
#define O_WRONLY    (1 << 0)
#define O_RDWR      (2 << 0)
#define O_NOATIME   (1 << 2)  /* Do not update last access timestamp */
#define O_NOINHERIT (1 << 7)  /* Not inherited from child processes */
#define O_DIRECT    (1 << 8)  /* Direct (not buffered) disk access */
#define O_SYNC      (1 << 14) /* Commit after every write */
#define O_FSYNC     O_SYNC
/* The following are extensions */
#define O_DIRECTORY (1 << 16) /* Must be a directory */
#define O_LINK      (1 << 17) /* Do not follow links */
#define O_NOFOLLOW  O_LINK
#define O_CREAT     (1 << 18)
#define O_EXCL      (1 << 19)
#define O_TRUNC     (1 << 20)
#define O_APPEND    (1 << 21)
#define O_NOTRANS   (1 << 22)
#define O_SHLOCK    (1 << 23)
#define O_EXLOCK    (1 << 24)

#endif /* #ifndef __FD32_FCNTL_H */
