#include <stdio.h>

int main(int argc, char *argv[])
{
  char c1, c2;
  char string[20];
  int a;

  c1 = getchar();
  printf("Got %c\n", c1);
  c2 = getchar();
  printf("Got %c\n", c2);
  scanf("%s", string);
  printf("String: %s\n", string);

  printf("Please, type a number:");
  scanf("%d", &a);
  printf("U Typed %d\n", a);
  return a;
}
