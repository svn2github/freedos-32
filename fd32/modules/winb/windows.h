#include <ll/i386/hw-data.h>

#define EXCEPTION_MAXIMUM_PARAMETERS 15


/*#define CALLBACK __stdcall*/
/*#define STDCALL __stdcall*/
#define CALLBACK __attribute__ ((stdcall))
#define STDCALL __attribute__ ((stdcall))
#define VOID void

typedef int BOOL, WINBOOL,*PWINBOOL,*LPWINBOOL;
typedef void *HANDLE;
typedef void *PVOID,*LPVOID;
typedef char CHAR;
typedef unsigned long int ULONG;
typedef long int LONG;
typedef unsigned int UINT;
typedef CHAR TCHAR;
typedef TCHAR *LPTCH,*PTSTR,*LPTSTR,*LP,*PTCHAR;
typedef const TCHAR *LPCTSTR;
typedef WORD ATOM;

typedef HANDLE HLOCAL;


#undef CONTEXT
typedef struct _CONTEXT {
	ULONG ContextFlags;

	ULONG R0;
	ULONG R1;
	ULONG R2;
	ULONG R3;
	ULONG R4;
	ULONG R5;
	ULONG R6;
	ULONG R7;
	ULONG R8;
	ULONG R9;
	ULONG R10;
	ULONG R11;
	ULONG R12;

	ULONG Sp;
	ULONG Lr;
	ULONG Pc;
	ULONG Psr;
} CONTEXT;

typedef CONTEXT *PCONTEXT,*LPCONTEXT;

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



typedef LONG(CALLBACK *PTOP_LEVEL_EXCEPTION_FILTER)(LPEXCEPTION_POINTERS);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;
