#include <stdio.h>
#include <conio.h>
#include "fslayer.h"

extern void fat_driver_init();
extern void register_floppies();
extern void register_harddisks();


static void *TestJft;
static int   TestJftSize = 20;
static void *CdsList;


void *fd32_get_jft(int *JftSize)
{
  if (JftSize) *JftSize = TestJftSize;
  return TestJft;
}


void **fd32_get_cdslist()
{
  return &CdsList;
}


int rajeev_test()
{
  int    Handle;
  int    index;
  char   tempString[260];
  char   wsfileName[260];
  int    loopCount;

  for (loopCount = 0; loopCount < 102; loopCount++)
  {
    sprintf(wsfileName, "%s\\TestFile%d.txt", "d:", loopCount);

    Handle = fd32_open(wsfileName, FD32_OPEN_ACCESS_WRITE |
                       FD32_OPEN_NOTEXIST_CREATE, 0, 0, NULL);

/*    hDataLogFile = CreateFile(wsfileName,GENERIC_WRITE,0, NULL,
                              CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);*/
    if (Handle < 0)
    {
      printf("Error creating %s: %i\n", wsfileName, Handle);
      return Handle;
    }

    for (index = 0; index < 300; index++)
    {
      sprintf(tempString, "%d\n", index);
      //printf("Written: %i\n",
         (    fd32_write(Handle, tempString, strlen(tempString)));
    }
    printf("Closed: %i\n", fd32_close(Handle));
  }
  return 0;
}


#if 1
int main()
{
  int               Handle, Res = 0;
  int               k;
  fd32_dev_t        *Dev;
  fd32_fs_lfnfind_t FindData;
  fd32_fs_dosfind_t Dta;
  BYTE              Buffer[1024];

  /* Initialization... */
  _set_screen_lines(50);
  fat_driver_init();
  register_floppies();
  register_harddisks();
  assign_drive_letters();
  TestJft = fd32_init_jft(TestJftSize);
  if (TestJft == NULL)
  {
    printf("Error preparing the JFT\n");
    return -1;
  }
  fd32_set_default_drive('d');
  #if 0
  if ((Res = fd32_create_subst("Z", "D:\\root")) < 0)
  {
    printf("Cannot SUBST: %i\n", Res);
    return Res;
  }
  #endif
  printf("Press any key to start. CAREFULLY!!!\n");
  getch();

  /* Do something with the file system */
  //rajeev_test();
  #if 0
  Handle = fd32_open("\\",
                     FD32_OPEN_ACCESS_READ | FD32_OPEN_EXIST_OPEN
                     | FD32_OPEN_DIRECTORY, 0, 0, NULL);
  printf("\nHandle is %i\n", Handle);
  if (Handle < 0) return Handle;
  Res = fd32_read(Handle, Buffer, sizeof(Buffer));
  printf("%i bytes read\n", Res);
  {
    int x,y;
    for (y = 0; y < sizeof(Buffer);)
    {
      for (x = 0; x < 32; x++, y++)
        printf("%c", Buffer[y]);
      printf("\n");
    }
  }
  Res = fd32_close(Handle);
  printf("File closed with result %i\n", Res);
  #endif

  #if 0
  Handle = fd32_open("root\\..\\logistica.doc",
                     FD32_OPEN_ACCESS_READ | FD32_OPEN_EXIST_OPEN, 0, 0, NULL);
  printf("\nHandle is %i\n", Handle);
  if (Handle < 0) return Handle;
  Res = fd32_read(Handle, Buffer, sizeof(Buffer));
  printf("%i bytes read\n", Res);
  Res = fd32_close(Handle);
  printf("File closed with result %i\n", Res);

  Handle = fd32_open("root\\..\\logistica.pdf",
                     FD32_OPEN_ACCESS_READ | FD32_OPEN_EXIST_OPEN, 0, 0, NULL);
  printf("\nHandle is %i\n", Handle);
  if (Handle < 0) return Handle;
  Res = fd32_read(Handle, Buffer, sizeof(Buffer));
  printf("%i bytes read\n", Res);
  Res = fd32_close(Handle);
  printf("File closed with result %i\n", Res);
  #endif

  #if 1
  Handle = fd32_lfn_findfirst("*",
                              FD32_FIND_ALLOW_ALL | FD32_FIND_REQUIRE_NONE,
                              &FindData);
  printf("Filefind handle is %li\n",Handle);
  if (Handle < 0) return Handle;

  while (!Res)
  {
    printf("Long Name: \"%s\", Short: \"%s\"\n", FindData.LongName, FindData.ShortName);
    Res = fd32_lfn_findnext(Handle,
                            FD32_FIND_ALLOW_ALL | FD32_FIND_REQUIRE_NONE,
                            &FindData);
  }
  fd32_lfn_findclose(Handle);
  #endif

  #if 0
  Res = fd32_dos_findfirst("*.*", FD32_ATTR_ALL & ~FD32_ATTR_VOLUMEID, &Dta);
  while (!Res)
  {
    printf("\"%s\"\n", Dta.Name);
    Res = fd32_dos_findnext(&Dta);
  }
  #endif

//  fd32_chdir('c',"\\prova\\bello");
//  fd32_add_subst("c","hda1");
//  fd32_resolve_subst(Name, Name);
//  fd32_canonicalize(Name, Name);
//  printf("Name: %s\n",Name);
  return 0;
}
#endif
