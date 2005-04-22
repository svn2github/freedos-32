#ifndef __WINDOWS_H__
#define __WINDOWS_H__


#define EXCEPTION_MAXIMUM_PARAMETERS 15

/* Error numbers */
#define ERROR_SUCCESS 0L
#define NO_ERROR 0L
#define ERROR_INVALID_FUNCTION 1L
#define ERROR_FILE_NOT_FOUND 2L
#define ERROR_PATH_NOT_FOUND 3L
#define ERROR_TOO_MANY_OPEN_FILES 4L
#define ERROR_ACCESS_DENIED 5L
#define ERROR_INVALID_HANDLE 6L
#define ERROR_ARENA_TRASHED 7L
#define ERROR_NOT_ENOUGH_MEMORY 8L
#define ERROR_INVALID_BLOCK 9L
#define ERROR_BAD_ENVIRONMENT 10L
#define ERROR_BAD_FORMAT 11L
#define ERROR_INVALID_ACCESS 12L
#define ERROR_INVALID_DATA 13L
#define ERROR_OUTOFMEMORY 14L
#define ERROR_INVALID_DRIVE 15L
#define ERROR_CURRENT_DIRECTORY 16L
#define ERROR_NOT_SAME_DEVICE 17L
#define ERROR_NO_MORE_FILES 18L
#define ERROR_WRITE_PROTECT 19L
#define ERROR_BAD_UNIT 20L
#define ERROR_NOT_READY 21L
#define ERROR_BAD_COMMAND 22L
#define ERROR_CRC 23L
#define ERROR_BAD_LENGTH 24L
#define ERROR_SEEK 25L
#define ERROR_NOT_DOS_DISK 26L
#define ERROR_SECTOR_NOT_FOUND 27L
#define ERROR_OUT_OF_PAPER 28L
#define ERROR_WRITE_FAULT 29L
#define ERROR_READ_FAULT 30L
#define ERROR_GEN_FAILURE 31L
#define ERROR_SHARING_VIOLATION 32L
#define ERROR_LOCK_VIOLATION 33L
#define ERROR_WRONG_DISK 34L
#define ERROR_SHARING_BUFFER_EXCEEDED 36L
#define ERROR_HANDLE_EOF 38L
#define ERROR_HANDLE_DISK_FULL 39L
#define ERROR_NOT_SUPPORTED 50L


#define CALLBACK __attribute__ ((stdcall))
#define STDCALL __attribute__ ((stdcall))
#define WINAPI  __attribute__ ((stdcall))
#define VOID  void
#define TRUE  1
#define FALSE 0

/* Basic data types */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL, WINBOOL, *PWINBOOL, *LPWINBOOL;
typedef void *PVOID, *LPVOID;
typedef char CHAR, *LPSTR;
typedef const char *LPCSTR;
typedef WORD ATOM;
/* Integer types */
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned int UINT;
typedef long long LONGLONG;
/* Handle types */
typedef void *HANDLE;
typedef HANDLE HKEY;
typedef HANDLE HLOCAL;

#define MAXIMUM_SUPPORTED_EXTENSION  512
typedef struct _FLOATING_SAVE_AREA {
	DWORD	ControlWord;
	DWORD	StatusWord;
	DWORD	TagWord;
	DWORD	ErrorOffset;
	DWORD	ErrorSelector;
	DWORD	DataOffset;
	DWORD	DataSelector;
	BYTE	RegisterArea[80];
	DWORD	Cr0NpxState;
} FLOATING_SAVE_AREA;
typedef struct _CONTEXT {
	DWORD	ContextFlags;
	DWORD	Dr0;
	DWORD	Dr1;
	DWORD	Dr2;
	DWORD	Dr3;
	DWORD	Dr6;
	DWORD	Dr7;
	FLOATING_SAVE_AREA FloatSave;
	DWORD	SegGs;
	DWORD	SegFs;
	DWORD	SegEs;
	DWORD	SegDs;
	DWORD	Edi;
	DWORD	Esi;
	DWORD	Ebx;
	DWORD	Edx;
	DWORD	Ecx;
	DWORD	Eax;
	DWORD	Ebp;
	DWORD	Eip;
	DWORD	SegCs;
	DWORD	EFlags;
	DWORD	Esp;
	DWORD	SegSs;
	BYTE	ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT;
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

typedef struct _SECURITY_ATTRIBUTES {
	DWORD nLength;
	LPVOID lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef LONG(CALLBACK *PTOP_LEVEL_EXCEPTION_FILTER)(LPEXCEPTION_POINTERS);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;


#endif
