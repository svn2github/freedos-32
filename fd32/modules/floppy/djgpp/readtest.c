#include <stdlib.h> /* atexit */
#include <time.h>
#include <bios.h>
#include <string.h>
#include <stdio.h>
#include <crt0.h>
#include "blockio.h"


#define LOG_PRINTF(x) printf x
int _crt0_startup_flags = _CRT0_FLAG_LOCK_MEMORY;


static Floppy drive0;


FdcSetupCallback drive_detected;
int drive_detected(Fdd *drive)
{
    static unsigned k = 0;
    if (drive->dp->cmos_type)
    {
        printf("Detected %s floppy drive \"fd%u\"\n", drive->dp->name, k);
        drive0.fdd = drive;
        drive0.buffers[0].data  = malloc(512 * 18 * 2);
        drive0.buffers[0].start = (unsigned) -1;
        drive0.buffers[0].flags = 0;
    }
    k++;
    return 0;
}


static void dispose(void)
{
    fdc_dispose();
    fd32_event_done();
}


int main()
{
    unsigned k;
    BYTE     buffer[512];

    fd32_event_init();
    if (fdc_setup(drive_detected) < 0) return -1;
    atexit(dispose);

    printf("Press Enter to start a floppy read test...\n");
    getchar();
    for (k = 0; k < 80; k++)
    {
        int b = rand() % 2880;
        int res = floppy_read(&drive0, b, 1, buffer);
        if (res == FDC_NODISK) break;
        else if (res < 0) printf("Error on block %u\n", k);
        /*if (k % 36 == 0) */printf("%llu: block %u read\n", uclock() * 1000 / UCLOCKS_PER_SEC, b);
    }
    printf("Read test completed.\n");
#if 0
    printf("Press Enter to start a floppy seek test...\n");
    getchar();
    srand(biostime(0, 0));
    /* seek a few times */
    for (k = 0; k < 10; k++)
    {
        unsigned track = rand() % 80;
        printf("seeking to %d: ",track);
        if (fdc_seek(drive0.fdd, track) == 0) printf("OK\n");
                                         else printf("error!\n");
    }
#endif
    puts("Press Enter to start another floppy read test...\n");
    getchar();
    for (k = 0; k < 80*18*2; k++)
    {
        int res = floppy_read(&drive0, k, 1, buffer);
        if (res == FDC_NODISK) break;
        else if (res < 0) printf("Error on block %u\n", k);
        if (k % 36 == 0) printf("%llu: block %u read\n", uclock() * 1000 / UCLOCKS_PER_SEC, k);
    }
    printf("All tests completed.\n");
    return 0;
}
