
#include "layout.h"
#include <filesys.h>


int keyb_choose_layout(const char *filename, char *layout_name)
{
	DWORD i;
	KHeader kheader;
	KLHeader klheader;
	KeybCB cb;
	struct kernel_file f;
	int fileid = (int)&f;

	if (fd32_kernel_open(filename, O_RDONLY, 0, 0, &f) >= 0)
	{
		KeyType *blk;
		BYTE *keybcb, *p;

		fd32_kernel_read(fileid, &kheader, SIZEOF_KHEADER);
		fd32_message("KC header:\n\tSig %c%c%c\n\tver %x\n\tlength %x\n", kheader.Sig[0], kheader.Sig[1], kheader.Sig[2], *((WORD *)kheader.version), kheader.length);
		kheader.description = mem_get(kheader.length+1);
		fd32_kernel_read(fileid, kheader.description, kheader.length);
		kheader.description[kheader.length-1] = 0;
		fd32_message("KC description: %s\n", kheader.description);

		for ( ; fd32_kernel_read(fileid, &klheader, SIZEOF_KLHEADER) > 0; ) {
			fd32_message("KL size: %x length: %x\n", klheader.klsize, klheader.length);
			klheader.id = mem_get(klheader.length);
			fd32_kernel_read(fileid, klheader.id, klheader.length);
		//	fd32_message("KL id: ");
		//	print_kl_id(klheader.id, klheader.length);
			fd32_message("Read KeybCB\n");
			keybcb = mem_get(klheader.klsize-klheader.length);
			if (klheader.id[2] == layout_name[0] && klheader.id[3] == layout_name[1]) {
				fd32_kernel_read(fileid, keybcb, klheader.klsize-klheader.length);
				cb.ref = keybcb;
				fd32_message("KeybCB: next %x:\n\tSubmap_num %d\n\tPlane_num %d\n\tDec_sep %x\n\tCur_submap %d\n\tMCBID %x\n", cb.ref->next, cb.ref->submap_num, cb.ref->plane_num, cb.ref->dec_sep, cb.ref->cur_submap, cb.ref->mcbid);
				cb.submaps = keybcb+SIZEOF_CB;
				cb.planes = keybcb+SIZEOF_CB+cb.ref->submap_num*sizeof(SubMapping);
				fd32_message("Codepage: ");
				for (i = 0; i < cb.ref->submap_num; i++)
					fd32_message("%d ", cb.submaps[i].codepage);
				fd32_message("\n");
				fd32_message("Submap_0 table offset: %x\n", cb.submaps[0].keysofs);
				p = keybcb+cb.submaps[0].keysofs;
				for (blk = (KeyType *)p; blk->scancode != 0; p += sizeof(KeyType)+(blk->flags.data_length), blk = (KeyType *)p) {
					fd32_message("scancode %x flags %x cmd %x data %x %x\n", blk->scancode, blk->flags, blk->cmdbits, blk->data[0], blk->data[1]);
				}
			} else {
				fd32_kernel_seek(fileid, klheader.klsize-klheader.length, FD32_SEEKCUR);
			}
		}
	}
	return 1;
}
