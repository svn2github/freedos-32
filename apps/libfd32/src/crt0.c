#include <ll/i386/hw-data.h>
#include <kernel.h>
#include <stdlib.h>

#define MAX_FILES 20
static char *environ[1] = {0};

extern int main(int argc,char **argv,char **envp);

void libc_init(struct process_info *pi)
{
  int ret;
  char **argv;
  int argc = fd32_get_argv(name_get(pi), args_get(pi), &argv);
  
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
