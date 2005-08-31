#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "layout.h"

int keyb_choose_layout(const char *filename, char *layout_name)
{
	DWORD i;
	KHeader kheader;
	KLHeader klheader;
	KeybCB cb;
	FILE *fp = fopen(filename, "rb");

	if (fp != NULL)
	{
		KeyType *blk;
		BYTE *keybcb;
		BYTE *p;

		printf("Read the KC file\n");
		fread(&kheader, SIZEOF_KHEADER, 1, fp);
		printf("KC header:\n\tSig %c%c%c\n\tver %x\n\tlength %x\n",
			kheader.Sig[0], kheader.Sig[1], kheader.Sig[2],
			*((WORD *)kheader.version),
			kheader.length);
		kheader.description = malloc(kheader.length+1);
		fread(kheader.description, sizeof(BYTE), kheader.length, fp);
		kheader.description[kheader.length-1] = 0;
		printf("KC description: %s\n", kheader.description);

		for ( fread(&klheader, SIZEOF_KLHEADER, 1, fp); !feof(fp); fread(&klheader, SIZEOF_KLHEADER, 1, fp)) {
		printf("Read KL Header\n");
		
		printf("KL size: %x length: %x\n", klheader.klsize, klheader.length);
		klheader.id = malloc(klheader.length);
		fread(klheader.id, sizeof(BYTE), klheader.length, fp);
		printf("KL id: ");
		print_kl_id(klheader.id, klheader.length);

		printf("Read KeybCB\n");
		keybcb = malloc(klheader.klsize-klheader.length);
		if (klheader.id[2] == layout_name[0] && klheader.id[3] == layout_name[1]) {
		fread(keybcb, sizeof(BYTE), klheader.klsize-klheader.length, fp);
		cb.ref = keybcb;

		printf("KeybCB: next %x:\n\tSubmap_num %d\n\tPlane_num %d\n\tDec_sep %x\n\tCur_submap %d\n\tMCBID %x\n",
			cb.ref->next,
			cb.ref->submap_num,
			cb.ref->plane_num,
			cb.ref->dec_sep,
			cb.ref->cur_submap,
			cb.ref->mcbid);
		cb.submaps = keybcb+SIZEOF_CB;
		cb.planes = keybcb+SIZEOF_CB+cb.ref->submap_num*sizeof(SubMapping);
		printf("Codepage: ");
		for (i = 0; i < cb.ref->submap_num; i++)
			printf("%d ", cb.submaps[i].codepage);
		puts("");
		printf("Submap_0 table offset: %x\n", cb.submaps[0].keysofs);

		p = keybcb+cb.submaps[0].keysofs;
		for (blk = (KeyType *)p; blk->scancode != 0; p += sizeof(KeyType)+(blk->flags.data_length), blk = (KeyType *)p) {
			printf("scancode %x flags %x cmd %x data %x %x\n", blk->scancode, blk->flags, blk->cmdbits, blk->data[0], blk->data[1]);
		}
		} else {
            fseek (fp, klheader.klsize-klheader.length, SEEK_CUR);
		}
		
		}
	}
	return 1;
}
