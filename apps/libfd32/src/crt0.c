#include <ll/i386/hw-data.h>
#include <kernel.h>

char **environ;
char *__env[1] = {0};
char *argv[255];
DWORD mem_limit;

extern int main(int argc,char **argv,char **envp);

int libc_init(struct process_info *pi)
{
  int argc;
  char *p, *args;

  environ = __env;

  args = args_get(pi);
  mem_limit = maxmem_get(pi);
  argc = 0;
  p = args;
  while (*p != 0) {
    argv[argc++] = p;
    while ((*p != 0) && (*p != ' ')) {
      p++;
    }
    do {
      p++;
    } while ((*p != 0) || (*p == ' '));
  }

  return main(argc, argv, environ);
}
