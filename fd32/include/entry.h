

#define SEGDECL1(s) \
extern void* entseg_##s##_s;

#define SEGDECL2(s) \
extern void* entseg_##s##_s;

#define SEGDECL3(s) \
extern void* entseg_##s##_s;

#define SEGDECL4(s) \
extern void* entseg_##s##_s;


SEGDECL1(saveES_DS)
SEGDECL1(save_newFS)
SEGDECL1(save_newGS)
SEGDECL1(restES_DS)
SEGDECL1(restFS)
SEGDECL1(restGS)
SEGDECL1(new_seg)
SEGDECL1(same_stack)
SEGDECL2(if_first_ent)
SEGDECL2(if_first_leave)
SEGDECL2(new_stack_1)
SEGDECL2(new_stack_2)
SEGDECL1(save_stack_1)
SEGDECL1(rest_stack_1)
SEGDECL1(save_stack_2)
SEGDECL1(rest_stack_2)
SEGDECL1(save_stack_3)
SEGDECL1(rest_stack_3)
SEGDECL1(save_stack_4)
SEGDECL1(rest_stack_4)
SEGDECL1(nonspecific_EOI_1)
SEGDECL1(nonspecific_EOI_2)
SEGDECL1(specific_EOI_1)
SEGDECL1(specific_EOI_2)
SEGDECL1(disable_slave)
SEGDECL1(disable_master)
SEGDECL1(enable_slave)
SEGDECL1(enable_master)
SEGDECL1(delay)
SEGDECL1(cli)
SEGDECL1(sti)
SEGDECL2(call_direct)
SEGDECL2(call_indir_1)
SEGDECL2(irq_count_1)
SEGDECL3(irq_count_2)
SEGDECL2(inc_1)
SEGDECL3(inc_2)
SEGDECL3(or_mask)
SEGDECL3(if_irr_set)

struct handler_segment
{
	void* segm_pt;
	char c;
	char flags;
	signed char stack;
};

