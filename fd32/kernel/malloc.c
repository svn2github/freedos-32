/* The FreeDOS-32 Kernel Port of dlmalloc and Buddy Memory Allocator
 * Copyright (C) 2005  Nils Labugt
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
  This file is derived from version 2.8.2 of the popular memory allocator
  by Doug Lea, which was release by him to the public domain, see below.
*/

/*
ESTOPPEL:
  I, Nils Labugt, find strong reason to believe that a copyright holder in
  an operating system do not get any rights to applications that use its
  Applications Programmable Interface trough copyright law, but to remove
  any doubt, I hereby waiver any such rights. If any court should find
  otherwise, then this notice should additionally count as an exception to
  the GPL for such use of this file. Note that this waiver or license
  modification only cover my and my successor' in interest rights, and not
  those of other copyright holders of any work in which this file is
  included. Any future copyright holder in this file, who do not want to be
  bound by this waiver, should remove this notice.
*/

/*
Non-exhaustive todo list:

- add page slab allocator
- add more comments
- Lea's comments need more editing
- mem_get_region
- todos for the buddy: see that part of the file
- add the rest of the high level interface code (some in mem.c?)
- add code for passing memory requests to other allocators (*optional*)
  if there is no more free zones
- rewrite and extend some of the initialization code
- simple wait-free allocator (non-urgent)
- code to add memory that have not been allocated (dynalink during boot)
- remove the magic?
- make it compile with DEBUG and without INSECURE
- get the locking in order
- general cleanup
- allocation of DMA memory and DOS memory (probably in mem.c)
- etc

  Comment on foot fields: Foot fields are used to store an identification
  number, typically the process ID, or alternatively, a mspace pointer
  (pointer to the memory state for the memory space). In the former case,
  cleanup after a process (memory leaks) is facilitated. In the latter case,
  supplying a mspace pointer is unnecessary when freeing a block. The lower
  two bits of the identification number is reserved, and should be 0.
  Therefore the mspace pointer and the the process ID (which supposed to be
  an address) should be properly aligned. 
*/
/*=========================================================================*/

/*
  This is a version (aka dlmalloc) of malloc/free/realloc written by
  Doug Lea and released to the public domain, as explained at
  http://creativecommons.org/licenses/publicdomain.  Send questions,
  comments, complaints, performance data, etc to dl@cs.oswego.edu
 
* Version 2.8.2 Sun Jun 12 16:05:14 2005  Doug Lea  (dl at gee)
 
   Note: There may be an updated version of this malloc obtainable at
           ftp://gee.cs.oswego.edu/pub/misc/malloc.c
         Check before installing!
 
* Quickstart
 
  This library is all in one file to simplify the most common usage:
  ftp it, compile it (-O3), and link it into another program. All of
  the compile-time options default to reasonable values for use on
  most platforms.  You might later want to step through various
  compile-time and dynamic tuning options.
 
  For convenience, an include file for code using this malloc is at:
     ftp://gee.cs.oswego.edu/pub/misc/malloc-2.8.0.h
  You don't really need this .h file unless you call functions not
  defined in your system include files.  The .h file contains only the
  excerpts from this file needed for using this malloc on ANSI C/C++
  systems, so long as you haven't changed compile-time options about
  naming and tuning parameters.  If you do, then you can create your
  own malloc.h that does include all settings by cutting at the point
  indicated below. Note that you may already by default be using a C
  library containing a malloc that is based on some version of this
  malloc (for example in linux). You might still want to use the one
  in this file to customize settings or to avoid overheads associated
  with library versions.
 
* Vital statistics:
 
  Supported pointer/size_t representation:       4 or 8 bytes
       size_t MUST be an unsigned type of the same width as
       pointers. (If you are using an ancient system that declares
       size_t as a signed type, or need it to be a different width
       than pointers, you can use a previous release of this malloc
       (e.g. 2.7.2) supporting these.)
 
  Alignment:                                     8 bytes (default)
       This suffices for nearly all current machines and C compilers.
       However, you can define MALLOC_ALIGNMENT to be wider than this
       if necessary (up to 128bytes), at the expense of using more space.
 
  Minimum overhead per allocated chunk:   4 or  8 bytes (if 4byte sizes)
                                          8 or 16 bytes (if 8byte sizes)
       Each malloced chunk has a hidden word of overhead holding size
       and status information, and additional cross-check word
       if FOOTERS is defined.
 
  Minimum allocated size: 4-byte ptrs:  16 bytes    (including overhead)
                          8-byte ptrs:  32 bytes    (including overhead)
 
       Even a request for zero bytes (i.e., malloc(0)) returns a
       pointer to something of the minimum allocatable size.
       The maximum overhead wastage (i.e., number of extra bytes
       allocated than were requested in malloc) is less than or equal
       to the minimum size, except for requests >= mmap_threshold that
       are serviced via mmap(), where the worst case wastage is about
       32 bytes plus the remainder from a system page (the minimal
       mmap unit); typically 4096 or 8192 bytes.
 
  Security: static-safe; optionally more or less
       The "security" of malloc refers to the ability of malicious
       code to accentuate the effects of errors (for example, freeing
       space that is not currently malloc'ed or overwriting past the
       ends of chunks) in code that calls malloc.  This malloc
       guarantees not to modify any memory locations below the base of
       heap, i.e., static variables, even in the presence of usage
       errors.  The routines additionally detect most improper frees
       and reallocs.  All this holds as long as the static bookkeeping
       for malloc itself is not corrupted by some other means.  This
       is only one aspect of security -- these checks do not, and
       cannot, detect all possible programming errors.
 
       If FOOTERS is defined nonzero, then each allocated chunk
       carries an additional check word to verify that it was malloced
       from its space.  These check words are the same within each
       execution of a program using malloc, but differ across
       executions, so externally crafted fake chunks cannot be
       freed. This improves security by rejecting frees/reallocs that
       could corrupt heap memory, in addition to the checks preventing
       writes to statics that are always on.  This may further improve
       security at the expense of time and space overhead.  (Note that
       FOOTERS may also be worth using with MSPACES.)
 
       By default detected errors cause the program to abort (calling
       "abort()"). You can override this to instead proceed past
       errors by defining PROCEED_ON_ERROR.  In this case, a bad free
       has no effect, and a malloc that encounters a bad address
       caused by user overwrites will ignore the bad address by
       dropping pointers and indices to all known memory. This may
       be appropriate for programs that should continue if at all
       possible in the face of programming errors, although they may
       run out of memory because dropped memory is never reclaimed.
 
       If you don't like either of these options, you can define
       CORRUPTION_ERROR_ACTION and USAGE_ERROR_ACTION to do anything
       else. And if if you are sure that your program using malloc has
       no errors or vulnerabilities, you can define INSECURE to 1,
       which might (or might not) provide a small performance improvement.
 
  Thread-safety: NOT thread-safe unless USE_LOCKS defined
       When USE_LOCKS is defined, each public call to malloc, free,
       etc is surrounded with either a pthread mutex or a win32
       spinlock (depending on WIN32). This is not especially fast, and
       can be a major bottleneck.  It is designed only to provide
       minimal protection in concurrent environments, and to provide a
       basis for extensions.  If you are using malloc in a concurrent
       program, consider instead using ptmalloc, which is derived from
       a version of this malloc. (See http://www.malloc.de).
 
 
* Overview of algorithms
 
  This is not the fastest, most space-conserving, most portable, or
  most tunable malloc ever written. However it is among the fastest
  while also being among the most space-conserving, portable and
  tunable.  Consistent balance across these factors results in a good
  general-purpose allocator for malloc-intensive programs.
 
  In most ways, this malloc is a best-fit allocator. Generally, it
  chooses the best-fitting existing chunk for a request, with ties
  broken in approximately least-recently-used order. (This strategy
  normally maintains low fragmentation.) However, for requests less
  than 256bytes, it deviates from best-fit when there is not an
  exactly fitting available chunk by preferring to use space adjacent
  to that used for the previous small request, as well as by breaking
  ties in approximately most-recently-used order. (These enhance
  locality of series of small allocations.)  And for very large requests
  (>= 256Kb by default), it relies on system memory mapping
  facilities, if supported.  (This helps avoid carrying around and
  possibly fragmenting memory used only for large chunks.)
 
  All operations (except malloc_stats and mallinfo) have execution
  times that are bounded by a constant factor of the number of bits in
  a size_t, not counting any clearing in calloc or copying in realloc,
  or actions surrounding MORECORE and MMAP that have times
  proportional to the number of non-contiguous regions returned by
  system allocation routines, which is often just 1.
 
  The implementation is not very modular and seriously overuses
  macros. Perhaps someday all C compilers will do as good a job
  inlining modular code as can now be done by brute-force expansion,
  but now, enough of them seem not to.
 
  Some compilers issue a lot of warnings about code that is
  dead/unreachable only on some platforms, and also about intentional
  uses of negation on unsigned types. All known cases of each can be
  ignored.
 
  For a longer but out of date high-level description, see
     http://gee.cs.oswego.edu/dl/html/malloc.html
 
* MSPACES
  If MSPACES is defined, then in addition to malloc, free, etc.,
  this file also defines mspace_malloc, mspace_free, etc. These
  are versions of malloc routines that take an "mspace" argument
  obtained using create_mspace, to control all internal bookkeeping.
  If ONLY_MSPACES is defined, only these versions are compiled.
  So if you would like to use this allocator for only some allocations,
  and your system malloc for others, you can compile with
  ONLY_MSPACES and then do something like...
    static mspace mymspace = create_mspace(0,0); // for example
    #define mymalloc(bytes)  mspace_malloc(mymspace, bytes)
 
  (Note: If you only need one instance of an mspace, you can instead
  use "USE_DL_PREFIX" to relabel the global malloc.)
 
  You can similarly create thread-local allocators by storing
  mspaces as thread-locals. For example:
    static __thread mspace tlms = 0;
    void*  tlmalloc(size_t bytes) {
      if (tlms == 0) tlms = create_mspace(0, 0);
      return mspace_malloc(tlms, bytes);
    }
    void  tlfree(void* mem) { mspace_free(tlms, mem); }
 
  Unless FOOTERS is defined, each mspace is completely independent.
  You cannot allocate from one and free to another (although
  conformance is only weakly checked, so usage errors are not always
  caught). If FOOTERS is defined, then each chunk carries around a tag
  indicating its originating mspace, and frees are directed to their
  originating spaces.
 
 -------------------------  Compile-time options ---------------------------
 
 
MALLOC_ALIGNMENT         default: 8
  Controls the minimum alignment for malloc'ed chunks.  It must be a
  power of two and at least 8, even on machines for which smaller
  alignments would suffice. It may be defined as larger than this
  though. Note however that code and data structures are optimized for
  the case of 8-byte alignment.
 
 
USE_LOCKS                default: 0 (false)
  Causes each call to each public routine to be surrounded with
  pthread or WIN32 mutex lock/unlock. (If set true, this can be
  overridden on a per-mspace basis for mspace versions.)
 
 
INSECURE                 default: 0
  If true, omit checks for usage errors and heap space overwrites.
 
 
ABORT                    default: defined as abort()
  Defines how to abort on failed checks.  On most systems, a failed
  check cannot die with an "assert" or even print an informative
  message, because the underlying print routines in turn call malloc,
  which will fail again.  Generally, the best policy is to simply call
  abort(). It's not very useful to do more than this because many
  errors due to overwriting will show up as address faults (null, odd
  addresses etc) rather than malloc-triggered checks, so will also
  abort.  Also, most compilers know that abort() does not return, so
  can better optimize code conditionally calling it.
 
PROCEED_ON_ERROR           default: defined as 0 (false)
  Controls whether detected bad addresses cause them to bypassed
  rather than aborting. If set, detected bad arguments to free and
  realloc are ignored. And all bookkeeping information is zeroed out
  upon a detected overwrite of freed heap space, thus losing the
  ability to ever return it from malloc again, but enabling the
  application to proceed. If PROCEED_ON_ERROR is defined, the
  static variable malloc_corruption_error_count is compiled in
  and can be examined to see if errors have occurred. This option
  generates slower code than the default abort policy.
 
DEBUG                    default: NOT defined
  The DEBUG setting is mainly intended for people trying to modify
  this code or diagnose problems when porting to new platforms.
  However, it may also be able to better isolate user errors than just
  using runtime checks.  The assertions in the check routines spell
  out in more detail the assumptions and invariants underlying the
  algorithms.  The checking is fairly extensive, and will slow down
  execution noticeably. Calling malloc_stats or mallinfo with DEBUG
  set will attempt to check every non-mmapped allocated and free chunk
  in the course of computing the summaries.
 
ABORT_ON_ASSERT_FAILURE   default: defined as 1 (true)
  Debugging assertion failures can be nearly impossible if your
  version of the assert macro causes malloc to be called, which will
  lead to a cascade of further failures, blowing the runtime stack.
  ABORT_ON_ASSERT_FAILURE cause assertions failures to call abort(),
  which will usually make debugging easier.
 
MALLOC_FAILURE_ACTION     default: sets errno to ENOMEM, or no-op on win32
  The action to take before "return 0" when malloc fails to be able to
  return memory because there is none available.
 
 
USE_BUILTIN_FFS            default: 0 (i.e., not used)
  Causes malloc to use the builtin ffs() function to compute indices.
  Some compilers may recognize and intrinsify ffs to be faster than the
  supplied C version. Also, the case of x86 using gcc is special-cased
  to an asm instruction, so is already as fast as it can be, and so
  this setting has no effect. (On most x86s, the asm version is only
  slightly faster than the C version.)
 
 
USE_DEV_RANDOM             default: 0 (i.e., not used)
  Causes malloc to use /dev/random to initialize secure magic seed for
  stamping footers. Otherwise, the current time is used.
 
 
MALLINFO_FIELD_TYPE        default: size_t
  The type of the fields in the mallinfo struct. This was originally
  defined as "int" in SVID etc, but is more usefully defined as
  size_t. The value is used only if  HAVE_USR_INCLUDE_MALLOC_H is not set
 
 
*/

/* **FIXME** move this somewhere else and/or include the appropriate files */
typedef unsigned long size_t;
struct malloc_state;
typedef struct malloc_state* mstate;
/*
  mspace is an opaque type representing an independent
  region of space that supports mspace_malloc, etc.
*/
typedef void* mspace;
#include <ll/assert.h>
#include <ll/i386/mem.h>
void abort(void);
#define INSECURE 1
#include <ll/stdlib.h>
#define MF_USE_ZONE_ALLOC 1
#define FOOT_IS_MEMSPACE 1
typedef void* memowner;
void* mspace_malloc(memowner id, mspace space, size_t bytes);
static void* zone_alloc_dl(mstate ms, void* footid, size_t req);
void* DMA_malloc(memowner id, mspace ms, size_t bytes);
#define MALLOC_FAILURE_ACTION
#define SEG_FLAG_ORIGINAL 1
#define SEG_FLAG_BUDDY 2
#define SEG_FLAG_DL 4
#define SEG_FLAG_ZONE 8
#define SEG_FLAG_DESTROY_ORIGINAL 16
#define ENOMEM 0;

#include "malloc.h"
/*#include <sys/types.h> */ /* For size_t */

#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT (8U)
#endif
#ifndef ABORT
#define ABORT  abort()
#endif
#ifndef ABORT_ON_ASSERT_FAILURE
#define ABORT_ON_ASSERT_FAILURE 1
#endif
#ifndef PROCEED_ON_ERROR
#define PROCEED_ON_ERROR 0
#endif
#ifndef USE_LOCKS
#define USE_LOCKS 0
#endif
#ifndef INSECURE
#define INSECURE 0
#endif
#ifndef MALLOC_FAILURE_ACTION
#define MALLOC_FAILURE_ACTION  errno = ENOMEM;
#endif
#ifndef USE_BUILTIN_FFS
#define USE_BUILTIN_FFS 0
#endif
#ifndef USE_DEV_RANDOM
#define USE_DEV_RANDOM 0
#endif
#ifndef NO_MALLINFO
#define NO_MALLINFO 0
#endif
#ifndef MALLINFO_FIELD_TYPE
#define MALLINFO_FIELD_TYPE size_t
#endif


/* ------------------------ Mallinfo declarations ------------------------ */

#if !NO_MALLINFO
/*
  This version of malloc supports the standard SVID/XPG mallinfo
  routine that returns a struct containing usage properties and
  statistics. It should work on any system that has a
  /usr/include/malloc.h defining struct mallinfo.  The main
  declaration needed is the mallinfo struct that is returned (by-copy)
  by mallinfo().  The malloinfo struct contains a bunch of fields that
  are not even meaningful in this version of malloc.  These fields are
  are instead filled by mallinfo() with other numbers that might be of
  interest.
 
  HAVE_USR_INCLUDE_MALLOC_H should be set if you have a
  /usr/include/malloc.h file that includes a declaration of struct
  mallinfo.  If so, it is included; else a compliant version is
  declared below.  These must be precisely the same for mallinfo() to
  work.  The original SVID version of this struct, defined on most
  systems with mallinfo, declares all fields as ints. But some others
  define as unsigned long. If your system defines the fields using a
  type of different width than listed here, you MUST #include your
  system version and #define HAVE_USR_INCLUDE_MALLOC_H.
*/

/* #define HAVE_USR_INCLUDE_MALLOC_H */

#ifdef HAVE_USR_INCLUDE_MALLOC_H
#include "/usr/include/malloc.h"
#else

struct mallinfo
{
  MALLINFO_FIELD_TYPE arena;    /* non-mmapped space allocated from system */
  MALLINFO_FIELD_TYPE ordblks;  /* number of free chunks */
  MALLINFO_FIELD_TYPE smblks;   /* always 0 */
  MALLINFO_FIELD_TYPE hblks;    /* always 0 */
  MALLINFO_FIELD_TYPE hblkhd;   /* space in mmapped regions */
  MALLINFO_FIELD_TYPE usmblks;  /* maximum total allocated space */
  MALLINFO_FIELD_TYPE fsmblks;  /* always 0 */
  MALLINFO_FIELD_TYPE uordblks; /* total allocated space */
  MALLINFO_FIELD_TYPE fordblks; /* total free space */
  MALLINFO_FIELD_TYPE keepcost; /* releasable (via malloc_trim) space */
};

#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if 0

  /* ------------------- Declarations of public routines ------------------- */

#ifndef USE_DL_PREFIX
#define dlcalloc               calloc
#define dlfree                 free
#define dlmalloc               malloc
#define dlmemalign             memalign
#define dlrealloc              realloc
#define dlvalloc               valloc
#define dlpvalloc              pvalloc
#define dlmallinfo             mallinfo
#define dlmallopt              mallopt
#define dlmalloc_trim          malloc_trim
#define dlmalloc_stats         malloc_stats
#define dlmalloc_usable_size   malloc_usable_size
#define dlmalloc_footprint     malloc_footprint
#define dlindependent_calloc   independent_calloc
#define dlindependent_comalloc independent_comalloc
#endif


  /*
    malloc(size_t n)
    Returns a pointer to a newly allocated chunk of at least n bytes, or
    null if no space is available, in which case errno is set to ENOMEM
    on ANSI C systems.

    If n is zero, malloc returns a minimum-sized chunk. (The minimum
    size is 16 bytes on most 32bit systems, and 32 bytes on 64bit
    systems.)  Note that size_t is an unsigned type, so calls with
    arguments that would be negative if signed are interpreted as
    requests for huge amounts of space, which will often fail. The
    maximum supported value of n differs across systems, but is in all
    cases less than the maximum representable value of a size_t.
  */
  void* dlmalloc(size_t);

  /*
    free(void* p)
    Releases the chunk of memory pointed to by p, that had been previously
    allocated using malloc or a related routine such as realloc.
    It has no effect if p is null. If p was not malloced or already
    freed, free(p) will by default cuase the current program to abort.
  */
  void  dlfree(void*);

  /*
    calloc(size_t n_elements, size_t element_size);
    Returns a pointer to n_elements * element_size bytes, with all locations
    set to zero.
  */
  void* dlcalloc(size_t, size_t);

  /*
    realloc(void* p, size_t n)
    Returns a pointer to a chunk of size n that contains the same data
    as does chunk p up to the minimum of (n, p's size) bytes, or null
    if no space is available.

    The returned pointer may or may not be the same as p. The algorithm
    prefers extending p in most cases when possible, otherwise it
    employs the equivalent of a malloc-copy-free sequence.

    If p is null, realloc is equivalent to malloc.

    If space is not available, realloc returns null, errno is set (if on
    ANSI) and p is NOT freed.

    if n is for fewer bytes than already held by p, the newly unused
    space is lopped off and freed if possible.  realloc with a size
    argument of zero (re)allocates a minimum-sized chunk.

    The old unix realloc convention of allowing the last-free'd chunk
    to be used as an argument to realloc is not supported.
  */

  void* dlrealloc(void*, size_t);

  /*
    memalign(size_t alignment, size_t n);
    Returns a pointer to a newly allocated chunk of n bytes, aligned
    in accord with the alignment argument.

    The alignment argument should be a power of two. If the argument is
    not a power of two, the nearest greater power is used.
    8-byte alignment is guaranteed by normal malloc calls, so don't
    bother calling memalign with an argument of 8 or less.

    Overreliance on memalign is a sure way to fragment space.
  */
  void* dlmemalign(size_t, size_t);

  /*
    valloc(size_t n);
    Equivalent to memalign(pagesize, n), where pagesize is the page
    size of the system. If the pagesize is unknown, 4096 is used.
  */
  void* dlvalloc(size_t);

  /*
    mallopt(int parameter_number, int parameter_value)
    Sets tunable parameters The format is to provide a
    (parameter-number, parameter-value) pair.  mallopt then sets the
    corresponding parameter to the argument value if it can (i.e., so
    long as the value is meaningful), and returns 1 if successful else
    0.  SVID/XPG/ANSI defines four standard param numbers for mallopt,
    normally defined in malloc.h.  None of these are use in this malloc,
    so setting them has no effect. But this malloc also supports other
    options in mallopt. See below for details.  Briefly, supported
    parameters are as follows (listed defaults are for "typical"
    configurations).

    Symbol            param #  default    allowed param values
    M_TRIM_THRESHOLD     -1   2*1024*1024   any   (-1U disables trimming)
    M_GRANULARITY        -2     page size   any power of 2 >= page size
    M_MMAP_THRESHOLD     -3      256*1024   any   (or 0 if no MMAP support)
  */
  int dlmallopt(int, int);

  /*
    malloc_footprint();
    Returns the number of bytes obtained from the system.  The total
    number of bytes allocated by malloc, realloc etc., is less than this
    value. Unlike mallinfo, this function returns only a precomputed
    result, so can be called frequently to monitor memory consumption.
    Even if locks are otherwise defined, this function does not use them,
    so results might not be up to date.
  */
  size_t dlmalloc_footprint();

#if !NO_MALLINFO
  /*
    mallinfo()
    Returns (by copy) a struct containing various summary statistics:

    arena:     current total non-mmapped bytes allocated from system
    ordblks:   the number of free chunks
    smblks:    always zero.
    hblks:     current number of mmapped regions
    hblkhd:    total bytes held in mmapped regions
    usmblks:   the maximum total allocated space. This will be greater
                  than current total if trimming has occurred.
    fsmblks:   always zero
    uordblks:  current total allocated space (normal or mmapped)
    fordblks:  total free space
    keepcost:  the maximum number of bytes that could ideally be released
                 back to system via malloc_trim. ("ideally" means that
                 it ignores page restrictions etc.)

    Because these fields are ints, but internal bookkeeping may
    be kept as longs, the reported values may wrap around zero and
    thus be inaccurate.
  */
  struct mallinfo dlmallinfo(void);
#endif

  /*
    independent_calloc(size_t n_elements, size_t element_size, void* chunks[]);

    independent_calloc is similar to calloc, but instead of returning a
    single cleared space, it returns an array of pointers to n_elements
    independent elements that can hold contents of size elem_size, each
    of which starts out cleared, and can be independently freed,
    realloc'ed etc. The elements are guaranteed to be adjacently
    allocated (this is not guaranteed to occur with multiple callocs or
    mallocs), which may also improve cache locality in some
    applications.

    The "chunks" argument is optional (i.e., may be null, which is
    probably the most typical usage). If it is null, the returned array
    is itself dynamically allocated and should also be freed when it is
    no longer needed. Otherwise, the chunks array must be of at least
    n_elements in length. It is filled in with the pointers to the
    chunks.

    In either case, independent_calloc returns this pointer array, or
    null if the allocation failed.  If n_elements is zero and "chunks"
    is null, it returns a chunk representing an array with zero elements
    (which should be freed if not wanted).

    Each element must be individually freed when it is no longer
    needed. If you'd like to instead be able to free all at once, you
    should instead use regular calloc and assign pointers into this
    space to represent elements.  (In this case though, you cannot
    independently free elements.)

    independent_calloc simplifies and speeds up implementations of many
    kinds of pools.  It may also be useful when constructing large data
    structures that initially have a fixed number of fixed-sized nodes,
    but the number is not known at compile time, and some of the nodes
    may later need to be freed. For example:

    struct Node { int item; struct Node* next; };

    struct Node* build_list() {
      struct Node** pool;
      int n = read_number_of_nodes_needed();
      if (n <= 0) return 0;
      pool = (struct Node**)(independent_calloc(n, sizeof(struct Node), 0);
      if (pool == 0) die();
      // organize into a linked list...
      struct Node* first = pool[0];
      for (i = 0; i < n-1; ++i)
        pool[i]->next = pool[i+1];
      free(pool);     // Can now free the array (or not, if it is needed later)
      return first;
    }
  */
  void** dlindependent_calloc(size_t, size_t, void**);

  /*
    independent_comalloc(size_t n_elements, size_t sizes[], void* chunks[]);

    independent_comalloc allocates, all at once, a set of n_elements
    chunks with sizes indicated in the "sizes" array.    It returns
    an array of pointers to these elements, each of which can be
    independently freed, realloc'ed etc. The elements are guaranteed to
    be adjacently allocated (this is not guaranteed to occur with
    multiple callocs or mallocs), which may also improve cache locality
    in some applications.

    The "chunks" argument is optional (i.e., may be null). If it is null
    the returned array is itself dynamically allocated and should also
    be freed when it is no longer needed. Otherwise, the chunks array
    must be of at least n_elements in length. It is filled in with the
    pointers to the chunks.

    In either case, independent_comalloc returns this pointer array, or
    null if the allocation failed.  If n_elements is zero and chunks is
    null, it returns a chunk representing an array with zero elements
    (which should be freed if not wanted).

    Each element must be individually freed when it is no longer
    needed. If you'd like to instead be able to free all at once, you
    should instead use a single regular malloc, and assign pointers at
    particular offsets in the aggregate space. (In this case though, you
    cannot independently free elements.)

    independent_comallac differs from independent_calloc in that each
    element may have a different size, and also that it does not
    automatically clear elements.

    independent_comalloc can be used to speed up allocation in cases
    where several structs or objects must always be allocated at the
    same time.  For example:

    struct Head { ... }
    struct Foot { ... }

    void send_message(char* msg) {
      int msglen = strlen(msg);
      size_t sizes[3] = { sizeof(struct Head), msglen, sizeof(struct Foot) };
      void* chunks[3];
      if (independent_comalloc(3, sizes, chunks) == 0)
        die();
      struct Head* head = (struct Head*)(chunks[0]);
      char*        body = (char*)(chunks[1]);
      struct Foot* foot = (struct Foot*)(chunks[2]);
      // ...
    }

    In general though, independent_comalloc is worth using only for
    larger values of n_elements. For small values, you probably won't
    detect enough difference from series of malloc calls to bother.

    Overuse of independent_comalloc can increase overall memory usage,
    since it cannot reuse existing noncontiguous small chunks that
    might be available for some of the elements.
  */
  void** dlindependent_comalloc(size_t, size_t*, void**);


  /*
    pvalloc(size_t n);
    Equivalent to valloc(minimum-page-that-holds(n)), that is,
    round up n to nearest pagesize.
   */
  void*  dlpvalloc(size_t);

  /*
    malloc_trim(size_t pad);

    If possible, gives memory back to the system (via negative arguments
    to sbrk) if there is unused memory at the `high' end of the malloc
    pool or in unused MMAP segments. You can call this after freeing
    large blocks of memory to potentially reduce the system-level memory
    requirements of a program. However, it cannot guarantee to reduce
    memory. Under some allocation patterns, some large free blocks of
    memory will be locked between two used chunks, so they cannot be
    given back to the system.

    The `pad' argument to malloc_trim represents the amount of free
    trailing space to leave untrimmed. If this argument is zero, only
    the minimum amount of memory to maintain internal data structures
    will be left. Non-zero arguments can be supplied to maintain enough
    trailing space to service future expected allocations without having
    to re-obtain memory from the system.

    Malloc_trim returns 1 if it actually released any memory, else 0.
  */
  int  dlmalloc_trim(size_t);

  /*
    malloc_usable_size(void* p);

    Returns the number of bytes you can actually use in
    an allocated chunk, which may be more than you requested (although
    often not) due to alignment and minimum size constraints.
    You can use this many bytes without worrying about
    overwriting other allocated objects. This is not a particularly great
    programming practice. malloc_usable_size can be more useful in
    debugging and assertions, for example:

    p = malloc(n);
    assert(malloc_usable_size(p) >= 256);
  */
  size_t dlmalloc_usable_size(void*);

  /*
    malloc_stats();
    Prints on stderr the amount of space obtained from the system (both
    via sbrk and mmap), the maximum amount (which may be more than
    current if malloc_trim and/or munmap got called), and the current
    number of bytes allocated via malloc (or realloc, etc) but not yet
    freed. Note that this is the number of bytes allocated, not the
    number requested. It will be larger than the number requested
    because of alignment and bookkeeping overhead. Because it includes
    alignment wastage as being in use, this figure may be greater than
    zero even when no user-level chunks are allocated.

    The reported current and maximum system memory can be inaccurate if
    a program makes other calls to system memory allocation functions
    (normally sbrk) outside of malloc.

    malloc_stats prints only the most commonly interesting statistics.
    More information can be obtained by calling mallinfo.
  */
  void  dlmalloc_stats();

#endif


#if 0
  /*
    create_mspace creates and returns a new independent space with the
    given initial capacity, or, if 0, the default granularity size.  It
    returns null if there is no system memory available to create the
    space.  If argument locked is non-zero, the space uses a separate
    lock to control access. The capacity of the space will grow
    dynamically as needed to service mspace_malloc requests.  You can
    control the sizes of incremental increases of this space by
    compiling with a different DEFAULT_GRANULARITY or dynamically
    setting with mallopt(M_GRANULARITY, value).
  */
  mspace create_mspace(size_t capacity, int locked);

  /*
    destroy_mspace destroys the given space, and attempts to return all
    of its memory back to the system, returning the total number of
    bytes freed. After destruction, the results of access to all memory
    used by the space become undefined.
  */
  size_t destroy_mspace(mspace msp);

  /*
    create_mspace_with_base uses the memory supplied as the initial base
    of a new mspace. Part (less than 128*sizeof(size_t) bytes) of this
    space is used for bookkeeping, so the capacity must be at least this
    large. (Otherwise 0 is returned.) When this initial space is
    exhausted, additional memory will be obtained from the system.
    Destroying this space will deallocate all additionally allocated
    space (if possible) but not the initial base.
  */
  mspace create_mspace_with_base(void* base, size_t capacity, int locked);

  /*
    mspace_malloc behaves as malloc, but operates within
    the given space.
  */
  void* mspace_malloc(mspace msp, size_t bytes);

  /*
    mspace_free behaves as free, but operates within
    the given space.

    If compiled with FOOTERS==1, mspace_free is not actually needed.
    free may be called instead of mspace_free because freed chunks from
    any space are handled by their originating spaces.
  */
  void mspace_free(mspace msp, void* mem);

  /*
    mspace_realloc behaves as realloc, but operates within
    the given space.

    If compiled with FOOTERS==1, mspace_realloc is not actually
    needed.  realloc may be called instead of mspace_realloc because
    realloced chunks from any space are handled by their originating
    spaces.
  */
  void* mspace_realloc(mspace msp, void* mem, size_t newsize);

  /*
    mspace_calloc behaves as calloc, but operates within
    the given space.
  */
  void* mspace_calloc(mspace msp, size_t n_elements, size_t elem_size);

  /*
    mspace_memalign behaves as memalign, but operates within
    the given space.
  */
  void* mspace_memalign(mspace msp, size_t alignment, size_t bytes);


  /*
    An alias for mallopt.
  */
  int mspace_mallopt(int, int);
#endif


#ifdef __cplusplus
}
;  /* end of extern "C" */
#endif

/*
  ========================================================================
  To make a fully customizable malloc.h header file, cut everything
  above this line, put into file malloc.h, edit to suit, and #include it
  on the next line, as well as in programs that use this malloc.
  ========================================================================
*/

/* #include "malloc.h" */

/*------------------------------ internal #includes ---------------------- */

#if 0
#include <stdio.h>       /* for printing in malloc_stats */

#ifndef LACKS_ERRNO_H
#include <errno.h>       /* for MALLOC_FAILURE_ACTION */
#endif
#if FOOTERS
#include <time.h>        /* for magic initialization */
#endif
#ifndef LACKS_STDLIB_H
#include <stdlib.h>      /* for abort() */
#endif
#ifdef DEBUG
#if ABORT_ON_ASSERT_FAILURE
#define assert(x) if(!(x)) ABORT
#else
#include <assert.h>
#endif
#else
#define assert(x)
#endif
#ifndef LACKS_STRING_H
#include <string.h>      /* for memset etc */
#endif
#if USE_BUILTIN_FFS
#ifndef LACKS_STRINGS_H
#include <strings.h>     /* for ffs */
#endif
#endif
#endif



/* ------------------- size_t and alignment properties -------------------- */

/* The byte and bit size of a size_t */
#define SIZE_T_SIZE         (sizeof(size_t))
#define SIZE_T_BITSIZE      (sizeof(size_t) << 3)

/* The size_t value with all bits set */
#define MAX_SIZE_T           (~(size_t)0)

/* The bit mask value corresponding to MALLOC_ALIGNMENT */
#define CHUNK_ALIGN_MASK    (MALLOC_ALIGNMENT - 1)

/* True if address a has acceptable alignment */
#define is_aligned(A)       (((size_t)((A)) & (CHUNK_ALIGN_MASK)) == 0)

/* the number of bytes to offset an address to align it */
#define align_offset(A)\
 ((((size_t)(A) & CHUNK_ALIGN_MASK) == 0)? 0 :\
  ((MALLOC_ALIGNMENT - ((size_t)(A) & CHUNK_ALIGN_MASK)) & CHUNK_ALIGN_MASK))


/* --------------------------- Lock preliminaries ------------------------ */

#if USE_LOCKS

/*
  When locks are defined, there are up to two global locks:
 
  * If HAVE_MORECORE, morecore_mutex protects sequences of calls to
    MORECORE.  In many cases sys_alloc requires two calls, that should
    not be interleaved with calls by other threads.  This does not
    protect against direct calls to MORECORE by other threads not
    using this lock, so there is still code to cope the best we can on
    interference.
 
  * If using secure footers, magic_init_mutex ensures that mparams.magic is
    initialized exactly once.
*/


/* By default use posix locks */
#include <pthread.h>
#define MLOCK_T pthread_mutex_t
#define ACQUIRE_LOCK(l)      pthread_mutex_lock(l)
#define RELEASE_LOCK(l)      pthread_mutex_unlock(l)


#if FOOTERS & !INSECURE
static MLOCK_T magic_init_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


#define USE_LOCK_BIT               (2U)
#else
#define USE_LOCK_BIT               (0U)
#endif

#if USE_LOCKS && HAVE_MORECORE
#define ACQUIRE_MORECORE_LOCK()    ACQUIRE_LOCK(&morecore_mutex);
#define RELEASE_MORECORE_LOCK()    RELEASE_LOCK(&morecore_mutex);
#else
#define ACQUIRE_MORECORE_LOCK()
#define RELEASE_MORECORE_LOCK()
#endif

#if USE_LOCKS && FOOTERS && !INSECURE
#define ACQUIRE_MAGIC_INIT_LOCK()  ACQUIRE_LOCK(&magic_init_mutex);
#define RELEASE_MAGIC_INIT_LOCK()  RELEASE_LOCK(&magic_init_mutex);
#else
#define ACQUIRE_MAGIC_INIT_LOCK()
#define RELEASE_MAGIC_INIT_LOCK()
#endif


/* -----------------------  Chunk representations ------------------------ */

/*
  (The following includes lightly edited explanations by Colin Plumb.)
 
  The malloc_chunk declaration below is misleading (but accurate and
  necessary).  It declares a "view" into memory allowing access to
  necessary fields at known offsets from a given base.
 
  Chunks of memory are maintained using a `boundary tag' method as
  originally described by Knuth.  (See the paper by Paul Wilson
  ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps for a survey of such
  techniques.)  Sizes of free chunks are stored both in the front of
  each chunk and at the end.  This makes consolidating fragmented
  chunks into bigger chunks fast.  The head fields also hold bits
  representing whether chunks are free or in use.
 
  Here are some pictures to make it clearer.  They are "exploded" to
  show that the state of a chunk can be thought of as extending from
  the high 31 bits of the head field of its header through the
  prev_foot and PINUSE_BIT bit of the following chunk header.
 
  A chunk that's in use looks like:
 
   chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           | Size of previous chunk (if P = 1)                             |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
         | Size of this chunk                                         1| +-+
   mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                                                               |
         +-                                                             -+
         |                                                               |
         +-                                                             -+
         |                                                               :
         +-      size - sizeof(size_t) available payload bytes          -+
         :                                                               |
 chunk-> +-                                                             -+
         |                                                               |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |1|
       | Size of next chunk (may or may not be in use)               | +-+
 mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 
    And if it's free, it looks like this:
 
   chunk-> +-                                                             -+
           | User payload (must be in use, or we would have merged!)       |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
         | Size of this chunk                                         0| +-+
   mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Next pointer                                                  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Prev pointer                                                  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                                                               :
         +-      size - sizeof(struct chunk) unused bytes               -+
         :                                                               |
 chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Size of this chunk                                            |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |0|
       | Size of next chunk (must be in use, or we would have merged)| +-+
 mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                                                               :
       +- User payload                                                -+
       :                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                                                     |0|
                                                                     +-+
  Note that since we always merge adjacent free chunks, the chunks
  adjacent to a free chunk must be in use.
 
  Given a pointer to a chunk (which can be derived trivially from the
  payload pointer) we can, in O(1) time, find out whether the adjacent
  chunks are free, and if so, unlink them from the lists that they
  are on and merge them with the current chunk.
 
  Chunks always begin on even word boundaries, so the mem portion
  (which is returned to the user) is also on an even word boundary, and
  thus at least double-word aligned.
 
  The P (PINUSE_BIT) bit, stored in the unused low-order bit of the
  chunk size (which is always a multiple of two words), is an in-use
  bit for the *previous* chunk.  If that bit is *clear*, then the
  word before the current chunk size contains the previous chunk
  size, and can be used to find the front of the previous chunk.
  The very first chunk allocated always has this bit set, preventing
  access to non-existent (or non-owned) memory. If pinuse is set for
  any given chunk, then you CANNOT determine the size of the
  previous chunk, and might even get a memory addressing fault when
  trying to do so.
 
  The C (CINUSE_BIT) bit, stored in the unused second-lowest bit of
  the chunk size redundantly records whether the current chunk is
  inuse. This redundancy enables usage checks within free and realloc,
  and reduces indirection when freeing and consolidating chunks.
 
  Each freshly allocated chunk must have both cinuse and pinuse set.
  That is, each allocated chunk borders either a previously allocated
  and still in-use chunk, or the base of its memory arena. This is
  ensured by making all allocations from the the `lowest' part of any
  found chunk.  Further, no free chunk physically borders another one,
  so each free chunk is known to be preceded and followed by either
  inuse chunks or the ends of memory.
 
  Note that the `foot' of the current chunk is actually represented
  as the prev_foot of the NEXT chunk. This makes it easier to
  deal with alignments etc but can be very confusing when trying
  to extend or adapt this code.
 
  The exceptions to all this are
 
     1. The special chunk `top' is the top-most available chunk (i.e.,
        the one bordering the end of available memory). It is treated
        specially.  Top is never included in any bin, is used only if
        no other chunk is available, and is released back to the
        system if it is very large (see M_TRIM_THRESHOLD).  In effect,
        the top chunk is treated as larger (and thus less well
        fitting) than any other available chunk.  The top chunk
        doesn't update its trailing size field since there is no next
        contiguous chunk that would have to index off it. However,
        space is still allocated for it (TOP_FOOT_SIZE) to enable
        separation or merging when space is extended.
 
     3. Chunks allocated via mmap, which have the lowest-order bit
        (IS_MMAPPED_BIT) set in their prev_foot fields, and do not set
        PINUSE_BIT in their head fields.  Because they are allocated
        one-by-one, each must carry its own prev_foot field, which is
        also used to hold the offset this chunk has within its mmapped
        region, which is needed to preserve alignment. Each mmapped
        chunk is trailed by the first two fields of a fake next-chunk
        for sake of usage checks.
 
*/

struct malloc_chunk
{
  size_t               prev_foot;  /* Size of previous chunk (if free).  */
  size_t               head;       /* Size and inuse bits. */
  struct malloc_chunk* fd;         /* double links -- used only if free. */
  struct malloc_chunk* bk;
};

typedef struct malloc_chunk  mchunk;
typedef struct malloc_chunk* mchunkptr;
typedef struct malloc_chunk* sbinptr;  /* The type of bins of chunks */
typedef unsigned int bindex_t;         /* Described below */
typedef unsigned int binmap_t;         /* Described below */
typedef unsigned int flag_t;           /* The type of various bit flag sets */

/* ------------------- Chunks sizes and alignments ----------------------- */

#define MCHUNK_SIZE         (sizeof(mchunk))

#define CHUNK_OVERHEAD      (SIZE_T_SIZE*2U)


/* The smallest size we can malloc is an aligned minimal chunk */
#define MIN_CHUNK_SIZE\
  (size_t)(((MCHUNK_SIZE + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK))

/* conversion from malloc headers to user pointers, and back */
#define chunk2mem(p)        ((void*)((char*)(p)       + (SIZE_T_SIZE*2U)))
#define mem2chunk(mem)      ((mchunkptr)((char*)(mem) - (SIZE_T_SIZE*2U)))
/* chunk associated with aligned address A */
#define align_as_chunk(A)   (mchunkptr)((A) + align_offset(chunk2mem(A)))

/* Bounds on request (not chunk) sizes. */
#define MAX_REQUEST         ((-MIN_CHUNK_SIZE) << 2)
#define MIN_REQUEST         (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - 1U)

/* pad request bytes into a usable size */
#define pad_request(req) \
   (((req) + CHUNK_OVERHEAD + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* pad request, checking for minimum (but not maximum) */
#define request2size(req) \
  (((req) < MIN_REQUEST)? MIN_CHUNK_SIZE : pad_request(req))


/* ------------------ Operations on head and foot fields ----------------- */

/*
  The head field of a chunk is or'ed with PINUSE_BIT when previous
  adjacent chunk in use, and or'ed with CINUSE_BIT if this chunk is in
  use.
*/

#define PINUSE_BIT          (1U)
#define CINUSE_BIT          (2U)
#define INUSE_BITS          (PINUSE_BIT|CINUSE_BIT)

/* Head value for fenceposts */
#define FENCEPOST_HEAD      (INUSE_BITS|SIZE_T_SIZE)

/* extraction of fields from head words */
#define cinuse(p)           ((p)->head & CINUSE_BIT)
#define pinuse(p)           ((p)->head & PINUSE_BIT)
#define chunksize(p)        ((p)->head & ~(INUSE_BITS))

#define clear_pinuse(p)     ((p)->head &= ~PINUSE_BIT)
#define clear_cinuse(p)     ((p)->head &= ~CINUSE_BIT)

/* Treat space at ptr +/- offset as a chunk */
#define chunk_plus_offset(p, s)  ((mchunkptr)(((char*)(p)) + (s)))
#define chunk_minus_offset(p, s) ((mchunkptr)(((char*)(p)) - (s)))

/* Ptr to next or previous physical malloc_chunk. */
#define next_chunk(p) ((mchunkptr)( ((char*)(p)) + ((p)->head & ~INUSE_BITS)))
#define prev_chunk(p) ((mchunkptr)( ((char*)(p)) - ((p)->prev_foot) ))

/* extract next chunk's pinuse bit */
#define next_pinuse(p)  ((next_chunk(p)->head) & PINUSE_BIT)

/* Get/set size at footer */
#define get_foot(p, s)  (((mchunkptr)((char*)(p) + (s)))->prev_foot)
#define set_foot(p, s)  (((mchunkptr)((char*)(p) + (s)))->prev_foot = (s))

/* Set size, pinuse bit, and foot */
#define set_size_and_pinuse_of_free_chunk(p, s)\
  ((p)->head = (s|PINUSE_BIT), set_foot(p, s))

/* Set size, pinuse bit, foot, and clear next pinuse */
#define set_free_with_pinuse(p, s, n)\
  (clear_pinuse(n), set_size_and_pinuse_of_free_chunk(p, s))

/* Get the internal overhead associated with chunk p */
#define overhead_for(p) (CHUNK_OVERHEAD)

#define calloc_must_clear(p) (1)

/* ---------------------- Overlaid data structures ----------------------- */

/*
  When chunks are not in use, they are treated as nodes of either
  lists or trees.
 
  "Small"  chunks are stored in circular doubly-linked lists, and look
  like this:
 
    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk in list             |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk in list            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space (may be 0 bytes long)                .
            .                                                               .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 
  Larger chunks are kept in a form of bitwise digital trees (aka
  tries) keyed on chunksizes.  Because malloc_tree_chunks are only for
  free chunks greater than 256 bytes, their size doesn't impose any
  constraints on user chunk sizes.  Each node looks like:
 
    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk of same size        |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk of same size       |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to left child (child[0])                  |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to right child (child[1])                 |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to parent                                 |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             bin index of this chunk                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space                                      .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 
  Each tree holding treenodes is a tree of unique chunk sizes.  Chunks
  of the same size are arranged in a circularly-linked list, with only
  the oldest chunk (the next to be used, in our FIFO ordering)
  actually in the tree.  (Tree members are distinguished by a non-null
  parent pointer.)  If a chunk with the same size an an existing node
  is inserted, it is linked off the existing node using pointers that
  work in the same way as fd/bk pointers of small chunks.
 
  Each tree contains a power of 2 sized range of chunk sizes (the
  smallest is 0x100 <= x < 0x180), which is is divided in half at each
  tree level, with the chunks in the smaller half of the range (0x100
  <= x < 0x140 for the top nose) in the left subtree and the larger
  half (0x140 <= x < 0x180) in the right subtree.  This is, of course,
  done by inspecting individual bits.
 
  Using these rules, each node's left subtree contains all smaller
  sizes than its right subtree.  However, the node at the root of each
  subtree has no particular ordering relationship to either.  (The
  dividing line between the subtree sizes is based on trie relation.)
  If we remove the last chunk of a given size from the interior of the
  tree, we need to replace it with a leaf node.  The tree ordering
  rules permit a node to be replaced by any leaf below it.
 
  The smallest chunk in a tree (a common operation in a best-fit
  allocator) can be found by walking a path to the leftmost leaf in
  the tree.  Unlike a usual binary tree, where we follow left child
  pointers until we reach a null, here we follow the right child
  pointer any time the left one is null, until we reach a leaf with
  both child pointers null. The smallest chunk in the tree will be
  somewhere along that path.
 
  The worst case number of steps to add, find, or remove a node is
  bounded by the number of bits differentiating chunks within
  bins. Under current bin calculations, this ranges from 6 up to 21
  (for 32 bit sizes) or up to 53 (for 64 bit sizes). The typical case
  is of course much better.
*/

struct malloc_tree_chunk
{
  /* The first four fields must be compatible with malloc_chunk */
  size_t                    prev_foot;
  size_t                    head;
  struct malloc_tree_chunk* fd;
  struct malloc_tree_chunk* bk;

  struct malloc_tree_chunk* child[2];
  struct malloc_tree_chunk* parent;
  bindex_t                  index;
};

typedef struct malloc_tree_chunk  tchunk;
typedef struct malloc_tree_chunk* tchunkptr;
typedef struct malloc_tree_chunk* tbinptr; /* The type of bins of trees */

/* A little helper macro for trees */
#define leftmost_child(t) ((t)->child[0] != 0? (t)->child[0] : (t)->child[1])

/* ----------------------------- Segments -------------------------------- */

/*
  Each malloc space may include non-contiguous segments, held in a
  list headed by an embedded malloc_segment record representing the
  initial space. Segments also include flags holding properties of
  the space. Large chunks that are directly allocated by mmap are not
  included in this list. They are instead independently created and
  destroyed without otherwise keeping track of them.
 
  Segment management mainly comes into play for spaces allocated by
  MMAP.  Any call to MMAP might or might not return memory that is
  adjacent to an existing segment.  MORECORE normally contiguously
  extends the current space, so this space is almost always adjacent,
  which is simpler and faster to deal with. (This is why MORECORE is
  used preferentially to MMAP when both are available -- see
  sys_alloc.)  When allocating using MMAP, we don't use any of the
  hinting mechanisms (inconsistently) supported in various
  implementations of unix mmap, or distinguish reserving from
  committing memory. Instead, we just ask for space, and exploit
  contiguity when we get it.  It is probably possible to do
  better than this on some systems, but no general scheme seems
  to be significantly better.
 
  Management entails a simpler variant of the consolidation scheme
  used for chunks to reduce fragmentation -- new adjacent memory is
  normally prepended or appended to an existing segment. However,
  there are limitations compared to chunk consolidation that mostly
  reflect the fact that segment processing is relatively infrequent
  (occurring only when getting memory from system) and that we
  don't expect to have huge numbers of segments:
 
  * Segments are not indexed, so traversal requires linear scans.  (It
    would be possible to index these, but is not worth the extra
    overhead and complexity for most programs on most platforms.)
  * New segments are only appended to old ones when holding top-most
    memory; if they cannot be prepended to others, they are held in
    different segments.
 
  Except for the initial segment of an mstate (which holds its own
  embedded segment record), segment records for one segment are
  kept in a different segment (the one in effect when the new
  segment was created).  So unmapping segments is delicate.
*/

struct malloc_segment
{
  size_t       size;             /* allocated size */
  struct malloc_segment* next;   /* ptr to next segment */
  struct malloc_segment* prev;   /* ptr to next segment */
  void* origin;
  flag_t       sflags;
};

typedef struct malloc_segment  msegment;
typedef struct malloc_segment* msegmentptr;

/* ---------------------------- malloc_state ----------------------------- */

/*
   A malloc_state holds all of the bookkeeping for a space.
   The main fields are:
 
  Top
    The topmost chunk of the currently active segment. Its size is
    cached in topsize.
 
  Designated victim (dv)
    This is the preferred chunk for servicing small requests that
    don't have exact fits.  It is normally the chunk split off most
    recently to service another small request.  Its size is cached in
    dvsize. The link fields of this chunk are not maintained since it
    is not kept in a bin.
 
  SmallBins
    An array of bin headers for free chunks.  These bins hold chunks
    with sizes less than MIN_LARGE_SIZE bytes. Each bin contains
    chunks of all the same size, spaced 8 bytes apart.  To simplify
    use in double-linked lists, each bin header acts as a malloc_chunk
    pointing to the real first node, if it exists (else pointing to
    itself).  This avoids special-casing for headers.  But to avoid
    waste, we allocate only the fd/bk pointers of bins, and then use
    repositioning tricks to treat these as the fields of a chunk.
 
  TreeBins
    Treebins are pointers to the roots of trees holding a range of
    sizes. There are 2 equally spaced treebins for each power of two
    from TREE_SHIFT to TREE_SHIFT+16. The last bin holds anything
    larger.
 
  Bin maps
    There is one bit map for small bins ("smallmap") and one for
    treebins ("treemap).  Each bin sets its bit when non-empty, and
    clears the bit when empty.  Bit operations are then used to avoid
    bin-by-bin searching -- nearly all "search" is done without ever
    looking at bins that won't be selected.  The bit maps
    conservatively use 32 bits per map word, even if on 64bit system.
    For a good description of some of the bit-based techniques used
    here, see Henry S. Warren Jr's book "Hacker's Delight" (and
    supplement at http://hackersdelight.org/). Many of these are
    intended to reduce the branchiness of paths through malloc etc, as
    well as to reduce the number of memory locations read or written.
 
  Segments
    A list of segments.
 
  Address check support
    The least_addr field is the least address ever obtained from
    MORECORE or MMAP. Attempted frees and reallocs of any address less
    than this are trapped (unless INSECURE is defined).
 
  Magic tag
    A cross-check field that should always hold same value as mparams.magic.
 
  Flags
    Bits recording whether to use MMAP, locks, or contiguous MORECORE
 
  Statistics
    Each space keeps track of current and maximum system memory
    obtained via MORECORE or MMAP.
 
  Locking
    If USE_LOCKS is defined, the "mutex" lock is acquired and released
    around every public call using this mspace.
*/

/* Bin types, widths and sizes */
#define NSMALLBINS        (32U)
#define NTREEBINS         (32U)
#define SMALLBIN_SHIFT    (3U)
#define SMALLBIN_WIDTH    (1U << SMALLBIN_SHIFT)
#define TREEBIN_SHIFT     (8U)
#define MIN_LARGE_SIZE    (1U << TREEBIN_SHIFT)
#define MAX_SMALL_SIZE    (MIN_LARGE_SIZE - 1)
#define MAX_SMALL_REQUEST (MAX_SMALL_SIZE - CHUNK_ALIGN_MASK - CHUNK_OVERHEAD)

struct malloc_state
{
  binmap_t   smallmap;
  binmap_t   treemap;
  size_t     dvsize;
  size_t     topsize;
  /*  char*      least_addr;*/
  mchunkptr  dv;
  mchunkptr  top;
  size_t     magic;
  mchunkptr  smallbins[(NSMALLBINS+1)*2];
  tbinptr    treebins[NTREEBINS];
  /*  size_t     footprint;*/
  /*  size_t     max_footprint;*/
  flag_t     mflags;
#if USE_LOCKS

  MLOCK_T    mutex;     /* locate lock among fields that rarely change */
#endif

  msegmentptr   seg;
};

/* typedef struct malloc_state*    mstate; */

/* ------------- Global malloc_state and malloc_params ------------------- */

/*
  malloc_params holds global properties, including those that can be
  dynamically set using mallopt. There is a single instance, mparams,
  initialized in init_mparams.
*/

struct malloc_params
{
  size_t magic;
  size_t page_size;
  size_t granularity;
  flag_t default_mflags;
};

static struct malloc_params mparams;

/* The global malloc_state used for all non-"mspace" calls */
static struct malloc_state _gm_;
#define gm                 (&_gm_)
#define is_global(M)       ((M) == &_gm_)
#define is_initialized(M)  ((M)->top != 0)

/* -------------------------- system alloc setup ------------------------- */

/* Operations on mflags */

#define use_lock(M)           ((M)->mflags &   USE_LOCK_BIT)
#define enable_lock(M)        ((M)->mflags |=  USE_LOCK_BIT)
#define disable_lock(M)       ((M)->mflags &= ~USE_LOCK_BIT)


#define set_lock(M,L)\
 ((M)->mflags = (L)?\
  ((M)->mflags | USE_LOCK_BIT) :\
  ((M)->mflags & ~USE_LOCK_BIT))

/* page-align a size */
#define page_align(S)\
 (((S) + (mparams.page_size)) & ~(mparams.page_size - 1))

/* granularity-align a size */
#define granularity_align(S)\
  (((S) + (mparams.granularity)) & ~(mparams.granularity - 1))

#define is_page_aligned(S)\
   (((size_t)(S) & (mparams.page_size - 1)) == 0)
#define is_granularity_aligned(S)\
   (((size_t)(S) & (mparams.granularity - 1)) == 0)





/* -------------------------------  Hooks -------------------------------- */

/*
  PREACTION should be defined to return 0 on success, and nonzero on
  failure. If you are not using locking, you can redefine these to do
  anything you like.
*/

#if USE_LOCKS

#define PREACTION(M)  ((use_lock(M))? ACQUIRE_LOCK(&(M)->mutex) : 0)
#define POSTACTION(M) { if (use_lock(M)) RELEASE_LOCK(&(M)->mutex); }
#else

#ifndef PREACTION
#define PREACTION(M) (0)
#endif

#ifndef POSTACTION
#define POSTACTION(M)
#endif

#endif

/*
  CORRUPTION_ERROR_ACTION is triggered upon detected bad addresses.
  USAGE_ERROR_ACTION is triggered on detected bad frees and
  reallocs. The argument p is an address that might have triggered the
  fault. It is ignored by the two predefined actions, but might be
  useful in custom actions that try to help diagnose errors.
*/

#if PROCEED_ON_ERROR

/* A count of the number of corruption errors causing resets */
int malloc_corruption_error_count;

/* default corruption action */
static void reset_on_error(mstate m);

#define CORRUPTION_ERROR_ACTION(m)  reset_on_error(m)
#define USAGE_ERROR_ACTION(m, p)

#else

#ifndef CORRUPTION_ERROR_ACTION
#define CORRUPTION_ERROR_ACTION(m) ABORT
#endif

#ifndef USAGE_ERROR_ACTION
#define USAGE_ERROR_ACTION(m,p) ABORT
#endif

#endif

/* -------------------------- Debugging setup ---------------------------- */

#if ! DEBUG

#define check_free_chunk(M,P)
#define check_inuse_chunk(M,P)
#define check_malloced_chunk(M,P,N)
#define check_mmapped_chunk(M,P)
#define check_malloc_state(M)
#define check_top_chunk(M,P)

#else
#define check_free_chunk(M,P)       do_check_free_chunk(M,P)
#define check_inuse_chunk(M,P)      do_check_inuse_chunk(M,P)
#define check_top_chunk(M,P)        do_check_top_chunk(M,P)
#define check_malloced_chunk(M,P,N) do_check_malloced_chunk(M,P,N)
#define check_mmapped_chunk(M,P)    do_check_mmapped_chunk(M,P)
#define check_malloc_state(M)       do_check_malloc_state(M)

static void   do_check_any_chunk(mstate m, mchunkptr p);
static void   do_check_top_chunk(mstate m, mchunkptr p);
static void   do_check_mmapped_chunk(mstate m, mchunkptr p);
static void   do_check_inuse_chunk(mstate m, mchunkptr p);
static void   do_check_free_chunk(mstate m, mchunkptr p);
static void   do_check_malloced_chunk(mstate m, void* mem, size_t s);
static void   do_check_tree(mstate m, tchunkptr t);
static void   do_check_treebin(mstate m, bindex_t i);
static void   do_check_smallbin(mstate m, bindex_t i);
static void   do_check_malloc_state(mstate m);
static int    bin_find(mstate m, mchunkptr x);
static size_t traverse_and_check(mstate m);
#endif

/* ---------------------------- Indexing Bins ---------------------------- */

#define is_small(s)         (((s) >> SMALLBIN_SHIFT) < NSMALLBINS)
#define small_index(s)      ((s)  >> SMALLBIN_SHIFT)
#define small_index2size(i) ((i)  << SMALLBIN_SHIFT)
#define MIN_SMALL_INDEX     (small_index(MIN_CHUNK_SIZE))

/* addressing by index. See above about smallbin repositioning */
#define smallbin_at(M, i)   ((sbinptr)((char*)&((M)->smallbins[(i)<<1])))
#define treebin_at(M,i)     (&((M)->treebins[i]))

/* assign tree index for size S to variable I */
#if defined(__GNUC__) && defined(i386)
#define compute_tree_index(S, I)\
{\
  size_t X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int K;\
    __asm__("bsrl %1,%0\n\t" : "=r" (K) : "rm"  (X));\
    I =  (bindex_t)((K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1)));\
  }\
}
#else
#define compute_tree_index(S, I)\
{\
  size_t X = S >> TREEBIN_SHIFT;\
  if (X == 0)\
    I = 0;\
  else if (X > 0xFFFF)\
    I = NTREEBINS-1;\
  else {\
    unsigned int Y = (unsigned int)X;\
    unsigned int N = ((Y - 0x100) >> 16) & 8;\
    unsigned int K = (((Y <<= N) - 0x1000) >> 16) & 4;\
    N += K;\
    N += K = (((Y <<= K) - 0x4000) >> 16) & 2;\
    K = 14 - N + ((Y <<= K) >> 15);\
    I = (K << 1) + ((S >> (K + (TREEBIN_SHIFT-1)) & 1));\
  }\
}
#endif

/* Bit representing maximum resolved size in a treebin at i */
#define bit_for_tree_index(i) \
   (i == NTREEBINS-1)? (SIZE_T_BITSIZE-1) : (((i) >> 1) + TREEBIN_SHIFT - 2)

/* Shift placing maximum resolved bit in a treebin at i as sign bit */
#define leftshift_for_tree_index(i) \
   ((i == NTREEBINS-1)? 0 : \
    ((SIZE_T_BITSIZE-1) - (((i) >> 1) + TREEBIN_SHIFT - 2)))

/* The size of the smallest chunk held in bin with index i */
#define minsize_for_tree_index(i) \
   (((size_t)(1U) << (((i) >> 1) + TREEBIN_SHIFT)) |  \
   (((size_t)((i) & 1U)) << (((i) >> 1U) + TREEBIN_SHIFT - 1)))


/* ------------------------ Operations on bin maps ----------------------- */

/* bit corresponding to given index */
#define idx2bit(i)              ((binmap_t)(1) << (i))

/* Mark/Clear bits with given index */
#define mark_smallmap(M,i)      ((M)->smallmap |=  idx2bit(i))
#define clear_smallmap(M,i)     ((M)->smallmap &= ~idx2bit(i))
#define smallmap_is_marked(M,i) ((M)->smallmap &   idx2bit(i))

#define mark_treemap(M,i)       ((M)->treemap  |=  idx2bit(i))
#define clear_treemap(M,i)      ((M)->treemap  &= ~idx2bit(i))
#define treemap_is_marked(M,i)  ((M)->treemap  &   idx2bit(i))

/* index corresponding to given bit */

#if defined(__GNUC__) && defined(i386)
#define compute_bit2idx(X, I)\
{\
  unsigned int J;\
  __asm__("bsfl %1,%0\n\t" : "=r" (J) : "rm" (X));\
  I = (bindex_t)J;\
}

#else
#if  USE_BUILTIN_FFS
#define compute_bit2idx(X, I) I = ffs(X)-1

#else
#define compute_bit2idx(X, I)\
{\
  unsigned int Y = X - 1;\
  unsigned int K = Y >> (16-4) & 16;\
  unsigned int N = K;        Y >>= K;\
  N += K = Y >> (8-3) &  8;  Y >>= K;\
  N += K = Y >> (4-2) &  4;  Y >>= K;\
  N += K = Y >> (2-1) &  2;  Y >>= K;\
  N += K = Y >> (1-0) &  1;  Y >>= K;\
  I = (bindex_t)(N + Y);\
}
#endif
#endif

/* isolate the least set bit of a bitmap */
#define least_bit(x)         ((x) & -(x))

/* mask with all bits to left of least bit of x on */
#define left_bits(x)         ((x<<1) | -(x<<1))

/* mask with all bits to left of or equal to least bit of x on */
#define same_or_left_bits(x) ((x) | -(x))


/* ----------------------- Runtime Check Support ------------------------- */

/*
  For security, the main invariant is that malloc/free/etc never
  writes to a static address other than malloc_state, unless static
  malloc_state itself has been corrupted, which cannot occur via
  malloc (because of these checks). In essence this means that we
  believe all pointers, sizes, maps etc held in malloc_state, but
  check all of those linked or offsetted from other embedded data
  structures.  These checks are interspersed with main code in a way
  that tends to minimize their run-time cost.
 
  When FOOTERS is defined, in addition to range checking, we also
  verify footer fields of inuse chunks, which can be used guarantee
  that the mstate controlling malloc/free is intact.  This is a
  streamlined version of the approach described by William Robertson
  et al in "Run-time Detection of Heap-based Overflows" LISA'03
  http://www.usenix.org/events/lisa03/tech/robertson.html The footer
  of an inuse chunk holds the xor of its mstate and a random seed,
  that is checked upon calls to free() and realloc().  This is
  (probablistically) unguessable from outside the program, but can be
  computed by any code successfully malloc'ing any chunk, so does not
  itself provide protection against code that has already broken
  security through some other means.  Unlike Robertson et al, we
  always dynamically check addresses of all offset chunks (previous,
  next, etc). This turns out to be cheaper than relying on hashes.
*/

#if !INSECURE
/* Check if address a is at least as high as any from MORECORE or MMAP */
#define ok_address(M, a) ((char*)(a) >= (M)->least_addr)
/* Check if address of next chunk n is higher than base chunk p */
#define ok_next(p, n)    ((char*)(p) < (char*)(n))
/* Check if p has its cinuse bit on */
#define ok_cinuse(p)     cinuse(p)
/* Check if p has its pinuse bit on */
#define ok_pinuse(p)     pinuse(p)

#else
#define ok_address(M, a) (1)
#define ok_next(b, n)    (1)
#define ok_cinuse(p)     (1)
#define ok_pinuse(p)     (1)
#endif

#if (FOOTERS && !INSECURE)
/* Check if (alleged) mstate m has expected magic field */
#define ok_magic(M)      ((M)->magic == mparams.magic)
#else
#define ok_magic(M)      (1)
#endif


/* In gcc, use __builtin_expect to minimize impact of checks */
#if !INSECURE
#if defined(__GNUC__) && __GNUC__ >= 3
#define RTCHECK(e)  __builtin_expect(e, 1)
#else
#define RTCHECK(e)  (e)
#endif
#else
#define RTCHECK(e)  (1)
#endif

/* macros to set up inuse chunks with footers */


/* Set foot of inuse chunk to be xor of mstate and seed */
#define mark_inuse_foot(M,p,s)\
  (((mchunkptr)((char*)(p) + (s)))->prev_foot = ((size_t)(M) ^ mparams.magic))

#if 1
#define get_mstate_for(p)\
  ((mstate)(((mchunkptr)((char*)(p) +\
    (chunksize(p))))->prev_foot ^ mparams.magic))
#else

#define get_footid_for(p)\
  ((mstate)(((mchunkptr)((char*)(p) +\
    (chunksize(p))))->prev_foot ^ mparams.magic))

#define get_mstate_for(p)\
  ((get_footid_for(p) & FOOT_IS_MEMSPACE) ?\
    (get_footpt_for(p) & ~FOOT_IS_MEMSPACE) : gm)

#endif

#define set_inuse(M,p,s)\
  ((p)->head = (((p)->head & PINUSE_BIT)|s|CINUSE_BIT),\
  (((mchunkptr)(((char*)(p)) + (s)))->head |= PINUSE_BIT), \
  mark_inuse_foot(M,p,s))

#define set_inuse_and_pinuse(M,p,s)\
  ((p)->head = (s|PINUSE_BIT|CINUSE_BIT),\
  (((mchunkptr)(((char*)(p)) + (s)))->head |= PINUSE_BIT),\
 mark_inuse_foot(M,p,s))

#define set_size_and_pinuse_of_inuse_chunk(M, p, s)\
  ((p)->head = (s|PINUSE_BIT|CINUSE_BIT),\
  mark_inuse_foot(M, p, s))


/* ---------------------------- setting mparams -------------------------- */

/* Initialize mparams */
static void init_mparams()
{
  if (mparams.page_size == 0) {

#if (FOOTERS && !INSECURE)
    {
      size_t s;
#if USE_DEV_RANDOM

      int fd;
      unsigned char buf[sizeof(size_t)];
      /* Try to use /dev/urandom, else fall back on using time */
      if ((fd = open("/dev/urandom", O_RDONLY)) >= 0 &&
          read(fd, buf, sizeof(buf)) == sizeof(buf)) {
        s = *((size_t *) buf);
        close(fd);
      } else
#endif

        s = (size_t)(time(0) ^ (size_t)0x55555555U);

      s |= 8U;    /* ensure nonzero */
      s &= ~7U;   /* improve chances of fault for bad values */

      ACQUIRE_MAGIC_INIT_LOCK();
      if (mparams.magic == 0)
        mparams.magic = s;
      RELEASE_MAGIC_INIT_LOCK();
    }

#else
    mparams.magic = (size_t)0x58585858U;
#endif


    mparams.default_mflags = USE_LOCK_BIT;


    /* Sanity-check configuration:
       size_t must be unsigned and as wide as pointer type.
       ints must be at least 4 bytes.
       alignment must be at least 8.
       Alignment, min chunk size, and page size must all be powers of 2.
    */
    if ((sizeof(size_t) != sizeof(char*)) ||
        (MAX_SIZE_T < MIN_CHUNK_SIZE)  ||
        (sizeof(int) < 4)  ||
        (MALLOC_ALIGNMENT < 8U) ||
        ((MALLOC_ALIGNMENT    & (MALLOC_ALIGNMENT-1))    != 0) ||
        ((MCHUNK_SIZE         & (MCHUNK_SIZE-1))         != 0))
      ABORT;
  }
}

/* support for mallopt */
static int change_mparam(int param_number, int value)
{
  //  size_t val = (size_t)value;
  init_mparams();
  switch(param_number) {
#if 0
  case M_TRIM_THRESHOLD:
    mparams.trim_threshold = val;
    return 1;
  case M_GRANULARITY:
    if (val >= mparams.page_size && ((val & (val-1)) == 0)) {
      mparams.granularity = val;
      return 1;
    } else
      return 0;
  case M_MMAP_THRESHOLD:
    mparams.mmap_threshold = val;
    return 1;
#endif

  default:
    return 0;
  }
}

#if DEBUG
/* ------------------------- Debugging Support --------------------------- */

/* Check properties of any chunk, whether free, inuse, mmapped etc  */
static void do_check_any_chunk(mstate m, mchunkptr p)
{
  assert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
  assert(ok_address(m, p));
}

/* Check properties of top chunk */
static void do_check_top_chunk(mstate m, mchunkptr p)
{
  msegmentptr sp = segment_holding(m, (char*)p);
  size_t  sz = chunksize(p);
  assert(sp != 0);
  assert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
  assert(ok_address(m, p));
  assert(sz == m->topsize);
  assert(sz > 0);
  assert(sz == ((sp->base + sp->size) - (char*)p) - TOP_FOOT_SIZE);
  assert(pinuse(p));
  assert(!next_pinuse(p));
}

/* Check properties of (inuse) mmapped chunks */
static void do_check_mmapped_chunk(mstate m, mchunkptr p)
{
  size_t  sz = chunksize(p);
  size_t len = (sz + (p->prev_foot & ~IS_MMAPPED_BIT) + MMAP_FOOT_PAD);
  assert(is_mmapped(p));
  assert(use_mmap(m));
  assert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
  assert(ok_address(m, p));
  assert(!is_small(sz));
  assert((len & (mparams.page_size-1)) == 0);
  assert(chunk_plus_offset(p, sz)->head == FENCEPOST_HEAD);
  assert(chunk_plus_offset(p, sz+SIZE_T_SIZE)->head == 0);
}

/* Check properties of inuse chunks */
static void do_check_inuse_chunk(mstate m, mchunkptr p)
{
  do_check_any_chunk(m, p);
  assert(cinuse(p));
  assert(next_pinuse(p));
  /* If not pinuse and not mmapped, previous chunk has OK offset */
  assert(is_mmapped(p) || pinuse(p) || next_chunk(prev_chunk(p)) == p);
  if (is_mmapped(p))
    do_check_mmapped_chunk(m, p);
}

/* Check properties of free chunks */
static void do_check_free_chunk(mstate m, mchunkptr p)
{
  size_t sz = p->head & ~(PINUSE_BIT|CINUSE_BIT);
  mchunkptr next = chunk_plus_offset(p, sz);
  do_check_any_chunk(m, p);
  assert(!cinuse(p));
  assert(!next_pinuse(p));
  assert (!is_mmapped(p));
  if (p != m->dv && p != m->top) {
    if (sz >= MIN_CHUNK_SIZE) {
      assert((sz & CHUNK_ALIGN_MASK) == 0);
      assert(is_aligned(chunk2mem(p)));
      assert(next->prev_foot == sz);
      assert(pinuse(p));
      assert (next == m->top || cinuse(next));
      assert(p->fd->bk == p);
      assert(p->bk->fd == p);
    } else  /* markers are always of size SIZE_T_SIZE */
      assert(sz == SIZE_T_SIZE);
  }
}

/* Check properties of malloced chunks at the point they are malloced */
static void do_check_malloced_chunk(mstate m, void* mem, size_t s)
{
  if (mem != 0) {
    mchunkptr p = mem2chunk(mem);
    size_t sz = p->head & ~(PINUSE_BIT|CINUSE_BIT);
    do_check_inuse_chunk(m, p);
    assert((sz & CHUNK_ALIGN_MASK) == 0);
    assert(sz >= MIN_CHUNK_SIZE);
    assert(sz >= s);
    /* unless mmapped, size is less than MIN_CHUNK_SIZE more than request */
    assert(is_mmapped(p) || sz < (s + MIN_CHUNK_SIZE));
  }
}

/* Check a tree and its subtrees.  */
static void do_check_tree(mstate m, tchunkptr t)
{
  tchunkptr head = 0;
  tchunkptr u = t;
  bindex_t tindex = t->index;
  size_t tsize = chunksize(t);
  bindex_t idx;
  compute_tree_index(tsize, idx);
  assert(tindex == idx);
  assert(tsize >= MIN_LARGE_SIZE);
  assert(tsize >= minsize_for_tree_index(idx));
  assert((idx == NTREEBINS-1) || (tsize < minsize_for_tree_index((idx+1))));

  do { /* traverse through chain of same-sized nodes */
    do_check_any_chunk(m, ((mchunkptr)u));
    assert(u->index == tindex);
    assert(chunksize(u) == tsize);
    assert(!cinuse(u));
    assert(!next_pinuse(u));
    assert(u->fd->bk == u);
    assert(u->bk->fd == u);
    if (u->parent == 0) {
      assert(u->child[0] == 0);
      assert(u->child[1] == 0);
    } else {
      assert(head == 0); /* only one node on chain has parent */
      head = u;
      assert(u->parent != u);
      assert (u->parent->child[0] == u ||
              u->parent->child[1] == u ||
              *((tbinptr*)(u->parent)) == u);
      if (u->child[0] != 0) {
        assert(u->child[0]->parent == u);
        assert(u->child[0] != u);
        do_check_tree(m, u->child[0]);
      }
      if (u->child[1] != 0) {
        assert(u->child[1]->parent == u);
        assert(u->child[1] != u);
        do_check_tree(m, u->child[1]);
      }
      if (u->child[0] != 0 && u->child[1] != 0) {
        assert(chunksize(u->child[0]) < chunksize(u->child[1]));
      }
    }
    u = u->fd;
  } while (u != t);
  assert(head != 0);
}

/*  Check all the chunks in a treebin.  */
static void do_check_treebin(mstate m, bindex_t i)
{
  tbinptr* tb = treebin_at(m, i);
  tchunkptr t = *tb;
  int empty = (m->treemap & (1 << i)) == 0;
  if (t == 0)
    assert(empty);
  if (!empty)
    do_check_tree(m, t);
}

/*  Check all the chunks in a smallbin.  */
static void do_check_smallbin(mstate m, bindex_t i)
{
  sbinptr b = smallbin_at(m, i);
  mchunkptr p = b->bk;
  unsigned int empty = (m->smallmap & (1 << i)) == 0;
  if (p == b)
    assert(empty);
  if (!empty) {
    for (; p != b; p = p->bk) {
      size_t size = chunksize(p);
      mchunkptr q;
      /* each chunk claims to be free */
      do_check_free_chunk(m, p);
      /* chunk belongs in bin */
      assert(small_index(size) == i);
      assert(p->bk == b || chunksize(p->bk) == chunksize(p));
      /* chunk is followed by an inuse chunk */
      q = next_chunk(p);
      if (q->head != FENCEPOST_HEAD)
        do_check_inuse_chunk(m, q);
    }
  }
}

/* Find x in a bin. Used in other check functions. */
static int bin_find(mstate m, mchunkptr x)
{
  size_t size = chunksize(x);
  if (is_small(size)) {
    bindex_t sidx = small_index(size);
    sbinptr b = smallbin_at(m, sidx);
    if (smallmap_is_marked(m, sidx)) {
      mchunkptr p = b;
      do {
        if (p == x)
          return 1;
      } while ((p = p->fd) != b);
    }
  } else {
    bindex_t tidx;
    compute_tree_index(size, tidx);
    if (treemap_is_marked(m, tidx)) {
      tchunkptr t = *treebin_at(m, tidx);
      size_t sizebits = size << leftshift_for_tree_index(tidx);
      while (t != 0 && chunksize(t) != size) {
        t = t->child[(sizebits >> (SIZE_T_BITSIZE-1)) & 1];
        sizebits <<= 1;
      }
      if (t != 0) {
        tchunkptr u = t;
        do {
          if (u == (tchunkptr)x)
            return 1;
        } while ((u = u->fd) != t);
      }
    }
  }
  return 0;
}

/* Traverse each chunk and check it; return total */
static size_t traverse_and_check(mstate m)
{
  size_t sum = 0;
  if (is_initialized(m)) {
    msegmentptr s = &m->seg;
    sum += m->topsize + TOP_FOOT_SIZE;
    while (s != 0) {
      mchunkptr q = align_as_chunk(s->base);
      mchunkptr lastq = 0;
      assert(pinuse(q));
      while (segment_holds(s, q) &&
             q != m->top && q->head != FENCEPOST_HEAD) {
        sum += chunksize(q);
        if (cinuse(q)) {
          assert(!bin_find(m, q));
          do_check_inuse_chunk(m, q);
        } else {
          assert(q == m->dv || bin_find(m, q));
          assert(lastq == 0 || cinuse(lastq)); /* Not 2 consecutive free */
          do_check_free_chunk(m, q);
        }
        lastq = q;
        q = next_chunk(q);
      }
      s = s->next;
    }
  }
  return sum;
}

/* Check all properties of malloc_state. */
static void do_check_malloc_state(mstate m)
{
  bindex_t i;
  size_t total;
  /* check bins */
  for (i = 0; i < NSMALLBINS; ++i)
    do_check_smallbin(m, i);
  for (i = 0; i < NTREEBINS; ++i)
    do_check_treebin(m, i);

  if (m->dvsize != 0) { /* check dv chunk */
    do_check_any_chunk(m, m->dv);
    assert(m->dvsize == chunksize(m->dv));
    assert(m->dvsize >= MIN_CHUNK_SIZE);
    assert(bin_find(m, m->dv) == 0);
  }

  if (m->top != 0) {   /* check top chunk */
    do_check_top_chunk(m, m->top);
    assert(m->topsize == chunksize(m->top));
    assert(m->topsize > 0);
    assert(bin_find(m, m->top) == 0);
  }

  total = traverse_and_check(m);
  assert(total <= m->footprint);
  assert(m->footprint <= m->max_footprint);
}
#endif


/* ----------------------- Operations on smallbins ----------------------- */

/*
  Various forms of linking and unlinking are defined as macros.  Even
  the ones for trees, which are very long but have very short typical
  paths.  This is ugly but reduces reliance on inlining support of
  compilers.
*/

/* Link a free chunk into a smallbin  */
#define insert_small_chunk(M, P, S) {\
  bindex_t I  = small_index(S);\
  mchunkptr B = smallbin_at(M, I);\
  mchunkptr F = B;\
  assert(S >= MIN_CHUNK_SIZE);\
  if (!smallmap_is_marked(M, I))\
    mark_smallmap(M, I);\
  else if (RTCHECK(ok_address(M, B->fd)))\
    F = B->fd;\
  else {\
    CORRUPTION_ERROR_ACTION(M);\
  }\
  B->fd = P;\
  F->bk = P;\
  P->fd = F;\
  P->bk = B;\
}

/* Unlink a chunk from a smallbin  */
#define unlink_small_chunk(M, P, S) {\
  mchunkptr F = P->fd;\
  mchunkptr B = P->bk;\
  bindex_t I = small_index(S);\
  assert(P != B);\
  assert(P != F);\
  assert(chunksize(P) == small_index2size(I));\
  if (F == B)\
    clear_smallmap(M, I);\
  else if (RTCHECK((F == smallbin_at(M,I) || ok_address(M, F)) &&\
                   (B == smallbin_at(M,I) || ok_address(M, B)))) {\
    F->bk = B;\
    B->fd = F;\
  }\
  else {\
    CORRUPTION_ERROR_ACTION(M);\
  }\
}

/* Unlink the first chunk from a smallbin */
#define unlink_first_small_chunk(M, B, P, I) {\
  mchunkptr F = P->fd;\
  assert(P != B);\
  assert(P != F);\
  assert(chunksize(P) == small_index2size(I));\
  if (B == F)\
    clear_smallmap(M, I);\
  else if (RTCHECK(ok_address(M, F))) {\
    B->fd = F;\
    F->bk = B;\
  }\
  else {\
    CORRUPTION_ERROR_ACTION(M);\
  }\
}

/* Replace dv node, binning the old one */
/* Used only when dvsize known to be small */
#define replace_dv(M, P, S) {\
  size_t DVS = M->dvsize;\
  if (DVS != 0) {\
    mchunkptr DV = M->dv;\
    assert(is_small(DVS));\
    insert_small_chunk(M, DV, DVS);\
  }\
  M->dvsize = S;\
  M->dv = P;\
}

/* ------------------------- Operations on trees ------------------------- */

/* Insert chunk into tree */
#define insert_large_chunk(M, X, S) {\
  tbinptr* H;\
  bindex_t I;\
  compute_tree_index(S, I);\
  H = treebin_at(M, I);\
  X->index = I;\
  X->child[0] = X->child[1] = 0;\
  if (!treemap_is_marked(M, I)) {\
    mark_treemap(M, I);\
    *H = X;\
    X->parent = (tchunkptr)H;\
    X->fd = X->bk = X;\
  }\
  else {\
    tchunkptr T = *H;\
    size_t K = S << leftshift_for_tree_index(I);\
    for (;;) {\
      if (chunksize(T) != S) {\
        tchunkptr* C = &(T->child[(K >> (SIZE_T_BITSIZE-1)) & 1]);\
        K <<= 1;\
        if (*C != 0)\
          T = *C;\
        else if (RTCHECK(ok_address(M, C))) {\
          *C = X;\
          X->parent = T;\
          X->fd = X->bk = X;\
          break;\
        }\
        else {\
          CORRUPTION_ERROR_ACTION(M);\
          break;\
        }\
      }\
      else {\
        tchunkptr F = T->fd;\
        if (RTCHECK(ok_address(M, T) && ok_address(M, F))) {\
          T->fd = F->bk = X;\
          X->fd = F;\
          X->bk = T;\
          X->parent = 0;\
          break;\
        }\
        else {\
          CORRUPTION_ERROR_ACTION(M);\
          break;\
        }\
      }\
    }\
  }\
}

/*
  Unlink steps:
 
  1. If x is a chained node, unlink it from its same-sized fd/bk links
     and choose its bk node as its replacement.
  2. If x was the last node of its size, but not a leaf node, it must
     be replaced with a leaf node (not merely one with an open left or
     right), to make sure that lefts and rights of descendents
     correspond properly to bit masks.  We use the rightmost descendent
     of x.  We could use any other leaf, but this is easy to locate and
     tends to counteract removal of leftmosts elsewhere, and so keeps
     paths shorter than minimally guaranteed.  This doesn't loop much
     because on average a node in a tree is near the bottom.
  3. If x is the base of a chain (i.e., has parent links) relink
     x's parent and children to x's replacement (or null if none).
*/

#define unlink_large_chunk(M, X) {\
  tchunkptr XP = X->parent;\
  tchunkptr R;\
  if (X->bk != X) {\
    tchunkptr F = X->fd;\
    R = X->bk;\
    if (RTCHECK(ok_address(M, F))) {\
      F->bk = R;\
      R->fd = F;\
    }\
    else {\
      CORRUPTION_ERROR_ACTION(M);\
    }\
  }\
  else {\
    tchunkptr* RP;\
    if (((R = *(RP = &(X->child[1]))) != 0) ||\
        ((R = *(RP = &(X->child[0]))) != 0)) {\
      tchunkptr* CP;\
      while ((*(CP = &(R->child[1])) != 0) ||\
             (*(CP = &(R->child[0])) != 0)) {\
        R = *(RP = CP);\
      }\
      if (RTCHECK(ok_address(M, RP)))\
        *RP = 0;\
      else {\
        CORRUPTION_ERROR_ACTION(M);\
      }\
    }\
  }\
  if (XP != 0) {\
    tbinptr* H = treebin_at(M, X->index);\
    if (X == *H) {\
      if ((*H = R) == 0) \
        clear_treemap(M, X->index);\
    }\
    else if (RTCHECK(ok_address(M, XP))) {\
      if (XP->child[0] == X) \
        XP->child[0] = R;\
      else \
        XP->child[1] = R;\
    }\
    else\
      CORRUPTION_ERROR_ACTION(M);\
    if (R != 0) {\
      if (RTCHECK(ok_address(M, R))) {\
        tchunkptr C0, C1;\
        R->parent = XP;\
        if ((C0 = X->child[0]) != 0) {\
          if (RTCHECK(ok_address(M, C0))) {\
            R->child[0] = C0;\
            C0->parent = R;\
          }\
          else\
            CORRUPTION_ERROR_ACTION(M);\
        }\
        if ((C1 = X->child[1]) != 0) {\
          if (RTCHECK(ok_address(M, C1))) {\
            R->child[1] = C1;\
            C1->parent = R;\
          }\
          else\
            CORRUPTION_ERROR_ACTION(M);\
        }\
      }\
      else\
        CORRUPTION_ERROR_ACTION(M);\
    }\
  }\
}

/* Relays to large vs small bin operations */

#define insert_chunk(M, P, S)\
  if (is_small(S)) insert_small_chunk(M, P, S)\
  else { tchunkptr TP = (tchunkptr)(P); insert_large_chunk(M, TP, S); }

#define unlink_chunk(M, P, S)\
  if (is_small(S)) unlink_small_chunk(M, P, S)\
  else { tchunkptr TP = (tchunkptr)(P); unlink_large_chunk(M, TP); }


/* -------------------------- mspace management -------------------------- */


/* Initialize bins for a new mstate that is otherwise zeroed out */
static void init_bins(mstate m)
{
  /* Establish circular links for smallbins */
  bindex_t i;
  for (i = 0; i < NSMALLBINS; ++i) {
    sbinptr bin = smallbin_at(m,i);
    bin->fd = bin->bk = bin;
  }
}

#if PROCEED_ON_ERROR

/* default corruption action */
static void reset_on_error(mstate m)
{
  int i;
  ++malloc_corruption_error_count;
  /* Reinitialize fields to forget about all memory */
  m->smallbins = m->treebins = 0;
  m->dvsize = m->topsize = 0;
  m->seg.base = 0;
  m->seg.size = 0;
  m->seg.next = 0;
  m->top = m->dv = 0;
  for (i = 0; i < NTREEBINS; ++i)
    *treebin_at(m, i) = 0;
  init_bins(m);
}
#endif



/* ---------------------------- malloc support --------------------------- */

/* allocate a large request from the best fitting chunk in a treebin */
static void* tmalloc_large(mstate m, void* footid, size_t nb)
{
  tchunkptr v = 0;
  size_t rsize = -nb; /* Unsigned negation */
  tchunkptr t;
  bindex_t idx;
  compute_tree_index(nb, idx);

  if ((t = *treebin_at(m, idx)) != 0) {
    /* Traverse tree for this bin looking for node with size == nb */
    size_t sizebits = nb << leftshift_for_tree_index(idx);
    tchunkptr rst = 0;  /* The deepest untaken right subtree */
    for (;;) {
      tchunkptr rt;
      size_t trem = chunksize(t) - nb;
      if (trem < rsize) {
        v = t;
        if ((rsize = trem) == 0)
          break;
      }
      rt = t->child[1];
      t = t->child[(sizebits >> (SIZE_T_BITSIZE-1)) & 1];
      if (rt != 0 && rt != t)
        rst = rt;
      if (t == 0) {
        t = rst; /* set t to least subtree holding sizes > nb */
        break;
      }
      sizebits <<= 1;
    }
  }

  if (t == 0 && v == 0) { /* set t to root of next non-empty treebin */
    binmap_t leftbits = left_bits(idx2bit(idx)) & m->treemap;
    if (leftbits != 0) {
      bindex_t i;
      binmap_t leastbit = least_bit(leftbits);
      compute_bit2idx(leastbit, i);
      t = *treebin_at(m, i);
    }
  }

  while (t != 0) { /* find smallest of tree or subtree */
    size_t trem = chunksize(t) - nb;
    if (trem < rsize) {
      rsize = trem;
      v = t;
    }
    t = leftmost_child(t);
  }

  /*  If dv is a better fit, return 0 so malloc will use it */
  if (v != 0 && rsize < (size_t)(m->dvsize - nb)) {
    if (RTCHECK(ok_address(m, v))) { /* split */
      mchunkptr r = chunk_plus_offset(v, nb);
      assert(chunksize(v) == rsize + nb);
      if (RTCHECK(ok_next(v, r))) {
        unlink_large_chunk(m, v);
        if (rsize < MIN_CHUNK_SIZE)
          set_inuse_and_pinuse(footid, v, (rsize + nb));
        else {
          set_size_and_pinuse_of_inuse_chunk(footid, v, nb);
          set_size_and_pinuse_of_free_chunk(r, rsize);
          insert_chunk(m, r, rsize);
        }
        return chunk2mem(v);
      }
    }
    CORRUPTION_ERROR_ACTION(m);
  }
  return 0;
}

/* allocate a small request from the best fitting chunk in a treebin */
static void* tmalloc_small(mstate m, void* footid, size_t nb)
{
  tchunkptr t, v;
  size_t rsize;
  bindex_t i;
  binmap_t leastbit = least_bit(m->treemap);
  compute_bit2idx(leastbit, i);

  v = t = *treebin_at(m, i);
  rsize = chunksize(t) - nb;

  while ((t = leftmost_child(t)) != 0) {
    size_t trem = chunksize(t) - nb;
    if (trem < rsize) {
      rsize = trem;
      v = t;
    }
  }

  if (RTCHECK(ok_address(m, v))) {
    mchunkptr r = chunk_plus_offset(v, nb);
    assert(chunksize(v) == rsize + nb);
    if (RTCHECK(ok_next(v, r))) {
      unlink_large_chunk(m, v);
      if (rsize < MIN_CHUNK_SIZE)
        set_inuse_and_pinuse(footid, v, (rsize + nb));
      else {
        set_size_and_pinuse_of_inuse_chunk(footid, v, nb);
        set_size_and_pinuse_of_free_chunk(r, rsize);
        replace_dv(m, r, rsize);
      }
      return chunk2mem(v);
    }
  }

  CORRUPTION_ERROR_ACTION(m);
  return 0;
}


/* -------------------------- public routines ---------------------------- */



void* mspace_malloc(memowner id, mspace space, size_t bytes)
{
  mstate ms;
  if(space == NULL)
    ms = gm;
  else
    ms = (mstate)space;
  if(id == 0)
    id = (memowner)ms;
  if (!ok_magic(ms)) {
    USAGE_ERROR_ACTION(ms,ms);
    return 0;
  }
  if (!PREACTION(ms)) {
    void* mem;
    size_t nb;
    if (bytes <= MAX_SMALL_REQUEST) {
      bindex_t idx;
      binmap_t smallbits;
      nb = (bytes < MIN_REQUEST)? MIN_CHUNK_SIZE : pad_request(bytes);
      idx = small_index(nb);
      smallbits = ms->smallmap >> idx;

      if ((smallbits & 0x3) != 0) { /* Remainderless fit to a smallbin. */
        mchunkptr b, p;
        idx += ~smallbits & 1;      /* Uses next bin if idx empty */
        b = smallbin_at(ms, idx);
        p = b->fd;
        assert(chunksize(p) == small_index2size(idx));
        unlink_first_small_chunk(ms, b, p, idx);
        set_inuse_and_pinuse(id, p, small_index2size(idx));
        mem = chunk2mem(p);
        check_malloced_chunk(ms, mem, nb);
        goto postaction;
      } else if (nb > ms->dvsize) {
        if (smallbits != 0) { /* Use chunk in next nonempty smallbin */
          mchunkptr b, p, r;
          size_t rsize;
          bindex_t i;
          binmap_t leftbits = (smallbits << idx) & left_bits(idx2bit(idx));
          binmap_t leastbit = least_bit(leftbits);
          compute_bit2idx(leastbit, i);
          b = smallbin_at(ms, i);
          p = b->fd;
          assert(chunksize(p) == small_index2size(i));
          unlink_first_small_chunk(ms, b, p, i);
          rsize = small_index2size(i) - nb;
          /* Fit here cannot be remainderless if 4byte sizes */
          if (SIZE_T_SIZE != 4 && rsize < MIN_CHUNK_SIZE)
            set_inuse_and_pinuse(id, p, small_index2size(i));
          else {
            set_size_and_pinuse_of_inuse_chunk(id, p, nb);
            r = chunk_plus_offset(p, nb);
            set_size_and_pinuse_of_free_chunk(r, rsize);
            replace_dv(ms, r, rsize);
          }
          mem = chunk2mem(p);
          check_malloced_chunk(ms, mem, nb);
          goto postaction;
        } else if (ms->treemap != 0 && (mem = tmalloc_small(ms, id, nb)) != 0) {
          check_malloced_chunk(ms, mem, nb);
          goto postaction;
        }
      }
    } else if (bytes >= MAX_REQUEST)
      nb = MAX_SIZE_T; /* Too big to allocate. Force failure (in sys alloc) */
    else {
      nb = pad_request(bytes);
      if (ms->treemap != 0 && (mem = tmalloc_large(ms, id, nb)) != 0) {
        check_malloced_chunk(ms, mem, nb);
        goto postaction;
      }
    }

    if (nb <= ms->dvsize) {
      size_t rsize = ms->dvsize - nb;
      mchunkptr p = ms->dv;
      if (rsize >= MIN_CHUNK_SIZE) { /* split dv */
        mchunkptr r = ms->dv = chunk_plus_offset(p, nb);
        ms->dvsize = rsize;
        set_size_and_pinuse_of_free_chunk(r, rsize);
        set_size_and_pinuse_of_inuse_chunk(id, p, nb);
      } else { /* exhaust dv */
        size_t dvs = ms->dvsize;
        ms->dvsize = 0;
        ms->dv = 0;
        set_inuse_and_pinuse(id, p, dvs);
      }
      mem = chunk2mem(p);
      check_malloced_chunk(ms, mem, nb);
      goto postaction;
    } else if (nb <= ms->topsize) { /* Split top */
      size_t rsize = ms->topsize -= nb;
      mchunkptr p = ms->top;
      if (rsize >= MIN_CHUNK_SIZE) {
        ms->top = NULL;
        size_t tsz = ms->topsize;
        ms->topsize = 0;
        set_inuse_and_pinuse(id, p, tsz);
      } else {
        mchunkptr r = ms->top = chunk_plus_offset(p, nb);
        r->head = rsize | PINUSE_BIT;
        set_size_and_pinuse_of_inuse_chunk(id, p, nb);
        check_top_chunk(ms, ms->top);
      }
      mem = chunk2mem(p);
      check_malloced_chunk(ms, mem, nb);
      goto postaction;
    }
    if(ms->mflags & MF_USE_ZONE_ALLOC) {
      mem = zone_alloc_dl(ms, (void*)id, nb);
      if(mem != 0)
        goto postaction;
    }

    /* TODO: try other allocators */

    /* Last resort: DMA memory (memory region below 16MiB "reserved" for DMA use) */
    mem = DMA_malloc(id, ms, bytes);

  postaction:
    POSTACTION(ms);
    return mem;
  }

  return 0;
}

void mspace_free(mspace space, void* mem)
{
  mstate fm;
  if (mem != 0) {
    mchunkptr p  = mem2chunk(mem);
    if(space == NULL)
      fm = get_mstate_for(p);
    else
      fm = (mstate)space;
    if (!ok_magic(fm)) {
      USAGE_ERROR_ACTION(fm, p);
      return;
    }
    if (!PREACTION(fm)) {
      check_inuse_chunk(fm, p);
      if (RTCHECK(ok_address(fm, p) && ok_cinuse(p))) {
        size_t psize = chunksize(p);
        mchunkptr next = chunk_plus_offset(p, psize);
        if (!pinuse(p)) {
          size_t prevsize = p->prev_foot;
          mchunkptr prev = chunk_minus_offset(p, prevsize);
          psize += prevsize;
          p = prev;
          if (RTCHECK(ok_address(fm, prev))) { /* consolidate backward */
            if (p != fm->dv) {
              unlink_chunk(fm, p, prevsize);
            } else if ((next->head & INUSE_BITS) == INUSE_BITS) {
              fm->dvsize = psize;
              set_free_with_pinuse(p, psize, next);
              goto postaction;
            }
          } else
            goto erroraction;
        }

        if (RTCHECK(ok_next(p, next) && ok_pinuse(next))) {
          if (!cinuse(next)) {  /* consolidate forward */
            if (next == fm->top) {
              size_t tsize = fm->topsize += psize;
              fm->top = p;
              p->head = tsize | PINUSE_BIT;
              if (p == fm->dv) {
                fm->dv = 0;
                fm->dvsize = 0;
              }
#if 0
              if (should_trim(fm, tsize))
                sys_trim(fm, 0);
#endif

              goto postaction;
            } else if (next == fm->dv) {
              size_t dsize = fm->dvsize += psize;
              fm->dv = p;
              set_size_and_pinuse_of_free_chunk(p, dsize);
              goto postaction;
            } else {
              size_t nsize = chunksize(next);
              psize += nsize;
              unlink_chunk(fm, next, nsize);
              set_size_and_pinuse_of_free_chunk(p, psize);
              if (p == fm->dv) {
                fm->dvsize = psize;
                goto postaction;
              }
            }
          } else
            set_free_with_pinuse(p, psize, next);
          insert_chunk(fm, p, psize);
          check_free_chunk(fm, p);
          goto postaction;
        }
      }
    erroraction:
      USAGE_ERROR_ACTION(fm, p);
    postaction:
      POSTACTION(fm);
    }
  }
}

void* mspace_calloc(memowner id, mspace space,
                    size_t n_elements, size_t elem_size)
{
  void* mem;
  size_t req = 0;
  mstate ms;
  if(space == NULL)
    ms = gm;
  else
    ms = (mstate)space;
  if(id == 0)
    id = (memowner)ms;
  if (!ok_magic(ms)) {
    USAGE_ERROR_ACTION(ms,ms);
    return 0;
  }
  if (n_elements != 0) {
    req = n_elements * elem_size;
    if (((n_elements | elem_size) & ~(size_t)0xffff) &&
        (req / n_elements != elem_size))
      req = MAX_SIZE_T; /* force downstream failure on overflow */
  }
  mem = mspace_malloc(id, space, req);
  if (mem != 0)
    memset(mem, 0, req);
  return mem;
}

void* mspace_realloc(memowner id, mspace space, void* oldmem, size_t bytes)
{
  if (oldmem == 0)
    return mspace_malloc(id, space, bytes);
  else {
    mchunkptr p  = mem2chunk(oldmem);
    mstate ms;
    if(space == NULL) {
      if( id == 0 )
        ms = get_mstate_for(p);
      else
        ms = gm;
    } else
      ms = (mstate)space;
    if(id == 0)
      id = (memowner)ms;
    if (!ok_magic(ms)) {
      USAGE_ERROR_ACTION(ms,ms);
      return 0;
    }
    if (bytes >= MAX_REQUEST) {
      MALLOC_FAILURE_ACTION;
      return 0;
    }
    if (!PREACTION(ms)) {
      mchunkptr oldp = mem2chunk(oldmem);
      size_t oldsize = chunksize(oldp);
      mchunkptr next = chunk_plus_offset(oldp, oldsize);
      mchunkptr newp = 0;
      void* extra = 0;

      /* Try to either shrink or extend into top. Else malloc-copy-free */

      if (RTCHECK(ok_address(ms, oldp) && ok_cinuse(oldp) &&
                  ok_next(oldp, next) && ok_pinuse(next))) {
        size_t nb = request2size(bytes);
        if (oldsize >= nb) { /* already big enough */
          size_t rsize = oldsize - nb;
          newp = oldp;
          if (rsize >= MIN_CHUNK_SIZE) {
            mchunkptr remainder = chunk_plus_offset(newp, nb);
            set_inuse(id, newp, nb);
            set_inuse(id, remainder, rsize);
            extra = chunk2mem(remainder);
          }
        } else if (next == ms->top && oldsize + ms->topsize > nb) {
          /* Expand into top */
          size_t newsize = oldsize + ms->topsize;
          size_t newtopsize = newsize - nb;
          mchunkptr newtop = chunk_plus_offset(oldp, nb);
          set_inuse(id, oldp, nb);
          newtop->head = newtopsize |PINUSE_BIT;
          ms->top = newtop;
          ms->topsize = newtopsize;
          newp = oldp;
        }
      } else {
        USAGE_ERROR_ACTION(ms, oldmem);
        POSTACTION(ms);
        return 0;
      }

      POSTACTION(ms);

      if (newp != 0) {
        if (extra != 0) {
          mspace_free(ms, extra);
        }
        check_inuse_chunk(ms, newp);
        return chunk2mem(newp);
      } else {
        void* newmem = mspace_malloc(id, space, bytes);
        if (newmem != 0) {
          size_t oc = oldsize - overhead_for(oldp);
          memcpy(newmem, oldmem, (oc < bytes)? oc : bytes);
          mspace_free(space, oldmem);
        }
        return newmem;
      }
    }
    return 0;
  }
}

void* mspace_memalign(memowner id, mspace space,
                      size_t alignment, size_t bytes)
{
  mstate ms;
  if(space == NULL)
    ms = gm;
  else
    ms = (mstate)space;
  if(id == 0)
    id = (memowner)ms;
  if (!ok_magic(ms)) {
    USAGE_ERROR_ACTION(ms,ms);
    return 0;
  }
  if (alignment <= MALLOC_ALIGNMENT)    /* Can just use malloc */
    return mspace_malloc(id, ms, bytes);
  if (alignment <  MIN_CHUNK_SIZE) /* must be at least a minimum chunk size */
    alignment = MIN_CHUNK_SIZE;
  if ((alignment & (alignment-1)) != 0) {/* Ensure a power of 2 */
    size_t a = MALLOC_ALIGNMENT * 2;
    while (a < alignment)
      a <<= 1;
    alignment = a;
  }

  if (bytes >= MAX_REQUEST - alignment) {
    MALLOC_FAILURE_ACTION;
  } else {
    size_t nb = request2size(bytes);
    size_t req = nb + alignment + MIN_CHUNK_SIZE - CHUNK_OVERHEAD;
    char* mem = (char*)mspace_malloc(id, space, req);
    if (mem != 0) {
      void* leader = 0;
      void* trailer = 0;
      mchunkptr p = mem2chunk(mem);

      if (PREACTION(ms))
        return 0;
      if ((((size_t)(mem)) % alignment) != 0) { /* misaligned */
        /*
          Find an aligned spot inside chunk.  Since we need to give
          back leading space in a chunk of at least MIN_CHUNK_SIZE, if
          the first calculation places us at a spot with less than
          MIN_CHUNK_SIZE leader, we can move to the next aligned spot.
          We've allocated enough total room so that this is always
          possible.
        */
        char* brk = (char*)mem2chunk((size_t)(((size_t)(mem + alignment-1)) &
                                              -alignment));
        char* pos = ((size_t)(brk - (char*)(p)) >= MIN_CHUNK_SIZE)?
                    brk : brk+alignment;
        mchunkptr newp = (mchunkptr)pos;
        size_t leadsize = pos - (char*)(p);
        size_t newsize = chunksize(p) - leadsize;

        /* Give back leader, use the rest */
        set_inuse(id, newp, newsize);
        set_inuse(id, p, leadsize);
        leader = chunk2mem(p);
        p = newp;
      }

      /* Give back spare room at the end */
      size_t size = chunksize(p);
      if (size > nb + MIN_CHUNK_SIZE) {
        size_t remainder_size = size - nb;
        mchunkptr remainder = chunk_plus_offset(p, nb);
        set_inuse(id, p, nb);
        set_inuse(id, remainder, remainder_size);
        trailer = chunk2mem(remainder);
      }

      assert (chunksize(p) >= nb);
      assert((((size_t)(chunk2mem(p))) % alignment) == 0);
      check_inuse_chunk(ms, p);
      POSTACTION(ms);
      if (leader != 0) {
        mspace_free(ms, leader);
      }
      if (trailer != 0) {
        mspace_free(ms, trailer);
      }
      return chunk2mem(p);
    }
  }
  return 0;
}

int mspace_mallopt(int param_number, int value)
{
  return change_mparam(param_number, value);
}


#if 0 /* Is this function worth the extra size? */



void** ialloc(memowner id,
              size_t n_elements,
              size_t* sizes,
              int opts,
              void* chunks[])
{
  /*
    This provides common support for independent_X routines, handling
    all of the combinations that can result.

    The opts arg has:
    bit 0 set if all elements are same size (using sizes[0])
    bit 1 set if elements should be zeroed
  */

  size_t    element_size;   /* chunksize of each element, if all same */
  size_t    contents_size;  /* total size of elements */
  size_t    array_size;     /* request size of pointer array */
  void*     mem;            /* malloced aggregate space */
  mchunkptr p;              /* corresponding chunk */
  size_t    remainder_size; /* remaining bytes while splitting */
  void**    marray;         /* either "chunks" or malloced ptr array */
  mchunkptr array_chunk;    /* chunk for malloced ptr array */
  flag_t    was_enabled;    /* to disable mmap */
  size_t    size;
  size_t    i;

  mstate ms;
  if(id->mflags & IS_MEMSPACE)
    ms = (mstate)id;
  else
    ms = gm;
  if (!ok_magic(ms)) {
    USAGE_ERROR_ACTION(ms,ms);
    return 0;
  }

  /* compute array length, if needed */
  if (chunks != 0) {
    if (n_elements == 0)
      return chunks; /* nothing to do */
    marray = chunks;
    array_size = 0;
  } else {
    /* if empty req, must still return chunk representing empty array */
    if (n_elements == 0)
      return (void**)mspace_malloc(id, 0);
    marray = 0;
    array_size = request2size(n_elements * (sizeof(void*)));
  }

  /* compute total element size */
  if (opts & 0x1) { /* all-same-size */
    element_size = request2size(*sizes);
    contents_size = n_elements * element_size;
  } else { /* add up all the sizes */
    element_size = 0;
    contents_size = 0;
    for (i = 0; i != n_elements; ++i)
      contents_size += request2size(sizes[i]);
  }

  size = contents_size + array_size;

  /*
     Allocate the aggregate chunk.  First disable direct-mmapping so
     malloc won't use it, since we would not be able to later
     free/realloc space internal to a segregated mmap region.
  */
  mem = mspace_malloc(id, size - CHUNK_OVERHEAD);
  if (mem == 0)
    return 0;

  if (PREACTION(ms))
    return 0;
  p = mem2chunk(mem);
  remainder_size = chunksize(p);

  if (opts & 0x2) {       /* optionally clear the elements */
    memset((size_t*)mem, 0, remainder_size - SIZE_T_SIZE - array_size);
  }

  /* If not provided, allocate the pointer array as final part of chunk */
  if (marray == 0) {
    size_t  array_chunk_size;
    array_chunk = chunk_plus_offset(p, contents_size);
    array_chunk_size = remainder_size - contents_size;
    marray = (void**) (chunk2mem(array_chunk));
    set_size_and_pinuse_of_inuse_chunk(id, array_chunk, array_chunk_size);
    remainder_size = contents_size;
  }

  /* split out elements */
  for (i = 0; ; ++i) {
    marray[i] = chunk2mem(p);
    if (i != n_elements-1) {
      if (element_size != 0)
        size = element_size;
      else
        size = request2size(sizes[i]);
      remainder_size -= size;
      set_size_and_pinuse_of_inuse_chunk(id, p, size);
      p = chunk_plus_offset(p, size);
    } else { /* the final element absorbs any overallocation slop */
      set_size_and_pinuse_of_inuse_chunk(footid, p, remainder_size);
      break;
    }
  }

#if DEBUG
  if (marray != chunks) {
    /* final element must have exactly exhausted chunk */
    if (element_size != 0) {
      assert(remainder_size == element_size);
    } else {
      assert(remainder_size == request2size(sizes[i]));
    }
    check_inuse_chunk(ms, mem2chunk(marray));
  }
  for (i = 0; i != n_elements; ++i)
    check_inuse_chunk(ms, mem2chunk(marray[i]));

#endif

  POSTACTION(ms);
  return marray;
}

#endif



static inline int log2_n_round_up(unsigned n)
{
  unsigned int m;
__asm__("bsrl %1,%0\n\t" : "=r" (m) : "rm" (n - 1));
  return m + 1;
}


static inline int log2(unsigned n)
{
  unsigned int m;
__asm__("bsrl %1,%0\n\t" : "=r" (m) : "rm" (n));
  return m;
}

 
/*
  Physical memory is divided into 32 equally sized zones, the zone size must
  be a power of two. Example: 48MiB of physical memory will be divided into
  24 zones, each of size 2MiB. Each zone is either free or is under
  exclusive control by an allocator, or rather allocator state "object".
  An exception to this is the legacy memory region below 1MiB, which gets
  special treatment. (Note that "zone" here has a different meaning than in
  Linux.) 

  Global Lea allocator: New zones should be added in such a way as to
  minimize the number of segments. Each segment should end on a zone
  boundary unless there is holes in physical memory. This does not apply to
  independent memory spaces, which would typically get memory regions from a
  global allocator.

  Global buddy allocator: See comments for the buddy allocator towards the
  end of this file.
*/
            
struct buddy_state;

struct zonemap
{
  int bm_available; /* zones that represent available physical memory */
  int bm_dl_global; /* zones in use by the global dlmalloc (gm) */
  int bm_dl_space;  /* zones in use by the dlmalloc spaces */
  int bm_buddy;     /* zones in use by a buddy allocator */
  size_t zone_size;
  void* zman[32];   /* array of pointers to the allocator (state)
                         that are using the zone */
  void* zinfo[32];  /* Extra per zone info specific to the allocator. In the
                         case of dlmalloc, this is the address of the start
                         of the segment whitch contains the zone. */
  struct buddy_state* global_buddy; /* the common list buddy if used */
};


static struct zonemap _zm_;
#define zm                 (&_zm_)

/*
  Each segment starts with a struct malloc_segment and ends with a fencepost,
  a dedicated chunk always marked as in use to prevent attempts to
  consolidate chunks beyond the end of the segment. The "memory" portion of
  the fencepost contains a pointer to the beginning of the segment.
*/

#define FENCEPOST_SIZE ((CHUNK_OVERHEAD + SIZE_T_SIZE +\
                        CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)
#define SEG_HEAD_SIZE ((sizeof(struct malloc_segment) +\
                        CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)


static inline void create_fencepost(msegmentptr sp)
{
  mchunkptr fp = (mchunkptr)((char*)sp + sp->size - FENCEPOST_SIZE);
  fp->head = (fp->head & PINUSE_BIT) | FENCEPOST_SIZE | CINUSE_BIT;
  *((msegmentptr*)chunk2mem(fp)) = sp;
}

static inline void release_chunk(mstate m, mchunkptr p, size_t size)
{
  if(m->top == p) {
    m->top = NULL;
    m->topsize = 0;
    return;
  }
  if(m->dv == p) {
    m->dv = NULL;
    m->dvsize = 0;
    return;
  }
  unlink_chunk(m, p, size);
}

static inline void unlink_seg(mstate m, msegmentptr sp)
{
  if(sp->next != NULL)
    sp->next->prev = sp->prev;
  if(sp->prev != NULL)
    sp->prev->next = sp->next;
  else
    m->seg = sp->next;
}

static inline void link_seg(mstate m, msegmentptr sp)
{
  sp->prev = NULL;
  sp->next = m->seg;
  m->seg = sp;
}

/*
  Join segments with free zones and/or another segment, and allocates chunk
  of size req in the new space between the segments. Returns NULL without
  making any changes if insufficient space. new_space_start is the base
  address of the lowest zone to be joined, and nr_zones is the size of the
  new space in zones. Expects either
  sp1 == NULL and new_space_start < sp2 or
  sp2 == NULL and sp1 < new_space_start or
  sp1 <= new_space_start <= sp2.
  In the last case nr_zones may be 0.
*/
static void* join_zones_and_segments(mstate ms,
                                     void* footid,
                                     size_t req,
                                     msegmentptr sp1,
                                     msegmentptr sp2,
                                     int nr_zones,
                                     char* new_space_start)
{
  mchunkptr fp, p = NULL;
  size_t pre_space = 0, post_space = 0, newspace;
  size_t overhead = FENCEPOST_SIZE + SEG_HEAD_SIZE;
  int log_zsize = log2(zm->zone_size);
  int first_zone, i;
  mchunkptr r, v;
  size_t rsize, span;
  int mask;
  msegmentptr newsp;

  if(sp1 != NULL) {
    /* Find the amount of free space at the end of sp1 when
        the fencepost is removed. */
    fp = (mchunkptr)(new_space_start - FENCEPOST_SIZE);
    pre_space = FENCEPOST_SIZE;
    if(!pinuse(fp))
      pre_space += fp->prev_foot;
    overhead -= SEG_HEAD_SIZE;
  }
  if(sp2 != NULL) {
    /* Find the amount of free space at the start of sp2 when
        the segment head is removed. */
    p = (mchunkptr)(((char*)sp2) + SEG_HEAD_SIZE);
    post_space = SEG_HEAD_SIZE;
    if( !cinuse(p) )
      post_space = chunksize(p);
    overhead -= FENCEPOST_SIZE;
  }
  newspace = nr_zones << log_zsize;
  span = pre_space + post_space + newspace - overhead;
  if( req > span)
    /* Insufficinet space! */
    return NULL;
  /* Reserve the new zones */
  first_zone = (size_t)new_space_start >> log_zsize;
  mask = 1 << first_zone;
  /* TODO: insert blocking to protect zm */
  for(i=0; i<nr_zones; i++) {
    zm->zman[first_zone + i] = ms;
    if(ms == gm)
      zm->bm_dl_global |= mask;
    else
      zm->bm_dl_space |= mask;
    if(sp1 != 0)
      zm->zinfo[first_zone + i] = sp1;
    else
      zm->zinfo[first_zone + i] = sp2;
    mask <<= 1;
  }
  if(p != NULL && !cinuse(p))
    release_chunk(ms, p, chunksize(p));
  if(sp1 != 0) {
    /* adjust sp1 */
    sp1->size += newspace;
    if(sp2 != 0) {
      sp1->size += sp2->size;
      unlink_seg(ms, sp2);
    } else
      create_fencepost(sp1);
    if(pinuse(fp))
      v = fp;
    else {
      v = prev_chunk(fp);
      release_chunk(ms, v, chunksize(v));
    }
  } else {
    /* new segmnet head */
    newsp = (msegmentptr)new_space_start;
    newsp->size = sp2->size + newspace;
    newsp->sflags = SEG_FLAG_ZONE;
    unlink_seg(ms, sp2);
    link_seg(ms, newsp);
    v = (mchunkptr)((char*)new_space_start + SEG_HEAD_SIZE);
  }
  rsize = span - req;
  r = chunk_plus_offset(v, req);
  if (rsize < MIN_CHUNK_SIZE) {
    set_inuse_and_pinuse(footid, v, span);
  } else {
    set_size_and_pinuse_of_inuse_chunk(footid, v, req);
    set_size_and_pinuse_of_free_chunk(r, rsize);
    /* Whether or not the remaining space should be a new top chunk.
        FIXME: improve this. */
    if(sp2 == NULL
        || ( (char*)r + rsize + FENCEPOST_SIZE == (char*)sp2 + sp2->size
             && zm->zman[(size_t)((char*)sp2 + sp2->size) >> log_zsize] == NULL
             && rsize > ms->topsize ) ) {
      if(ms->top != NULL) {
        insert_chunk(ms, ms->top, ms->topsize);
      }
      ms->top = r;
      ms->topsize = rsize;
    } else
      insert_chunk(ms, r, rsize);
  }
  return chunk2mem(v);
}


/*
  Try to satisfy a request for a chunk of size req by adding zones to ms.
  There is room for optimizing this. Specifically:
  TODO: treat the case req < zone_size - FENCEPOST_SIZE - SEG_HEAD_SIZE
      separately.
*/

static void* zone_alloc_dl(mstate ms, void* footid, size_t req)
{
  mchunkptr fp, p;
  size_t last_size, first_size;
  char* segend;
  int log_zsize = log2(zm->zone_size);
  int nr_zones, next_zone, i, j;
  mchunkptr r, v;
  size_t rsize, span;
  int mask, freemask;
  void* mem;
  msegmentptr sp, sp2;

  /* First try to enlarge an existing segment. */
  sp = ms->seg;
  while(sp != NULL) {
    segend = (char*)sp + sp->size;
    /* end of segment zone aligned? */
    if((size_t)segend & (zm->zone_size - 1)) {
      next_zone = (size_t)segend >> log_zsize;
      fp = (mchunkptr)(segend - FENCEPOST_SIZE);
      if(pinuse(fp))
        last_size = 0;
      else
        last_size = fp->prev_foot;
      nr_zones = ((req - last_size - 1) >> log_zsize) + 1;
      i = 0;
      while(next_zone < 32) {
        if(zm->zman[next_zone] != NULL) { /* zone available? */
          if(zm->zman[next_zone] == ms) { /* ours? */
            sp2 = (msegmentptr)((size_t)next_zone << log_zsize);
            /* do the segment actually start at the beginnign of the zone? */
            if(sp2 != zm->zinfo[next_zone])
              break;
            /* OK, maybe enough space at the bottom of sp2, try it */
            mem = join_zones_and_segments(ms, footid, req,
                                          sp, sp2, i, segend);
            if(mem != 0)
              return mem;
          }
          break;
        }
        next_zone++;
        i++;
        if(i == nr_zones)
          return join_zones_and_segments(ms, footid, req,
                                         sp, NULL, i, segend);
      }
    }
    /* not enough space above the segment, try below */
    if((size_t)sp & (zm->zone_size - 1)
        && zm->zman[(size_t)sp >> log_zsize] == ms) {
      next_zone = ((size_t)sp >> log_zsize) - 1;
      p = (mchunkptr)((char*)sp + SEG_HEAD_SIZE);
      if(pinuse(p))
        first_size = 0;
      else
        first_size = chunksize(p);
      nr_zones = ((req - first_size - 1) >> log_zsize) + 1;
      i = 0;
      while(next_zone >= 0) {
        if(zm->zman[next_zone] != NULL) {
          if(zm->zman[next_zone] == ms) {
            next_zone++;
            segend = (void*)(next_zone << log_zsize);
            fp = (mchunkptr)(segend - FENCEPOST_SIZE);
            sp2 = zm->zinfo[next_zone];
            if((char*)sp2 + sp2->size != segend)
              break;
            assert(sp2 == (void*)chunk2mem(fp));
            mem = join_zones_and_segments(ms, footid, req,
                                          sp2, sp, i, segend);
            if(mem != 0)
              return mem;
          }
          break;
        }
        i++;
        if(i == nr_zones)
          return join_zones_and_segments(ms, footid, req, NULL,
                                         sp, i, (char*)(next_zone << log_zsize));
        next_zone--;
      }
    }
    sp = sp->next;
  }
  /* Try to create new segment instead */
  nr_zones = ((req + FENCEPOST_SIZE + SEG_HEAD_SIZE - 1) >> log_zsize) + 1;
  mask = (1 << nr_zones) - 1;
  freemask = zm->bm_available
             & ~(zm->bm_dl_global | zm->bm_dl_space | zm->bm_buddy);
  for(i=0; mask != (mask & freemask); i++) {
    if(i == 32)
      return NULL;
    mask <<= 1;
  }
  sp = (msegmentptr)(i << log_zsize);
  sp->size = 1 << nr_zones;
  sp->sflags = SEG_FLAG_ZONE;
  link_seg(ms, sp);
  create_fencepost(sp);
  /* TODO: insert blocking to protect zm */
  for(j=0; j<nr_zones; j++) {
    zm->zman[j + i] = ms;
    zm->zinfo[j + i] = sp;
    if(ms == gm)
      zm->bm_dl_global |= mask;
    else
      zm->bm_dl_space |= mask;
  }
  v = (mchunkptr)((char*)sp + SEG_HEAD_SIZE);
  span = sp->size - FENCEPOST_SIZE - SEG_HEAD_SIZE;
  rsize = span - req;
  r = chunk_plus_offset(v, req);
  if (rsize < MIN_CHUNK_SIZE) {
    set_inuse_and_pinuse(footid, v, span);
  } else {
    set_size_and_pinuse_of_inuse_chunk(footid, v, req);
    set_size_and_pinuse_of_free_chunk(r, rsize);
    if(zm->zman[(size_t)((char*)sp + sp->size) >> log_zsize] == NULL
        && rsize > ms->topsize ) {
      if(ms->top != NULL) {
        insert_chunk(ms, ms->top, ms->topsize);
      }
      ms->top = r;
      ms->topsize = rsize;
    } else
      insert_chunk(ms, r, rsize);
  }
  return chunk2mem(v);
}


static mstate init_user_mstate(char* tbase, size_t tsize)
{
  size_t msize = pad_request(sizeof(struct malloc_state));
  void* id;
  size_t rsize;
  mchunkptr mn;
  msegmentptr sp = (msegmentptr)tbase;
  mchunkptr msp = (mchunkptr)((char*)sp + SEG_HEAD_SIZE);
  sp->size = (size_t)tbase;
  sp->sflags = SEG_FLAG_ORIGINAL;
  mstate m = (mstate)(chunk2mem(msp));
  id = m; /* ?? */
  memset(m, 0, msize);
  link_seg(m, sp);
  create_fencepost(sp);
  //  msp->head = (msize|PINUSE_BIT|CINUSE_BIT);
#if 0

  m->seg.base = m->least_addr = tbase;
  m->seg.size = m->footprint = m->max_footprint = tsize;
#endif

  m->magic = mparams.magic;
  m->mflags = mparams.default_mflags;
  init_bins(m);
  mn = chunk_plus_offset(msp, msize);
  rsize = tsize - msize - FENCEPOST_SIZE - SEG_HEAD_SIZE;
  if (rsize < MIN_CHUNK_SIZE) {
    set_inuse_and_pinuse(id, msp, (msize + rsize));
  } else {
    set_size_and_pinuse_of_inuse_chunk(id, msp, msize);
    set_size_and_pinuse_of_free_chunk(mn, rsize);
#if 0

    insert_chunk(m, mn, rsize);
    m->top = NULL;
    ms->topsize = 0;
#else

    m->top = mn;
    m->topsize = rsize;
    check_top_chunk(m, m->top);
#endif

  }
  return m;
}

mspace create_mspace_with_base(void* base, size_t capacity, int locked)
{
  mstate m = 0;
  size_t msize = pad_request(sizeof(struct malloc_state));
  init_mparams(); /* Ensure pagesize etc initialized */

  if (capacity > msize + FENCEPOST_SIZE + SEG_HEAD_SIZE
      && capacity < (size_t)
      - (msize + FENCEPOST_SIZE + SEG_HEAD_SIZE + mparams.page_size)
      /* Alignment is callers responibility */
      && !(CHUNK_ALIGN_MASK & capacity)
      && !(CHUNK_ALIGN_MASK & (size_t)base) ) {
    m = init_user_mstate((char*)base, capacity);
    set_lock(m, locked);
  }
  return (mspace)m;
}

void buddy_free(struct buddy_state* bs, void* block, unsigned log_size);

static size_t free_segment(mstate ms, msegmentptr sp)
{
  int nr_zones, i, mask;
  size_t size = sp->size;

  if(sp->sflags & SEG_FLAG_BUDDY) {
    buddy_free(sp->origin, sp, log2(size));
    return size;
  }
  if(sp->sflags & SEG_FLAG_DL) {
    mspace_free(sp->origin, sp);
    return size;
  }
  if(sp->sflags & SEG_FLAG_ZONE) {
    int log_zsize = log2(zm->zone_size);
    i = (size_t)sp >> log_zsize;
    nr_zones = sp->size >> log_zsize;
    mask = 1 << i;
    while(nr_zones--) {
      /* TODO: insert blocking to protect zm */
      zm->zman[i] = NULL;
      zm->zinfo[i] = NULL;
      if(ms == gm)
        zm->bm_dl_global &= ~mask;
      else
        zm->bm_dl_space &= ~mask;
      i++;
      mask <<=1;
    }
    return size;
  }
  return 0;
}

size_t destroy_mspace(mspace msp)
{
  msegmentptr sp1, sp2, sp_original = NULL;
  size_t freed = 0;
  mstate ms = (mstate)msp;

  if (ok_magic(ms)) {
    sp1 = ms->seg;
    while(sp1 != NULL) {
      sp2 = sp1;
      sp1 = sp1->next;
      if(sp2->sflags & SEG_FLAG_ORIGINAL) {
        sp_original = sp2;
        continue;
      }
      freed += free_segment(ms, sp2);
    }
    assert(sp_original != NULL);
    if(sp_original->sflags & SEG_FLAG_DESTROY_ORIGINAL)
      freed += free_segment(ms, sp_original);
  } else {
    USAGE_ERROR_ACTION(ms,ms);
  }
  return freed;
}

/* ========================== buddy allocator ============================ */

/*
  This is a simple buddy allocator. This allocator operator operates in two
  modes, one global mode and one for separate, continuous memory spaces.
  In both modes the struct buddy_state, together with the embedded free
  list and the buddy bitmap it points to, holds the state of the allocator.
  A bit in the bitmap is set iff the corresponding two buddies are in
  different states, i.e. one in use and one free. The bits for the smallest
  blocks comes first, then the bits for the next smallest size blocks etc.

  The global mode is supposed to work in a SMP friendly way, with partially
  separate independently lockable memory spaces consisting of one zone each.
  For this to function well, there should be some shared data (bitmap?) used
  to find which zones have free memory of the right size, in order to avoid
  each thread having to try several zones.
  TODO: This is not implemented yet!
  In the mean time, the zones shares the free lists, which means the whole
  thing have to be locked, and therefore is not SMP friendly.


  TODO: Implement bitmaps with available sizes for each space to reduce
      memory accesses. Note: this is different from the TODO above.

  TODO: buddy_realloc
*/

/* flags */
#define COMMON_LISTS 1
#define BUDDY_KMALLOC_USED 1


struct free_list_link
{
  struct free_list_link* next;
  struct free_list_link* prev;
};


struct buddy_state
{
  int flags;
  size_t size;
  int log_granularity;
  int nr_levels;      /* == log_sp_size - log_granularity + 1 , redundant? */
  size_t bitmap_total_size; /* rounded to power of 2 */
  void* base;   /* not used by common list buddy */
  int log_sp_size;
  struct free_list_link** list_pt;
  char** bitmap_array; /* used by common list buddy only */
  char* bitmap; /* else use this */
};

static inline void* unlink_head(struct buddy_state* bs, int i)
{
  struct free_list_link* l;
  l = bs->list_pt[i];
  if(l->next == l)
  {
    bs->list_pt[i] = 0;
    return l;
  }
  l->next->prev = l->prev;
  l->prev->next = l->next;
  bs->list_pt[i] = l->next;
  return l;
}

static inline void link_head(struct buddy_state* bs, int i, void* pt)
{
  struct free_list_link* l = pt;
  if(bs->list_pt[i] == 0)
  {
    bs->list_pt[i] = l;
    l->next = l;
    l->prev = l;
    return;
  }
  l->next = bs->list_pt[i];
  l->prev = l->next->prev;
  l->next->prev = l;
  l->prev->next = l;
}

static inline void unlink_block(struct buddy_state* bs,
                                int i, size_t size, void* block)
{
  struct free_list_link* l = block;
  if(l->next == l)
  {
    bs->list_pt[i] = 0;
    return;
  }
  l->next->prev = l->prev;
  l->prev->next = l->next;
  bs->list_pt[i] = l->next;
}

void init_zonemap(/* struct zonemap* zm,*/ size_t mem,
    mstate ms, int dl_zones_init)
{
  int i;

  i = log2_n_round_up(mem);
  zm->zone_size = 1 << (i - 5);
  zm->bm_available = (1 << ( mem >> (i - 5) )) - 1;
  zm->bm_dl_global = 0;
  zm->bm_dl_space = 0;
  zm->bm_buddy = 0;
  if(ms)
    adjust_zones(ms, dl_zones_init);
}

/* Add zone specified by mask to the common list buddy state cb */

int add_buddy_zone_common(/* struct zonemap* zm,*/struct buddy_state* cb,
    int mask)
{
  int index;
  void* pt;
  /* more than a single bit set? */
  if(mask & (mask - 1))
    return 0;
  /* zone already taken? */
  if(mask & (zm->bm_dl_global | zm->bm_dl_space
             | zm->bm_buddy
             | ~(zm->bm_available)))
    return 0;
  zm->bm_buddy |= mask;
  index = log2(mask);
  zm->zman[index] = cb;
  pt = kmalloc(cb->bitmap_total_size);
  if(pt == NULL)
    return 0;
  cb->bitmap_array[index] = pt;
  memset(pt, 0, cb->bitmap_total_size);
  link_head(cb, cb->nr_levels - 1, (void*)((unsigned)index * zm->zone_size));
  return 1;
}

int init_buddy_common(int zone_bmap, int nr_zones, size_t granularity)
{
  struct buddy_state* cb;
  struct free_list_link** list_array;
  void* pt;
  int n, i;
  int mask = 1;

  cb = kmalloc(sizeof(struct buddy_state));
  if(cb == NULL)
    return ENOMEM;
  zm->global_buddy = cb;
  cb->flags = COMMON_LISTS;
  if(granularity < 16)
    granularity = 16;
  cb->log_granularity = log2_n_round_up(granularity);
  cb->nr_levels = log2(zm->zone_size) - cb->log_granularity + 1;
  list_array = kmalloc(cb->nr_levels * sizeof(void*));
  if(list_array == NULL) {
    kfree(cb);
    return ENOMEM;
  }
  cb->list_pt = list_array;
  for(i=0; i < cb->nr_levels; i++)
    list_array[i] = NULL;
  cb->log_sp_size = log2(zm->zone_size);
  cb->size = zm->zone_size;
  cb->bitmap_total_size = 1 << (cb->log_sp_size - cb->log_granularity - 3);
  pt = kmalloc(32 * sizeof(void*));
  if(pt == NULL) {
    kfree(cb);
    kfree(list_array);
    return ENOMEM;
  }
  cb->bitmap_array = pt;
  n = 0;
  while(mask) {
    if(mask & zone_bmap) {
      if(add_buddy_zone_common(cb, mask))
        n++;
    }
    mask <<= 1;
  }
  mask = 1 << 31;
  while(mask && n < nr_zones) {
    if(add_buddy_zone_common(cb, mask))
      n++;
    mask >>=1;
  }
  return n;
}

struct buddy_state* create_buddy_space_with_base(void* base,
          size_t capacity/*, int locked */,
          size_t granularity,
          int use_kmalloc)
{
  struct buddy_state* cb;
  struct free_list_link** list_array;
  void* pt;
  size_t rounded_capacity, n;
  size_t net_capacity = capacity;
  int i;

  if(!(capacity > granularity && capacity > sizeof(struct buddy_state) +140))
    return NULL;
  rounded_capacity = 1 << log2_n_round_up(capacity);
  if(use_kmalloc)
  {
    cb = kmalloc(sizeof(struct buddy_state));
    if(cb == NULL)
      return NULL;
  } else
  {
    cb = (struct buddy_state*)((char*)base + capacity
                               - sizeof(struct buddy_state));
    cb = (struct buddy_state*)((size_t)cb & -16L); /* align by 16 */
    net_capacity -= (char*)base + capacity - (char*)cb;
  }
  /*    cb->info.flags = BUDDY_SPACE;*/
  if(granularity < 4)
    granularity = 4;
  cb->log_granularity = log2_n_round_up(granularity);
  cb->nr_levels = log2(rounded_capacity) - cb->log_granularity + 1;
  if(use_kmalloc)
  {
    list_array = kmalloc(cb->nr_levels * sizeof(void*));
    if(list_array == NULL) {
      kfree(cb);
      return NULL;
    }
    cb->flags = BUDDY_KMALLOC_USED;
  } else
  {
    list_array = (struct free_list_link**)((char*)cb
                                           - cb->nr_levels * sizeof(void*));
    net_capacity -= cb->nr_levels * sizeof(void*);
    cb->flags = 0;
  }
  cb->list_pt = list_array;
  for(i=0; i < cb->nr_levels; i++)
    list_array[i] = NULL;
  cb->log_sp_size = log2(rounded_capacity);
  cb->size = capacity;
  cb->bitmap_total_size = 1 << (cb->log_sp_size - cb->log_granularity - 3);
  if(use_kmalloc)
  {
    pt = kmalloc(cb->bitmap_total_size);
    if(pt == NULL) {
      kfree(cb);
      kfree(list_array);
      return NULL;
    }
  } else
  {
    pt = (char*)list_array - cb->bitmap_total_size;
    net_capacity -= cb->bitmap_total_size;
  }
  cb->bitmap = pt;
  memset(pt, 0, cb->bitmap_total_size);
  cb->base = base;
  n = rounded_capacity;
  i = cb->nr_levels - 1;
  while(i >= 0)
  {
    if(n <= net_capacity) {
      buddy_free(cb, base, i + cb->log_granularity);
      base += n;
    }
    net_capacity -= n;
    n >>= 1;
    i--;
  }
  return cb;
}

void destroy_buddy_space(struct buddy_state* bs)
{
  if(!(bs->flags & BUDDY_KMALLOC_USED) )
    return;
  kfree(bs->bitmap);
  kfree(bs->list_pt);
  kfree(bs);
}


void* buddy_alloc(struct buddy_state* bs, unsigned log_size)
{
  int block_offset, index, zindex, temp, i, j, mask;
  char* bitmap_start;
  unsigned sz, bit_offs, mask_offs, bmap_offs;
  char* block;
  size_t bm_size;

  if (!PREACTION(bs))
  {
    index = log_size - bs->log_granularity;
    if(index < 0)
      index = 0;
    i = index;
    while(i < bs->nr_levels) {
      if(bs->list_pt[i] != NULL) {
        block = unlink_head(bs, i);
        j = index;
        sz = 1 << log_size;
        while(j < i) {
          link_head(bs, j, block + sz);
          sz <<= 1;
          j++;
        }
        if(bs->flags & COMMON_LISTS) {
          zindex = (size_t)block >> bs->log_sp_size;
          bitmap_start = bs->bitmap_array[zindex];
          block_offset = (size_t)block & (bs->size - 1);
        } else {
          bitmap_start = bs->bitmap;
          block_offset = block - (char*)bs->base;
        }
        bm_size = bs->bitmap_total_size;
        bm_size >>= index;
        bmap_offs = (-bm_size) ^ (-bs->bitmap_total_size);
        bit_offs = (size_t)block_offset >> (log_size + 1);
        j = index;
        while(j < bs->nr_levels - 2) {
          mask = 1 << (bit_offs & 7);
          mask_offs = bit_offs >> 3;
          temp = bitmap_start[ bmap_offs + mask_offs ] ^= mask;
          if( !(temp & mask) )
            break;
          j++;
          bit_offs >>= 1;
          bm_size >>= 1;
          bmap_offs += bm_size;
        }
        goto postaction;
      }
      i++;
    }
    block = 0;
  postaction:
    POSTACTION(ms);
    return block;
  }
  return 0;
}

void buddy_free(struct buddy_state* bs, void* block, unsigned log_size)
{
  int block_offset, index, zindex, temp, i, mask;
  char* bitmap_start;
  unsigned sz, bit_offs, mask_offs, bmap_offs;
  char* bl = block;
  size_t bm_size;

  if (!PREACTION(bs))
  {
    index = log_size - bs->log_granularity;
    if(index < 0)
      index = 0;
    i = index;
    sz = 1 << log_size;
    if(bs->flags & COMMON_LISTS) {
      zindex = (unsigned long)block >> bs->log_sp_size;
      bitmap_start = bs->bitmap_array[zindex];
      block_offset = (size_t)block & (bs->size - 1);
    } else {
      bitmap_start = bs->bitmap;
      block_offset = block - bs->base;
    }
    bm_size = bs->bitmap_total_size;
    bm_size >>= index;
    bmap_offs = (-bm_size) ^ (-bs->bitmap_total_size);
    bit_offs = (size_t)block_offset >> (log_size + 1);
    while(i < bs->nr_levels - 1) {
      mask = 1 << (bit_offs & 7);
      mask_offs = bit_offs >> 3;
      temp = bitmap_start[ bmap_offs + mask_offs ] ^= mask;
      if( !(temp & mask) )
        break;
      if((size_t)block & sz) {
        bl -= sz;
        unlink_block(bs, i, sz, bl);
      } else
        unlink_block(bs, i, sz, bl + sz);
      i++;
      bit_offs >>= 1;
      bm_size >>= 1;
      bmap_offs += bm_size;
    }
    link_head(bs, i + 1, bl);
    POSTACTION(ms);
    return;
  }
  return;
}




