CC  = i586-pc-msdosdjgpp-gcc
AS  = i586-pc-msdosdjgpp-gcc
LD  = i586-pc-msdosdjgpp-ld
AR  = i586-pc-msdosdjgpp-ar
INCL   = $(BASE)
VMINCL  = -I$(BASE)/include/i386 -I$(BASE)/include -I$(BASE)/include/sys/ll
LIB_PATH    = $(BASE)/lib/
LIB_DIR  = $(BASE)/lib

C_OPT =  -Wall -O -finline-functions -fno-builtin -nostdinc -D__GNU__ -I$(INCL)
ASM_OPT =  -x assembler-with-cpp -D__GNU__ -I$(INCL)
LINK_OPT = -T $(OSLIB)/mk/os.x -Bstatic -Ttext 0x120000 -nostartfiles -nostdlib -L$(LIB_PATH)

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
