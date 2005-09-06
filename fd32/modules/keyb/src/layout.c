/* Keyborad Driver for FD32 - Layout support
 * - Using KEY file pack for FD-KEYB
 * - The file's structure to parse is based on KC of FD-KEYB
 *
 * by Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
 */

#include "layout.h"
#include <filesys.h>


static KeybCB cb;


int keyb_layout_decode(BYTE c, int flags, int lock)
{
	if (cb.ref != NULL) {
		KeyType *blk;
		BYTE *p = (BYTE *)cb.ref+cb.submaps[0].keysofs;

		for (blk = (KeyType *)p; blk->scancode != 0; p += sizeof(KeyType)+(blk->flags.data_length), blk = (KeyType *)p) {
			if (blk->scancode == c)
				return blk->data[0];
			/*
			fd32_message("scancode %x flags %x cmd %x data %x %x\n",
				blk->scancode, blk->flags, blk->cmdbits, blk->data[0], blk->data[1]);
			*/
		}
	}

	return -1;
}


int keyb_layout_choose(const char *filename, char *layout_name)
{
	int ret = -1;
	KHeader kheader;
	KLHeader klheader;
	struct kernel_file f;
	int fileid = (int)&f;

	if (fd32_kernel_open(filename, O_RDONLY, 0, 0, &f) >= 0)
	{
		DWORD keybcb;

		/* The keyboard library header */
		fd32_kernel_read(fileid, &kheader, SIZEOF_KHEADER);
		#ifdef __KEYB_DEBUG__
		fd32_log_printf("[KEYB] KC header:\n\tSig %c%c%c\n\tver %x\n\tlength %x\n", kheader.Sig[0], kheader.Sig[1], kheader.Sig[2], *((WORD *)kheader.version), kheader.length);
		#endif
		/* The keyboard library description */
		kheader.description = (BYTE *)mem_get(kheader.length+1);
		fd32_kernel_read(fileid, kheader.description, kheader.length);
		kheader.description[kheader.length-1] = 0;
		fd32_message("KC description: %s\n", kheader.description);
		mem_free((DWORD)kheader.description, kheader.length+1);

		for ( ; fd32_kernel_read(fileid, &klheader, SIZEOF_KLHEADER) > 0; ) {
			#ifdef __KEYB_DEBUG__
			fd32_log_printf("[KEYB] KL size: %x length: %x\n", klheader.klsize, klheader.length);
			#endif
			klheader.id = (BYTE *)mem_get(klheader.length);
			fd32_kernel_read(fileid, klheader.id, klheader.length);
			if (toupper(klheader.id[2]) == toupper(layout_name[0]) &&
				toupper(klheader.id[3]) == toupper(layout_name[1])) {
				cb.size = klheader.klsize-klheader.length;
				keybcb = mem_get(cb.size);
				cb.ref = (void *)keybcb;

				fd32_kernel_read(fileid, cb.ref, cb.size);
				cb.submaps = (SubMapping *)(keybcb+SIZEOF_CB);
				cb.planes = (Plane *)(keybcb+SIZEOF_CB+cb.ref->submap_num*sizeof(SubMapping));
				#ifdef __KEYB_DEBUG__
				/*	fd32_message("KL id: ");
					print_kl_id(klheader.id, klheader.length); */
				fd32_log_printf("[KEYB] KeybCB: next %x:\n\tSubmap_num %d\n\tPlane_num %d\n\tDec_sep %x\n\tCur_submap %d\n\tMCBID %x\n",
					cb.ref->next, cb.ref->submap_num, cb.ref->plane_num, cb.ref->dec_sep, cb.ref->cur_submap, cb.ref->mcbid);
				fd32_log_printf("[KEYB] Codepage: ");
				for (i = 0; i < cb.ref->submap_num; i++)
					fd32_log_printf("[KEYB] %d ", cb.submaps[i].codepage);
				fd32_log_printf("\n");
				#endif
				mem_free((DWORD)klheader.id, klheader.length);
				ret = 0;
				break;
			} else {
				/* Seek to the next KL */
				fd32_kernel_seek(fileid, klheader.klsize-klheader.length, FD32_SEEKCUR);
				mem_free((DWORD)klheader.id, klheader.length);
			}
		}

		fd32_kernel_close(fileid);
	}

	return ret;
}

int keyb_layout_free(void)
{
	if (cb.ref != NULL)
		return mem_free((DWORD)cb.ref, cb.size);
	else
		return -1;
}
