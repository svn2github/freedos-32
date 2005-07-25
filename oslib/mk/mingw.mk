CC  = i386-mingw32-gcc
AS  = i386-mingw32-gcc
LD  = i386-mingw32-ld
AR  = i386-mingw32-ar

INCL   = $(BASE)
VMINCL  = -I$(BASE)/include/i386 -I$(BASE)/include -I$(BASE)/include/sys/ll
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)/lib

C_OPT =  -Wall -O -finline-functions -nostdinc -mno-stack-arg-probe -ffreestanding -D__WIN32__ -I$(INCL)
ASM_OPT =  -x assembler-with-cpp -D__WIN32__ -I$(INCL)
LINK_OPT = -T $(BASE)/mk/os-mingw.x -Bstatic -Ttext 0x320000 -s -nostartfiles -nostdlib -L$(LIB_PATH)

MKDIR	= mkdir
CP	= cp
CAT	= cat
RM	= rm -rf
RMDIR	= rm -rf

# Common rules
%.o : %.s
	$(REDIR) $(CC) $(ASM_OPT) $(A_OUTPUT) -c $<
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
