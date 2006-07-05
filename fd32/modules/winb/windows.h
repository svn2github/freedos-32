#ifndef __WINDOWS_H__
#define __WINDOWS_H__


#define EXCEPTION_MAXIMUM_PARAMETERS 15

#define VER_PLATFORM_WIN32s        0
#define VER_PLATFORM_WIN32_WINDOWS 1
#define VER_PLATFORM_WIN32_NT      2

#define PROCESSOR_INTEL_386     386
#define PROCESSOR_INTEL_486     486
#define PROCESSOR_INTEL_PENTIUM 586
#define PROCESSOR_INTEL_IA64    2200

#define PROCESSOR_ARCHITECTURE_INTEL 0
#define PROCESSOR_ARCHITECTURE_PPC   3
#define PROCESSOR_ARCHITECTURE_ARM   5
#define PROCESSOR_ARCHITECTURE_IA64  6
#define PROCESSOR_ARCHITECTURE_AMD64 9
#define PROCESSOR_ARCHITECTURE_UNKNOWN 0xFFFF

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

#define STARTF_USESHOWWINDOW 1
#define STARTF_USESIZE 2
#define STARTF_USEPOSITION 4
#define STARTF_USECOUNTCHARS 8
#define STARTF_USEFILLATTRIBUTE 16
#define STARTF_RUNFULLSCREEN 32
#define STARTF_FORCEONFEEDBACK 64
#define STARTF_FORCEOFFFEEDBACK 128
#define STARTF_USESTDHANDLES 256
#define STARTF_USEHOTKEY 512

#define MAXIMUM_ALLOWED 0x2000000
#define GENERIC_READ    0x80000000
#define GENERIC_WRITE   0x40000000
#define GENERIC_EXECUTE 0x20000000
#define GENERIC_ALL     0x10000000

#define CREATE_NEW        1
#define CREATE_ALWAYS     2
#define OPEN_EXISTING     3
#define OPEN_ALWAYS       4
#define TRUNCATE_EXISTING 5

#define ENABLE_LINE_INPUT 2
#define ENABLE_ECHO_INPUT 4
#define ENABLE_PROCESSED_INPUT 1
#define ENABLE_WINDOW_INPUT 8
#define ENABLE_MOUSE_INPUT 16
#define ENABLE_PROCESSED_OUTPUT 1
#define ENABLE_WRAP_AT_EOL_OUTPUT 2

#define FOREGROUND_BLUE      1
#define FOREGROUND_GREEN     2
#define FOREGROUND_RED       4
#define FOREGROUND_INTENSITY 8
#define BACKGROUND_BLUE      16
#define BACKGROUND_GREEN     32
#define BACKGROUND_RED       64
#define BACKGROUND_INTENSITY 128
#if (_WIN32_WINNT >= 0x0501)
#define CONSOLE_FULLSCREEN_MODE 1
#define CONSOLE_WINDOWED_MODE   0
#endif 

#define CALLBACK __attribute__ ((stdcall))
#define STDCALL __attribute__ ((stdcall))
#define WINAPI  __attribute__ ((stdcall))
#define CONST const
#define VOID  void
#define TRUE  1
#define FALSE 0

/* Basic data types */
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef DWORD *PDWORD, *LPDWORD;
typedef BYTE *PBYTE, *LPBYTE;
typedef int BOOL, WINBOOL, *PWINBOOL, *LPWINBOOL;
typedef void *PVOID, *LPVOID;
typedef const void *PCVOID,*LPCVOID;
typedef char CHAR, *LPSTR;
typedef const char *LPCSTR;
typedef wchar_t WCHAR;
typedef WORD ATOM;
/* Integer types */
typedef short SHORT;
typedef long LONG, *PLONG;
typedef long LONG_PTR, *PLONG_PTR;
typedef unsigned long ULONG;
typedef unsigned int UINT;
typedef long long LONGLONG;
/* Handle types */
typedef void *HANDLE;
typedef HANDLE HKEY;
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
#define STD_INPUT_HANDLE (DWORD)(0xfffffff6)
#define STD_OUTPUT_HANDLE (DWORD)(0xfffffff5)
#define STD_ERROR_HANDLE (DWORD)(0xfffffff4)
#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define INVALID_FILE_ATTRIBUTES	((DWORD)-1)

#define MAXIMUM_SUPPORTED_EXTENSION  512
typedef struct _FLOATING_SAVE_AREA {
  DWORD  ControlWord;
  DWORD  StatusWord;
  DWORD  TagWord;
  DWORD  ErrorOffset;
  DWORD  ErrorSelector;
  DWORD  DataOffset;
  DWORD  DataSelector;
  BYTE  RegisterArea[80];
  DWORD  Cr0NpxState;
} FLOATING_SAVE_AREA;
typedef struct _CONTEXT {
  DWORD  ContextFlags;
  DWORD  Dr0;
  DWORD  Dr1;
  DWORD  Dr2;
  DWORD  Dr3;
  DWORD  Dr6;
  DWORD  Dr7;
  FLOATING_SAVE_AREA FloatSave;
  DWORD  SegGs;
  DWORD  SegFs;
  DWORD  SegEs;
  DWORD  SegDs;
  DWORD  Edi;
  DWORD  Esi;
  DWORD  Ebx;
  DWORD  Edx;
  DWORD  Ecx;
  DWORD  Eax;
  DWORD  Ebp;
  DWORD  Eip;
  DWORD  SegCs;
  DWORD  EFlags;
  DWORD  Esp;
  DWORD  SegSs;
  BYTE  ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];
} CONTEXT;
typedef CONTEXT *PCONTEXT, *LPCONTEXT;

typedef struct _CHAR_INFO {
  union {
    WCHAR UnicodeChar;
    CHAR AsciiChar;
  } Char;
  WORD Attributes;
} CHAR_INFO, *PCHAR_INFO;

typedef struct _CONSOLE_CURSOR_INFO {
  DWORD dwSize;
  BOOL  bVisible;
} CONSOLE_CURSOR_INFO,*PCONSOLE_CURSOR_INFO;

typedef struct _COORD {
  SHORT X;
  SHORT Y;
} COORD, *PCOORD;

typedef struct _SMALL_RECT {
  SHORT Left;
  SHORT Top;
  SHORT Right;
  SHORT Bottom;
} SMALL_RECT, *PSMALL_RECT;

typedef struct _CONSOLE_SCREEN_BUFFER_INFO {
  COORD  dwSize;
  COORD  dwCursorPosition;
  WORD   wAttributes;
  SMALL_RECT srWindow;
  COORD  dwMaximumWindowSize;
} CONSOLE_SCREEN_BUFFER_INFO, *PCONSOLE_SCREEN_BUFFER_INFO;

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

typedef struct _OSVERSIONINFOA {
  DWORD dwOSVersionInfoSize;
  DWORD dwMajorVersion;
  DWORD dwMinorVersion;
  DWORD dwBuildNumber;
  DWORD dwPlatformId;
  CHAR szCSDVersion[128];
} OSVERSIONINFOA, *POSVERSIONINFOA, *LPOSVERSIONINFOA;

typedef struct _OVERLAPPED {
  DWORD Internal;
  DWORD InternalHigh;
  DWORD Offset;
  DWORD OffsetHigh;
  HANDLE hEvent;
} OVERLAPPED, *POVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES {
  DWORD nLength;
  LPVOID lpSecurityDescriptor;
  BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef struct _STARTUPINFOA {
  DWORD  cb;
  LPSTR  lpReserved;
  LPSTR  lpDesktop;
  LPSTR  lpTitle;
  DWORD  dwX;
  DWORD  dwY;
  DWORD  dwXSize;
  DWORD  dwYSize;
  DWORD  dwXCountChars;
  DWORD  dwYCountChars;
  DWORD  dwFillAttribute;
  DWORD  dwFlags;
  WORD   wShowWindow;
  WORD   cbReserved2;
  PBYTE  lpReserved2;
  HANDLE hStdInput;
  HANDLE hStdOutput;
  HANDLE hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;

typedef struct _SYSTEM_INFO {
  DWORD dwOemId;
  DWORD dwPageSize;
  PVOID lpMinimumApplicationAddress;
  PVOID lpMaximumApplicationAddress;
  DWORD dwActiveProcessorMask;
  DWORD dwNumberOfProcessors;
  DWORD dwProcessorType;
  DWORD dwAllocationGranularity;
  WORD wProcessorLevel;
  WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

typedef LONG(CALLBACK *PTOP_LEVEL_EXCEPTION_FILTER)(LPEXCEPTION_POINTERS);
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;


#endif
