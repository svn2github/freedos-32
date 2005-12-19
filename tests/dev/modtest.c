#include <dr-env.h>

static int integer0;
static int integer1 = 0;
int integer2 = 0x10;

static char *str1 = "static pointer to const: Hello World! again!~\n";
static char str2[] = "static array: Hello World! again!~\n";
char str9[] = "modtest9: Hello World! again!~\n";

int func(int a, int b)
{
  int c = a*b;
  fd32_message("modtest5: Hello World!~ %d\n", c);
  return c;
}

static func1(int a, int b)
{
  int c = a*b;
  fd32_message("modtest7: Hello World!~ 0x%x\n", c);
  return c;
}

int modtest_init(int argc, char *argv[])
{
  char *str3 = "local pointer to const (stack): Hello World! again!~\n";
  char str4[] = "local array (stack): Hello World! again!~\n";
  /* direct const (absolute) */
  fd32_message("direct const: Hello World!~\n");
  /* static pointer to const */
  fd32_message(str1);
  /* static array */
  fd32_message(str2);
  /* local pointer to const (stack) */
  fd32_message(str3);
  /* local array (stack) */
  fd32_message(str4);
  /* global function */
  int c = func(12, 4);
  fd32_message("global function: Hello World!~ %d\n", c);
  /* static function */
  c = func1(12, 8);
  fd32_message("static function: Hello World!~ 0x%x\n", c);
  /* static un-initialized */
  integer0 = func1(12, 8);
  fd32_message("static un-initialized variable: Hello World!~ 0x%x\n", c);
  
  /* global array */
  fd32_message(str9);
  
  /* static integer */
  fd32_message("static integer: %d\n", integer0);
  /* static integer initialized to zero */
  fd32_message("static integer initialized to zero: %d\n", integer1);
  /* global integer */
  fd32_message("global integer initialized: %d\n", integer2);
  return 0;
}
