#include <ll/i386/hw-data.h>

#define EXCEPTION_MAXIMUM_PARAMETERS 15


/*#define CALLBACK __stdcall*/
/*#define STDCALL __stdcall*/
#define CALLBACK __attribute__ ((stdcall))
#define STDCALL __attribute__ ((stdcall))
#define WINAPI  __attribute__ ((stdcall))
#define VOID void

typedef int BOOL, WINBOOL, *PWINBOOL, *LPWINBOOL;
typedef void *HANDLE;
typedef void *PVOID, *LPVOID;
typedef char CHAR, *LPSTR;
typedef const char *LPCSTR;
typedef unsigned long int ULONG;
typedef long int LONG;
typedef unsigned int UINT;
typedef long long LONGLONG;
typedef WORD ATOM;
typedef HANDLE HLOCAL;
typedef CONTEXT *PCONTEXT, *LPCONTEXT;

typedef struct _EXCEPTION_RECORD {
  DWORD ExceptionCode;
  DWORD ExceptionFlags;
  struct _EXCEPTION_RECORD *ExceptionRecord;
  PVOID ExceptionAddress;
  DWORD NumberParameters;
  DWORD ExceptionInformation[EXCEPTION_MAXIMUM_PARAMETERS];
} EXCEPTION_RECORD,*PEXCEPTION_RECORD,*LPEXCEPTION_RECORD;

typedef struct _EXCEPTION_POINTERS {
  PEXCEPTION_RECORD ExceptionRecord;
  PCONTEXT ContextRecord;
} EXCEPTION_POINTERS,*PEXCEPTION_POINTERS,*LPEXCEPTION_POINTERS;

typedef struct _FILETIME {
  DWORD dwLowDateTime;
  DWORD dwHighDateTime;
} FILETIME,*PFILETIME,*LPFILETIME;

typedef union _LARGE_INTEGER {
  struct {
    DWORD LowPart;
    LONG  HighPart;
  } u;
  LONGLONG QuadPart;
} LARGE_INTEGER, *PLARGE_INTEGER;

typedef LONG(CALLBACK *PTOP_LEVEL_EXCEPTION_FILTER)(LPEXCEPTION_POINTERS);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;
