CC  = gcc
AS  = gcc
LD  = ld
AR  = ar
LIB_PATH = $(BASE)/lib/
LIB_DIR  = $(BASE)/lib

C_OPT = -Wall -O -finline-functions -fno-builtin -nostdinc -D__GNU__
LINK_OPT = -Bstatic -nostartfiles -nostdlib -L$(LIB_PATH)

MKDIR	= mkdir
CP		= cp
CAT		= cat
RM		= rm -rf
RMDIR	= rm -rf

# Common rules
%.o : %.s
	$(REDIR) $(CC) $(ASM_OPT) $(A_OUTPUT) -c $<
%.o : %.c
	$(REDIR) $(CC) $(C_OPT) $(C_OUTPUT) -c $<
