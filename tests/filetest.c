#include <stdio.h>

#define FILENAME "c:\\test.txt"

int main(int argc, char *argv[])
{
  FILE *f;
  char buff[100];
  int res;

  printf("Going to open \"%s\"...\n", FILENAME);
  f = fopen(FILENAME, "rb");

  if (f == NULL) {
    perror("Error opening the file");
    exit(-1);
  }
  res = fread(buff, 1, 100, f);
  buff[99] = 0;
  printf("Read %d bytes: %s\n", res, buff);
  fclose(f);

  return 0;
}
