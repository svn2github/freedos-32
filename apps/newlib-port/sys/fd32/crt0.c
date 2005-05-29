
#include <stdlib.h>

#include "sys/syscalls.h"

#define MAX_FILES 20
static char **environ;
static char *__env[1] = {0};
static char *argv[255];
uint32_t mem_limit;
extern struct psp *current_psp;

extern int main(int argc,char **argv,char **envp);
static struct psp local_psp;

void libc_init(struct process_info *pi)
{
  int argc;
  char *p, *args;

  environ = __env;

  /* Set up args... */
  args = args_get(pi);
  mem_limit = maxmem_get(pi);
  argc = 1;
  argv[0] = name_get(pi);
  p = args;
  if (p != NULL) {
    while (*p != 0) {
      argv[argc++] = p;
      while ((*p != 0) && (*p != ' ')) {
        p++;
      }
      if (*p != 0) {
        *p++ = 0;
      }
      while(*p == ' ') {
        p++;
      };
    }
  }
  
  /* Set up the JFT */
  local_psp.jft_size = MAX_FILES;
  local_psp.jft = fd32_init_jft(MAX_FILES);
  local_psp.link = current_psp;
  current_psp = &local_psp;
  _exit(main(argc, argv, environ));
  /* FIXME: how do we restore the psp??? Simple: in exit.c!!! */
}
