#include <ll/i386/hw-instr.h>
void _init(void)
{
  char c;
  int res, done;

  done = 0;
  sti();
  while(!done) {
    res = 0;
    while (res == 0) {
      res = keyb_read(0, 0, &c);
    }
    message("U pressed %c\n", c);
    if (c == 'q') {
      message("Ok, now testing blocking read...\n");
      done = 1;
    }
  }

  done = 0;
  while(!done) {
    res = 0;
    message("U pressed... ");
    res = keyb_read(0, 1, &c);
    message("%c!!!\n", c);
    if (c == 'q') {
      message("Ok, now testing n-char blocking read...\n");
      done = 1;
    }
  }

  done = 0;
  while(!done) {
    char c1[6];

    c1[5] = 0;
    res = 0;
    message("U pressed... ");
    res = keyb_read(0, 5, c1);
    message("%s!!!\n", c1);
    if (c1[0] == 'q') {
      message("G-Bye!!!\n");
      done = 1;
    }
  }
}
