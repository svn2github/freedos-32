#include <dr-env.h>

int contest_init(struct process_info *pi)
{
  char c;

  pi->jft_size = 12;
  pi->jft = fd32_init_jft(12);

  while(c != 0x1B)
  {
    fd32_read(0, &c, 1);
    fd32_message("KEY CODE: %x\n", c);
  }
  return 0;
}
