#include <dr-env.h>
#include <errors.h>
#include <stubinfo.h>

int contest_init(int argc, char *argv[])
{
  char c;
  struct psp new_psp;
  extern struct psp *current_psp;

  current_psp = &new_psp;
  new_psp.jft = fd32_init_jft(8);
  new_psp.jft_size = 8;

  while(c != 0x1B)
  {
    fd32_read(0, &c, 1);
    fd32_message("KEY CODE: %x\n", c);
  }
  return 0;
}
