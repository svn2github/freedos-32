#include <stdio.h>

int a;
float f;

int main(int argc, char *argv[])
{
  a = 6;
  f = 3.14;
  _write(1, "Hello, world\n", 13);
  printf("%f Hi again...%d\n", f, a);
  return 100;
}
