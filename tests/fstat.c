#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
  int fd, res;
  struct stat stat;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);

    exit(-1);
  }
  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("Open");

    exit(-1);
  }

  res = fstat(fd, &stat);
  if (res < 0) {
    perror("FStat");

    exit(-1);
  }
  printf("Size: %d\n", stat.st_size);

  return 0;
}
