#include <stdio.h>

int main(int argc, char *argv[])
{
  FILE *f;
  char buff[100];
  int res;

  f = fopen("c:\\test.txt", "r");

  if (f == NULL) {
    perror("Error opening test.txt");
    exit(-1);
  }
  res = fread(buff, 100, 1, f);
  buff[99] = 0;
  printf("Read %d chars: %s\n", res, buff);
  fclose(f);

  return 0;
}
