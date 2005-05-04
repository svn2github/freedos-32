#include <sys/stat.h>
#include <sys/fcntl.h>

#include <ll/i386/hw-data.h>
#include <filesys.h>
#include <kernel.h>

int mkdir(const char *path, mode_t mode)
{
    int res;
    
    res = fd32_mkdir(path);
    /* TODO: call if(res<0)
        return -1;
    return 0;
}