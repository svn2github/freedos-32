#include <stdio.h>
#include <process.h>

char *env[] = {
    0
};

int main(int argc, char *argv[])
{
  int res;
  
  printf("Going to execute...\n");
  res = _dos_exec("c:\\test.exe", "", env);
  if (res < 0) {
    perror("Cannot execute");
  } else {
    printf("Done\n");
  }
}
