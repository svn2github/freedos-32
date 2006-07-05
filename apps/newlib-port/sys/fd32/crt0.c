
#include <stdlib.h>

#include "sys/syscalls.h"

#define RESIDENT  0x8000
#define MAX_FILES 20
static char *environ[1] = {0};

extern int main(int argc,char **argv,char **envp);

int __main(void)
{
	return 0;
}

void libc_init(struct process_info *pi)
{
  int ret;
  char **argv;
  int argc = fd32_get_argv(pi->filename, pi->args, &argv);

  /* Make it a normal native program (not a resident module) */
  pi->type &= ~RESIDENT;

  /* Set up the JFT */
  pi->jft_size = MAX_FILES;
  pi->jft = fd32_init_jft(MAX_FILES);

  /* program's entry-point */
  ret = main(argc, argv, environ);

  /* Free the argv */
  fd32_unget_argv(argc, argv);
  /* NOTE: Free the JFT in exit.c */
  exit(ret);
}
