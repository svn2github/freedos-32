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
    unsigned k = 0;
    BYTE     buffer[512];
    unsigned long long start_time;

    fd32_event_init();
    if (fdc_setup(drive_detected) < 0) return -1;
    atexit(dispose);

    printf("Going to write some data in all sectors...\n");
    getchar();
    start_time = uclock();
    do
    {
        memset(buffer, k, sizeof(buffer));
        int res = floppy_write(&drive0, k, 1, buffer);
        if (res == FDC_NODISK) break;
        else if (res < 0) printf("Error on block %u\n", k);
        if (k % (drive0.fdd->fmt->sec_per_trk * drive0.fdd->fmt->heads) == 0)
            printf("%llu: block %u written\n", uclock() * 1000 / UCLOCKS_PER_SEC, k);
    }
    while (++k < drive0.fdd->fmt->size);
    printf("Write test elapsed in %llu ms\n", (uclock() - start_time) * 1000 / UCLOCKS_PER_SEC);
    puts("Going to read data from all sectors...\n");
    getchar();
    k = 0;
    start_time = uclock();
    do
    {
        BYTE check[512];
        memset(check, k, sizeof(check));
        int res = floppy_read(&drive0, k, 1, buffer);
        if (res == FDC_NODISK) break;
        else if (res < 0) printf("Error on block %u\n", k);
        if (k % (drive0.fdd->fmt->sec_per_trk * drive0.fdd->fmt->heads) == 0)
            printf("%llu: block %u read\n", uclock() * 1000 / UCLOCKS_PER_SEC, k);
        if (memcmp(buffer, check, sizeof(buffer)))
            printf("Block %u MISMATCH!\n", k);
    }
    while (++k < drive0.fdd->fmt->size);
    printf("Read test elapsed in %llu ms\n", (uclock() - start_time) * 1000 / UCLOCKS_PER_SEC);
    printf("All tests completed.\n");
    return 0;
}
