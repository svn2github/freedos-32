#include <dr-env.h>
#include <errors.h>

static char *str1 = "modtest1: Hello World! again!~\n";
static char str2[] = "modtest2: Hello World! again!~\n";
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
  char *str3 = "modtest3: Hello World! again!~\n";
  char str4[] = "modtest4: Hello World! again!~\n";
  /* direct const (absolute) */
  fd32_message("modtest0: Hello World!~\n");
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
  fd32_message("modtest6: Hello World!~ %d\n", c);
  /* static function */
  c = func1(12, 8);
  fd32_message("modtest8: Hello World!~ 0x%x\n", c);
  
  /* global array */
  fd32_message(str9);
  return 0;
}
