#define x 20
#define y 10

void main(void)
{
  char *video;
  char *p;

  video = 0xb8000;
  p = video + x * 2 + 160 * y;

  *p++ = 'H';
  *p++ = ' ';
  *p++ = 'E';
  *p++ = ' ';
  *p++ = 'L';
  *p++ = ' ';
  *p++ = 'L';
  *p++ = ' ';
  *p++ = '0';
  *p++ = ' ';
}
