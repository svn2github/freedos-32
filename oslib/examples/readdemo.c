/* Project:     OSLib
 * Description: The OS Construction Kit
 * Date:                1.6.2000
 * Idea by:             Luca Abeni & Gerardo Lamastra
 *
 * OSLib is an SO project aimed at developing a common, easy-to-use
 * low-level infrastructure for developing OS kernels and Embedded
 * Applications; it partially derives from the HARTIK project but it
 * currently is independently developed.
 *
 * OSLib is distributed under GPL License, and some of its code has
 * been derived from the Linux kernel source; also some important
 * ideas come from studying the DJGPP go32 extender.
 *
 * We acknowledge the Linux Community, Free Software Foundation,
 * D.J. Delorie and all the other developers who believe in the
 * freedom of software and ideas.
 *
 * For legalese, check out the included GPL license.
 */

/*	DOS Read demo: reads a file using RM DOS calls */

#include <ll/i386/hw-func.h>
#include <ll/i386/tss-ctx.h>
#include <ll/i386/x-dos.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>

int main (int argc, char *argv[])
{
    DWORD sp1, sp2;
    int l;
    DOS_FILE *f;
    char b[100];
    struct multiboot_info *mbi;

    sp1 = get_sp();
    mbi = l1_init();
    if (dos_init() < 0) {
        error("Error in dos_init()");
	l1_exit(-1);
    }

    if (mbi == NULL) {
	    message("Error in LowLevel initialization code...\n");
	    l1_exit(-1);
    }
    
    message("Opening file c:\\test.txt...");
    f = dos_fopen("C:\\TEST.TXT", "r");
    if (f == NULL) {
	error("Error in dos_fopen()");
	message("dos_error: %d\n", dos_error());
    } else {
	message("Reading 100 bytes...");
	l = dos_fread(b, 100, 1, f);
	if (l <= 0) {
            error("Problems in dos_fread()");
	    message("dos_error: %d\n", dos_error());
	} else {
	    b[99]=0;
	    message("Read:\n%s\n", b);
	}
	dos_fclose(f);
    }
    
    sp2 = get_sp();
    message("End reached!\n");
    message("Actual stack : %lx - ", sp2);
    message("Begin stack : %lx\n", sp1);
    message("Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");
    
    l1_end();
    return 1;
}
