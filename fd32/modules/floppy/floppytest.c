#include <dr-env.h>
#include <block/block.h>

static uint8_t buf[512*100];

void floppytest_init(void)
{
	BlockOperations *bops;
	void *pu;
	void *handle;
	int k, j;
	int res = block_get("fd0", BLOCK_OPERATIONS_TYPE, &pu, &handle);
	message("block_get: %08xh\n", res);
	bops = (BlockOperations *) pu;
	res = bops->open(handle);
	message("bops->open: %08xh\n", res);
	for (k = 0; k < 3; k++)
	{
		res = bops->test_unit_ready(handle);
		if (BLOCK_IS_ERROR(res) && (BLOCK_GET_SENSE(res) == BLOCK_SENSE_ATTENTION))
		{
			message("bops->revalidate... ");
			res = bops->revalidate(handle);
			message("done: %08xh\n", res);
		}
		message("Floppy test round #%i\n", k);
		res = bops->read(handle, buf, 0, 1, 0);
		message("bops->read: %08xh\n", res);
		message("Delay...\n");
		for (j = 0; j < 100000000; j++);
	}
	//for (k = 0; k < res*512; k++) fd32_outb(0xE9, buf[k]);
	res = bops->request(REQ_RELEASE, handle);
	message("REQ_RELEASE: %i\n", res);
}
