CC  = mingw32-gcc
AS  = mingw32-gcc
LD  = ld
AR  = ar
DLLTOOL = dlltool

ifndef OSLIB_DIR
OSLIB_DIR = ..\oslib
endif

SEP    = \\
INCL   = $(BASE)
VMINCL  = -I$(BASE)/include/i386 -I$(BASE)/include -I$(BASE)/include/sys/ll
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)\lib

C_OPT =  -Wall -O -finline-functions -nostdinc -mno-stack-arg-probe -ffreestanding -D__WIN32__ -I$(INCL)
ASM_OPT =  -x assembler-with-cpp -D__WIN32__ -I$(INCL)
LINK_OPT = -T $(OSLIB)/mk/os-mingw.x -Bstatic -Ttext 0x120000 -s -nostartfiles -nostdlib -L$(LIB_PATH)

MKDIR	= md
CP	= copy
CAT	= @type
RM	= del
RMDIR	= rd

# Common rules
%.o : %.s
	$(REDIR) $(CC) $(ASM_OPT) $(A_OUTPUT) -c $<
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
