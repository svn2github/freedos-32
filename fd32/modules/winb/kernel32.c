/* Mini KERNEL32
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <windows.h>

#include "winb.h"


static LPCSTR atomname = 0;
static ATOM WINAPI fd32_imp__AddAtomA( LPCSTR str )
{
  printf("AddAtomA: %s\n", str);
  atomname = str;
  return 1;
}

static ATOM WINAPI fd32_imp__FindAtomA( LPCSTR str )
{
  printf("FindAtomA: %s\n", str);
  if (atomname != 0)
    return 1;
  else
    return 0;
}

static UINT WINAPI fd32_imp__GetAtomNameA( DWORD atom, LPSTR buffer, int nsize )
{
  if (atomname != 0)
    strcpy(buffer, atomname);
  else
    nsize = 0;
  return nsize;
}

static UINT WINAPI fd32_imp__SetErrorMode(UINT uMode)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetErrorMode: %x, not implemented, always return 0!\n", uMode);
#endif
  return 0;
}

static DWORD errcode;
static DWORD WINAPI fd32_imp__GetLastError(VOID)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GetLastError: %lx\n", errcode);
#endif
  return errcode;
}

static VOID WINAPI fd32_imp__SetLastError(DWORD dwErrCode)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetLastError: %lx\n", errcode);
#endif
  errcode = dwErrCode;
}

static BOOL WINAPI fd32_imp__GetVersionExA(LPOSVERSIONINFOA lpVersionInformation)
{
  /* TODO: Check the PlatformId since it's the minimum implementation of Win32 APIs */
  if (lpVersionInformation != NULL && lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOA)) {
    lpVersionInformation->dwMajorVersion = 0x05;
    lpVersionInformation->dwMinorVersion = 0x01;
    lpVersionInformation->dwBuildNumber  = 0x05;
    lpVersionInformation->dwPlatformId   = VER_PLATFORM_WIN32_NT;
    strcpy(lpVersionInformation->szCSDVersion, "FreeDOS-32 WINB 0.05 alpha");
    return TRUE;
  } else {
    return FALSE;
  }
}

static VOID WINAPI fd32_imp__GetSystemInfo(LPSYSTEM_INFO lpSystemInfo)
{
  /* TODO: Check the memory limits, get the current processor type, level and revision */
  if (lpSystemInfo != NULL) {
    lpSystemInfo->dwOemId = PROCESSOR_ARCHITECTURE_INTEL;
    lpSystemInfo->dwPageSize = sysconf(_SC_PAGESIZE);
    lpSystemInfo->lpMinimumApplicationAddress = (LPVOID)0x10000;
    lpSystemInfo->lpMaximumApplicationAddress = (LPVOID)0x7FFEFFFF;
    lpSystemInfo->dwActiveProcessorMask = 1;
    lpSystemInfo->dwNumberOfProcessors = 1;
    lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_386;
    lpSystemInfo->dwAllocationGranularity = 0x10000;
    lpSystemInfo->wProcessorLevel = 3;
    lpSystemInfo->wProcessorRevision = 0;
  }
}

static PTOP_LEVEL_EXCEPTION_FILTER top_filter;
static LPVOID WINAPI fd32_imp__SetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
  LPTOP_LEVEL_EXCEPTION_FILTER old = top_filter;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetUnhandledExceptionFilter: %lx\n", (DWORD)filter);
#endif
  top_filter = filter;
  return old;
}

static BOOL WINAPI fd32_imp__PeekNamedPipe(HANDLE hNamedPipe, LPVOID lpBuffer, DWORD nBufferSize, LPDWORD lpBytesRead, LPDWORD lpTotalBytesAvail, LPDWORD lpBytesLeftThisMessage)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] PeekNamedPipe: %lx\n", (DWORD)hNamedPipe);
#endif
  return FALSE;
}


/* ______________________________ *\
 *|                              |*
 *|Dynamic-Link Library Functions|*
\*|______________________________|*/

extern struct psp *current_psp;
static DWORD WINAPI fd32_imp__GetModuleHandleA( LPCSTR module )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GetModuleHandle: %s\n", module);
#endif
  return (DWORD)current_psp;
}


/* ______________________________ *\
 *|                              |*
 *| Process and Thread Functions |*
\*|______________________________|*/

void restore_sp(DWORD s);
static VOID WINAPI fd32_imp__ExitProcess( UINT ecode )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] ExitProcess: %x and do memory clear-up!\n", ecode);
#endif
  if (current_psp->mem_clear_up != NULL)
    current_psp->mem_clear_up();
  restore_sp(ecode);
}

static LPSTR WINAPI fd32_imp__GetCommandLineA(VOID)
{
  return current_psp->command_line;
}

static LPSTR WINAPI fd32_imp__GetEnvironmentStringsA(void)
{
  /* NOTE: FD32 currently support GDT only */
  return (LPSTR)gdt_read(current_psp->environment_selector, NULL, NULL, NULL);
}

static VOID WINAPI fd32_imp__GetStartupInfoA(LPSTARTUPINFOA lpStartupInfo)
{
  /* TODO: Set StartupInfo title to be the program's name */
  if (lpStartupInfo != NULL && lpStartupInfo->cb == sizeof(STARTUPINFOA)) {
    lpStartupInfo->lpReserved = NULL;
    lpStartupInfo->lpDesktop = "FD32-DESKTOP";
    lpStartupInfo->lpTitle = NULL;
    lpStartupInfo->dwX = 0;
    lpStartupInfo->dwY = 0;
    lpStartupInfo->dwXSize = 0;
    lpStartupInfo->dwYSize = 0;
    lpStartupInfo->dwXCountChars = 80;
    lpStartupInfo->dwYCountChars = 25;
    lpStartupInfo->dwFillAttribute = 0;
    lpStartupInfo->dwFlags = STARTF_USESHOWWINDOW|STARTF_USECOUNTCHARS|STARTF_USECOUNTCHARS;
    lpStartupInfo->wShowWindow = 5; /* SW_SHOW */
    lpStartupInfo->cbReserved2 = 0;
    lpStartupInfo->lpReserved2 = NULL;
    lpStartupInfo->hStdInput = INVALID_HANDLE_VALUE; /* Not specified */
    lpStartupInfo->hStdOutput = INVALID_HANDLE_VALUE;
    lpStartupInfo->hStdError = INVALID_HANDLE_VALUE;
  }
}

/* The GetCurrentProcessId function returns the process identifier of the calling process.
 *
 * Return Values
 * The return value is the process identifier of the calling process. 
 */
static DWORD WINAPI fd32_imp__GetCurrentProcessId( void )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GetCurrentProcessId ...\n");
#endif
  return (DWORD)current_psp;
}

/* The GetCurrentThreadId function returns the thread identifier of the calling thread. 
 *
 * Return Values
 * The return value is the thread identifier of the calling thread. 
 */
static DWORD WINAPI fd32_imp__GetCurrentThreadId( void )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GetCurrentThreadId ... not supported\n");
#endif
  /* FD32 currently no multi-threading */
  return 0;
}

static void WINAPI fd32_imp__Sleep(DWORD dwMilliseconds)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] Sleep ... not implemented\n");
#endif
  /* TODO: Using int nanosleep (const struct timespec *requested_time, struct timespec *remaining) in newlib */
  /* int nanosleep(const struct timespec *rqtp, struct timespec *rmtp);
     struct timespec rqt = {dwMilliseconds/1000, dwMilliseconds*1000000-(dwMilliseconds/1000)*1000000000};
     struct timespec rmt;
     nanosleep(&rqt, &rmt); */
  // sleep(dwMilliseconds/1000);
}


/* ______________________________ *\
 *|                              |*
 *|        Time Functions        |*
\*|______________________________|*/

/* The GetSystemTimeAsFileTime function obtains the current system date and time. The information is in Coordinated Universal Time (UTC) format.
 *
 * Parameters
 * lpSystemTimeAsFileTime
 *   Pointer to a FILETIME structure to receive the current system date and time in UTC format. 
 */
static void WINAPI fd32_imp__GetSystemTimeAsFileTime( LPFILETIME lpftime )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GetSystemTimeAsFileTime: %lx ... not implemented\n", (DWORD)lpftime);
#endif
}

/* The GetTickCount function retrieves the number of milliseconds that have elapsed since Windows was started. 
 *
 * Return Values
 * If the function succeeds, the return value is the number of milliseconds that have elapsed since Windows was started. 
 */
static DWORD WINAPI fd32_imp__GetTickCount( VOID )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GetTickCount ... not implemented\n");
#endif
  return 0;
}

/* The QueryPerformanceCounter function retrieves the current value of the high-resolution performance counter, if one exists. 
 *
 * Parameters
 * lpPerformanceCount
 *   Points to a variable that the function sets, in counts, to the current performance-counter value. If the installed hardware does not support a high-resolution performance counter, this parameter can be to zero. 
 *
 * Return Values
 * If the installed hardware supports a high-resolution performance counter, the return value is nonzero.
 * If the installed hardware does not support a high-resolution performance counter, the return value is zero. 
 */
static BOOL WINAPI fd32_imp__QueryPerformanceCounter( PLARGE_INTEGER lpcount )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] QueryPerformanceCounter ... not implemented, return FALSE\n");
#endif
  return FALSE;
}


/* ______________________________ *\
 *|                              |*
 *| Memory Management Functions  |*
\*|______________________________|*/

static LPVOID WINAPI fd32_imp__VirtualAlloc( LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] VirtualAlloc: at %lx with %d bytes\n", (DWORD)lpAddress, dwSize);
#endif
  if (mem_get_region((uint32_t)lpAddress, dwSize+sizeof(DWORD)) == 1) {
    DWORD *pd = (DWORD *) lpAddress;
    pd[0] = dwSize+sizeof(DWORD);
    return pd+1;
  } else {
    return NULL;
  }
}

static BOOL WINAPI fd32_imp__VirtualFree( LPVOID lpAddress, size_t dwSize, DWORD dwFreeType )
{
  DWORD *pd = (DWORD *)lpAddress;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] VirtualFree: at %lx with %d bytes\n", (DWORD)lpAddress, dwSize);
#endif
  if (pd != NULL) {
    mem_free((uint32_t)(pd-1), pd[0]);
    return TRUE;
  } else {
    return FALSE;
  }
}

/*
 * The GlobalAlloc function allocates the specified number of bytes from the heap. In the linear Win32 API environment, there is no difference between the local heap and the global heap.
 *
 * Return Values
 * If the function succeeds, the return value is the handle of the newly allocated memory object.
 * If the function fails, the return value is NULL. To get extended error information, call GetLastError.
 */
static HGLOBAL WINAPI fd32_imp__GlobalAlloc( UINT uFlags, size_t uBytes )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GlobalAlloc: %d bytes\n", uBytes);
#endif
  /* Allocate memory from heap, so using malloc implemented in newlib */
  return malloc(uBytes);
}

/*
 * The GlobalFree function frees the specified global memory object and invalidates its handle. 
 *
 * Return Values
 * If the function succeeds, the return value is NULL.
 * If the function fails, the return value is equal to the handle of the global memory object. To get extended error information, call GetLastError. 
 */
static HGLOBAL WINAPI fd32_imp__GlobalFree( HGLOBAL hMem )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GlobalFree: %lx\n", (DWORD)hMem);
#endif
  free(hMem);
  return NULL;
}

/*
 * The LocalAlloc function allocates the specified number of bytes from the heap. In the linear Win32 API environment, there is no difference between the local heap and the global heap. 
 *
 * Return Values
 * If the function succeeds, the return value is the handle of the newly allocated memory object.
 * If the function fails, the return value is NULL. To get extended error information, call GetLastError. 
 */
static HLOCAL WINAPI fd32_imp__LocalAlloc( UINT uFlags, size_t uBytes )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] LocalAlloc: %d bytes\n", uBytes);
#endif
  /* Allocate memory from heap, so using malloc implemented in newlib */
  return malloc(uBytes);
}

/*
 * The LocalFree function frees the specified local memory object and invalidates its handle. 
 *
 * Return Values
 * If the function succeeds, the return value is NULL.
 * If the function fails, the return value is equal to the handle of the local memory object. 
 */
static HLOCAL WINAPI fd32_imp__LocalFree( HLOCAL hMem )
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] LocalFree: %lx\n", (DWORD)hMem);
#endif
  free(hMem);
  return NULL;
}


/* ______________________________ *\
 *|                              |*
 *|       File Functions         |*
\*|______________________________|*/

/* The CloseHandle function closes an open object handle. 
 *
 * Return Values
 * If the function succeeds, the return value is nonzero.
 * If the function fails, the return value is zero. To get extended error information, call GetLastError. 
 */
static BOOL WINAPI fd32_imp__CloseHandle(HANDLE hObject)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] CloseHandle: %lx\n", (DWORD)hObject);
#endif
  if (close((int)hObject) == 0)
    return TRUE;
  else
    return FALSE;
}

/* The CreateFile function creates or opens the following objects and returns a handle that can be used to access the object: 
 *  files 
 *  pipes 
 *  mailslots 
 *  communications resources 
 *  disk devices (Windows NT only)
 *  consoles 
 *  directories (open only)
 * Return Values
 * If the function succeeds, the return value is an open handle to the specified file. If the specified file exists before the function call and dwCreationDistribution is CREATE_ALWAYS or OPEN_ALWAYS, a call to GetLastError returns ERROR_ALREADY_EXISTS (even though the function has succeeded). If the file does not exist before the call, GetLastError returns zero. 
 * If the function fails, the return value is INVALID_HANDLE_VALUE. To get extended error information, call GetLastError.
 */
static HANDLE WINAPI fd32_imp__CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
  int filedes, mode, flags;

#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] CreateFileA: %s (%lx), %lx, %lx, ...\n", lpFileName, *((DWORD *)lpFileName), dwDesiredAccess, dwShareMode);
#endif
  if (*((DWORD *)lpFileName) == 0x494E4F43 && *((WORD *)(lpFileName+4)) == 0x244E) {
#ifdef __WINB_DEBUG__
    fd32_log_printf("[WINB] Create CONIN$ handle\n");
#endif
    return (HANDLE)0;
  } else if (*((DWORD *)lpFileName) == 0x4F4E4F43 && *((DWORD *)(lpFileName+4)) == 0x00245455) {
#ifdef __WINB_DEBUG__
    fd32_log_printf("[WINB] Create CONOUT$ handle\n");
#endif
    return (HANDLE)1;
  }
  /* access (read-write) mode */
  
  if (dwDesiredAccess == (GENERIC_READ|GENERIC_WRITE))
    mode = O_RDWR;
  else if (dwDesiredAccess == GENERIC_READ)
    mode = O_RDONLY;
  else if (dwDesiredAccess == GENERIC_WRITE)
    mode = O_WRONLY;
  else
    mode = 0;
  
  /* share mode */
  
  /* security attributes */
  
  /* how to create */
  if (dwCreationDisposition == CREATE_NEW)
    flags = O_CREAT|O_EXCL;
  else if (dwCreationDisposition == CREATE_ALWAYS)
    flags = O_CREAT|O_TRUNC;
  else if (dwCreationDisposition == OPEN_EXISTING)
    flags = O_EXCL;
  else if (dwCreationDisposition == OPEN_ALWAYS)
    flags = O_CREAT;
  else if (dwCreationDisposition == TRUNCATE_EXISTING)
    flags = O_TRUNC;
  else
    flags = 0;
  
  /* file attributes */
  
  filedes = open (lpFileName, flags, mode);
  if (filedes >= 0)
    return (HANDLE)filedes;
  else
    return INVALID_HANDLE_VALUE;
}

static BOOL WINAPI fd32_imp__FlushFileBuffers(HANDLE hFile)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] FlushFileBuffers: %lx\n", (DWORD)hFile);
#endif
  /* NOTE: Direct using FD32 system call */
  if (fd32_fflush((int)hFile) == 0)
    return TRUE;
  else
    return FALSE;
}

/* The ReadFile function reads data from a file, starting at the position indicated by the file pointer. After the read operation has been completed, the file pointer is adjusted by the number of bytes actually read, unless the file handle is created with the overlapped attribute. If the file handle is created for overlapped input and output (I/O), the application must adjust the position of the file pointer after the read operation. 
 *
 * Return Values
 * If the function succeeds, the return value is nonzero. 
 * If the return value is nonzero and the number of bytes read is zero, the file pointer was beyond the current end of the file at the time of the read operation. However, if the file was opened with FILE_FLAG_OVERLAPPED and lpOverlapped is not NULL, the return value is FALSE and GetLastError returns ERROR_HANDLE_EOF when the file pointer goes beyond the current end of file.
 * If the function fails, the return value is zero. To get extended error information, call GetLastError. 
 */
static BOOL WINAPI fd32_imp__ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
  ssize_t res;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] ReadFile: %lx %lx %ld\n", (DWORD)hFile, (DWORD)lpBuffer, nNumberOfBytesToRead);
#endif
  res = read((int)hFile, lpBuffer, nNumberOfBytesToRead);
  
  if (res >= 0) {
    *lpNumberOfBytesRead = res;
    return TRUE;
  } else {
    return FALSE;
  }
}

/* The SetEndOfFile function moves the end-of-file (EOF) position for the specified file to the current position of the file pointer. 
 *
 * Return Values
 * If the function succeeds, the return value is nonzero.
 * If the function fails, the return value is zero. To get extended error information, call GetLastError. 
 */
static BOOL WINAPI fd32_imp__SetEndOfFile(HANDLE hFile)
{
  off_t length = lseek((int)hFile, 0, SEEK_CUR);

#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetEndOfFile: %lx\n", (DWORD)hFile);
#endif
  if (length >= 0) {
    if (ftruncate((int)hFile, length) == 0)
      return TRUE;
  }
  return FALSE;
}

/* The SetFilePointer function moves the file pointer of an open file. 
 *
 * Return Values
 * If the SetFilePointer function succeeds, the return value is the low-order doubleword of the new file pointer, and if lpDistanceToMoveHigh is not NULL, the function puts the high-order doubleword of the new file pointer into the LONG pointed to by that parameter. 
 * If the function fails and lpDistanceToMoveHigh is NULL, the return value is 0xFFFFFFFF. To get extended error information, call GetLastError. 
 * If the function fails, and lpDistanceToMoveHigh is non-NULL, the return value is 0xFFFFFFFF and GetLastError will return a value other than NO_ERROR. 
 */
DWORD WINAPI fd32_imp__SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
  /* The move method is compatible with newlib and FD32's FS layer
     #define FILE_BEGIN   0
     #define FILE_CURRENT 1
     #define FILE_END     2 */
  off_t res;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetFilePointer: %lx\n", (DWORD)hFile);
#endif
  /* TODO: Support 64 bits mode of file seek */
  res = lseek((int)hFile, lDistanceToMove, dwMoveMethod);
  if (res >= 0) {
    return res;
  } else {
    return 0xFFFFFFFF;
  }
}

/* The WriteFile function writes data to a file and is designed for both synchronous and asynchronous operation. The function starts writing data to the file at the position indicated by the file pointer. After the write operation has been completed, the file pointer is adjusted by the number of bytes actually written, except when the file is opened with FILE_FLAG_OVERLAPPED. If the file handle was created for overlapped input and output (I/O), the application must adjust the position of the file pointer after the write operation is finished. 
 *
 * Return Values
 * If the function succeeds, the return value is nonzero.
 * If the function fails, the return value is zero. To get extended error information, call GetLastError. 
 */
static BOOL WINAPI fd32_imp__WriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
  ssize_t res;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] WriteFile: %lx %lx %ld\n", (DWORD)hFile, (DWORD)lpBuffer, nNumberOfBytesToWrite);
  if (hFile == (HANDLE)1)
    fd32_log_printf("[WINB] buf: %x %x\n", ((BYTE *)lpBuffer)[0], ((BYTE *)lpBuffer)[1]);
#endif
  res = write((int)hFile, lpBuffer, nNumberOfBytesToWrite);
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] res: %d\n", res);
#endif
  if (res >= 0) {
    *lpNumberOfBytesWritten = res;
    return TRUE;
  } else {
    return FALSE;
  }
}


/* ______________________________ *\
 *|                              |*
 *|      Console Functions       |*
\*|______________________________|*/

static HANDLE WINAPI fd32_imp__CreateConsoleScreenBuffer(DWORD dwDesiredAccess, DWORD dwShareMode, SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] CreateConsoleScreenBuffer: %lx %lx Scrbuf at %lx\n", dwDesiredAccess, dwShareMode, (DWORD)lpScreenBufferData);
#endif
  return 0;
}

static BOOL WINAPI fd32_imp__FillConsoleOutputAttribute(HANDLE hConsoleOutput, WORD wAttribute, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfAttrsWritten)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] FillConsoleOutputAttribute: %lx %d %ld (%d %d)\n", (DWORD)hConsoleOutput, wAttribute, nLength, dwWriteCoord.X, dwWriteCoord.Y);
#endif
  return FALSE;
}

static BOOL WINAPI fd32_imp__FillConsoleOutputCharacterA(HANDLE hConsoleOutput, CHAR cCharacter, DWORD nLength, COORD dwWriteCoord, LPDWORD lpNumberOfCharsWritten)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] FillConsoleOutputCharacterA: %lx %c %ld (%d %d)\n", (DWORD)hConsoleOutput, cCharacter, nLength, dwWriteCoord.X, dwWriteCoord.Y);
#endif
  return FALSE;
}

static BOOL WINAPI fd32_imp__FlushConsoleInputBuffer(HANDLE hConsoleInput)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] FlushConsoleInputBuffer: %lx\n", (DWORD)hConsoleInput);
#endif
  return fd32_imp__FlushFileBuffers(hConsoleInput);
}

static BOOL WINAPI fd32_imp__GetConsoleCursorInfo(HANDLE hConsoleOutput, PCONSOLE_CURSOR_INFO lpConsoleCursorInfo)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetConsoleCursorInfo: %lx (%ld %d)\n", (DWORD)hConsoleOutput, lpConsoleCursorInfo->dwSize, lpConsoleCursorInfo->bVisible);
#endif
  return TRUE;
}

static BOOL WINAPI fd32_imp__GetConsoleMode(HANDLE hConsoleHandle, LPDWORD lpMode)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GetConsoleMode: %lx\n", (DWORD)hConsoleHandle);
#endif
  *lpMode = ENABLE_LINE_INPUT|ENABLE_ECHO_INPUT; /* WRAP_AT_EOL */
  return TRUE;
}

static BOOL WINAPI fd32_imp__GetConsoleScreenBufferInfo(HANDLE hConsoleOutput, PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo)
{
  /* TODO: Check if the display window size is correct */
  lpConsoleScreenBufferInfo->dwSize.X = 80;
  lpConsoleScreenBufferInfo->dwSize.Y = 25;
  lpConsoleScreenBufferInfo->wAttributes = FOREGROUND_BLUE|FOREGROUND_GREEN|FOREGROUND_RED;
  lpConsoleScreenBufferInfo->srWindow.Left = 0;
  lpConsoleScreenBufferInfo->srWindow.Top = 0;
  lpConsoleScreenBufferInfo->srWindow.Right = 79;
  lpConsoleScreenBufferInfo->srWindow.Bottom = 24;
  lpConsoleScreenBufferInfo->dwMaximumWindowSize.X = 80;
  lpConsoleScreenBufferInfo->dwMaximumWindowSize.Y = 25;
  return TRUE;
}

static DWORD WINAPI fd32_imp__GetConsoleTitleA(LPSTR lpConsoleTitle, DWORD nSize)
{
  /* TODO: Set the title to the program's name */
  if (lpConsoleTitle != NULL)
    return snprintf(lpConsoleTitle, nSize, "FD32-CONSOLE");
  else
    return 0;
}

COORD WINAPI fd32_imp__GetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
  COORD size = {80, 25};
  return size;
}

BOOL WINAPI fd32_imp__GetNumberOfConsoleMouseButtons(LPDWORD lpNumberOfMouseButtons)
{
  *lpNumberOfMouseButtons = 2;
  return TRUE;
}

/* The GetStdHandle function returns a handle for the standard input, standard output, or standard error device. 
 *
 * Return Values
 * If the function succeeds, the return value is a handle of the specified device.
 * If the function fails, the return value is the INVALID_HANDLE_VALUE flag. To get extended error information, call GetLastError. 
 */
static HANDLE WINAPI fd32_imp__GetStdHandle(DWORD nStdHandle)
{
  HANDLE handle;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] GetStdHandle: %lx\n", nStdHandle);
#endif

  switch(nStdHandle)
  {
    case STD_INPUT_HANDLE:
      handle = (HANDLE)0;
      break;
    case STD_OUTPUT_HANDLE:
      handle = (HANDLE)1;
      break;
    case STD_ERROR_HANDLE:
      handle = (HANDLE)2;
      break;
    default:
      handle = INVALID_HANDLE_VALUE;
      break;
  }
  
  return handle;
}

static BOOL WINAPI fd32_imp__ReadConsoleOutputA(HANDLE hConsoleOutput, PCHAR_INFO lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, PSMALL_RECT lpReadRegion)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] ReadConsoleOutputA: %lx\n", (DWORD)hConsoleOutput);
#endif
  return TRUE;
}

static BOOL WINAPI fd32_imp__ScrollConsoleScreenBufferA(HANDLE hConsoleOutput, CONST SMALL_RECT *lpScrollRectangle, CONST SMALL_RECT *lpClipRectangle, COORD dwDestinationOrigin, CONST CHAR_INFO *lpFill)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] ScrollConsoleScreenBufferA: %lx\n", (DWORD)hConsoleOutput);
#endif
  /* NOTE: fake function return true */
  return TRUE;
}

static BOOL WINAPI fd32_imp__SetConsoleActiveScreenBuffer(HANDLE hConsoleOutput)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetConsoleActiveScreenBuffer: %lx\n", (DWORD)hConsoleOutput);
#endif
  /* NOTE: Support only one screen buffer return true directly */
  return TRUE;
}

static BOOL WINAPI fd32_imp__SetConsoleCursorInfo(HANDLE hConsoleOutput, CONST CONSOLE_CURSOR_INFO *lpConsoleCursorInfo)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetConsoleCursorInfo: %lx (%ld %d)\n", (DWORD)hConsoleOutput, lpConsoleCursorInfo->dwSize, lpConsoleCursorInfo->bVisible);
#endif
  return TRUE;
}

static BOOL WINAPI fd32_imp__SetConsoleCursorPosition(HANDLE hConsoleOutput, COORD dwCursorPosition)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetConsoleCursorPosition: %lx (%d %d)\n", (DWORD)hConsoleOutput, dwCursorPosition.X, dwCursorPosition.Y);
#endif
  return TRUE;
}

static BOOL WINAPI fd32_imp__SetConsoleMode(HANDLE hConsoleHandle, DWORD dwMode)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetConsoleMode: %lx, %lx\n", (DWORD)hConsoleHandle, dwMode);
#endif
  return TRUE;
}

static BOOL WINAPI fd32_imp__SetConsoleScreenBufferSize(HANDLE hConsoleOutput, COORD dwSize)
{
  /* TODO: Support screen buffer size changing */
  return FALSE;
}

static BOOL WINAPI fd32_imp__SetConsoleTitleA(LPCSTR lpConsoleTitle)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetConsoleTitleA: %s\n", lpConsoleTitle);
#endif
  return TRUE;
}

static BOOL WINAPI fd32_imp__SetConsoleWindowInfo(HANDLE hConsoleOutput, BOOL bAbsolute, CONST SMALL_RECT *lpConsoleWindow)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SetConsoleWindowInfo: %lx, %x (%d, %d, %d, %d)\n", (DWORD)hConsoleOutput, bAbsolute, lpConsoleWindow->Left, lpConsoleWindow->Top, lpConsoleWindow->Right, lpConsoleWindow->Bottom);
#endif
  /* NOTE: fake function return true */
  return TRUE;
}

static BOOL WINAPI fd32_imp__WriteConsoleOutputA(HANDLE hConsoleOutput, CONST CHAR_INFO *lpBuffer, COORD dwBufferSize, COORD dwBufferCoord, PSMALL_RECT lpWriteRegion)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] WriteConsoleOutputA: %lx\n", (DWORD)hConsoleOutput);
#endif
  return TRUE;
}

static char kernel32_name[] = "kernel32.dll";
static struct symbol kernel32_symarray[] = {
  {"AddAtomA",                    (uint32_t)fd32_imp__AddAtomA},
  {"CloseHandle",                 (uint32_t)fd32_imp__CloseHandle},
  {"CreateConsoleScreenBuffer",   (uint32_t)fd32_imp__CreateConsoleScreenBuffer},
  {"CreateFileA",                 (uint32_t)fd32_imp__CreateFileA},
  {"ExitProcess",                 (uint32_t)fd32_imp__ExitProcess},
  {"FillConsoleOutputAttribute",  (uint32_t)fd32_imp__FillConsoleOutputAttribute},
  {"FillConsoleOutputCharacterA", (uint32_t)fd32_imp__FillConsoleOutputCharacterA},
  {"FindAtomA",                   (uint32_t)fd32_imp__FindAtomA},
  {"FlushConsoleInputBuffer",     (uint32_t)fd32_imp__FlushConsoleInputBuffer},
  {"FlushFileBuffers",            (uint32_t)fd32_imp__FlushFileBuffers},
  {"GetAtomNameA",                (uint32_t)fd32_imp__GetAtomNameA},
  {"GetCommandLineA",             (uint32_t)fd32_imp__GetCommandLineA},
  {"GetConsoleCursorInfo",        (uint32_t)fd32_imp__GetConsoleCursorInfo},
  {"GetConsoleMode",              (uint32_t)fd32_imp__GetConsoleMode},
  {"GetConsoleScreenBufferInfo",  (uint32_t)fd32_imp__GetConsoleScreenBufferInfo},
  {"GetConsoleTitleA",            (uint32_t)fd32_imp__GetConsoleTitleA},
  {"GetCurrentProcessId",         (uint32_t)fd32_imp__GetCurrentProcessId},
  {"GetCurrentThreadId",          (uint32_t)fd32_imp__GetCurrentThreadId},
  {"GetEnvironmentStringsA",      (uint32_t)fd32_imp__GetEnvironmentStringsA},
  {"GetLargestConsoleWindowSize", (uint32_t)fd32_imp__GetLargestConsoleWindowSize},
  {"GetLastError",                (uint32_t)fd32_imp__GetLastError},
  {"GetModuleHandleA",            (uint32_t)fd32_imp__GetModuleHandleA},
  {"GetNumberOfConsoleMouseButtons", (uint32_t)fd32_imp__GetNumberOfConsoleMouseButtons},
  {"GetStartupInfoA",             (uint32_t)fd32_imp__GetStartupInfoA},
  {"GetStdHandle",                (uint32_t)fd32_imp__GetStdHandle},
  {"GetSystemInfo",               (uint32_t)fd32_imp__GetSystemInfo},
  {"GetSystemTimeAsFileTime",     (uint32_t)fd32_imp__GetSystemTimeAsFileTime},
  {"GetTickCount",                (uint32_t)fd32_imp__GetTickCount},
  {"GetVersionExA",               (uint32_t)fd32_imp__GetVersionExA},
  {"GlobalAlloc",                 (uint32_t)fd32_imp__GlobalAlloc},
  {"GlobalFree",                  (uint32_t)fd32_imp__GlobalFree},
  {"LocalAlloc",                  (uint32_t)fd32_imp__LocalAlloc},
  {"LocalFree",                   (uint32_t)fd32_imp__LocalFree},
  {"PeekNamedPipe",               (uint32_t)fd32_imp__PeekNamedPipe},
  {"QueryPerformanceCounter",     (uint32_t)fd32_imp__QueryPerformanceCounter},
  {"ReadConsoleOutputA",          (uint32_t)fd32_imp__ReadConsoleOutputA},
  {"ReadFile",                    (uint32_t)fd32_imp__ReadFile},
  {"ScrollConsoleScreenBufferA",  (uint32_t)fd32_imp__ScrollConsoleScreenBufferA},
  {"SetConsoleActiveScreenBuffer",(uint32_t)fd32_imp__SetConsoleActiveScreenBuffer},
  {"SetConsoleCursorInfo",        (uint32_t)fd32_imp__SetConsoleCursorInfo},
  {"SetConsoleCursorPosition",    (uint32_t)fd32_imp__SetConsoleCursorPosition},
  {"SetConsoleMode",              (uint32_t)fd32_imp__SetConsoleMode},
  {"SetConsoleTitleA",            (uint32_t)fd32_imp__SetConsoleTitleA},
  {"SetConsoleScreenBufferSize",  (uint32_t)fd32_imp__SetConsoleScreenBufferSize},
  {"SetConsoleWindowInfo",        (uint32_t)fd32_imp__SetConsoleWindowInfo},
  {"SetEndOfFile",                (uint32_t)fd32_imp__SetEndOfFile},
  {"SetErrorMode",                (uint32_t)fd32_imp__SetErrorMode},
  {"SetFilePointer",              (uint32_t)fd32_imp__SetFilePointer},
  {"SetLastError",                (uint32_t)fd32_imp__SetLastError},
  {"SetUnhandledExceptionFilter", (uint32_t)fd32_imp__SetUnhandledExceptionFilter},
  {"Sleep",                       (uint32_t)fd32_imp__Sleep},
  {"VirtualAlloc",                (uint32_t)fd32_imp__VirtualAlloc},
  {"VirtualFree",                 (uint32_t)fd32_imp__VirtualFree},
  {"WriteConsoleOutputA",         (uint32_t)fd32_imp__WriteConsoleOutputA},
  {"WriteFile",                   (uint32_t)fd32_imp__WriteFile},
};
static uint32_t kernel32_symnum = sizeof(kernel32_symarray)/sizeof(struct symbol);

void install_kernel32(void)
{
  add_dll_table(kernel32_name, HANDLE_OF_KERNEL32, kernel32_symnum, kernel32_symarray);
}

