#include <stdio.h>

int a, c;

int main(int argc, char *argv[])
{
  int b;
  printf("Going to test the exception mechanism...\n");
  b = 0; a = 100;
  printf("Computing %d / %d\n", a, b);
  c = a / b;
  printf("I Should not be here...\n");
  
  return a;
}
