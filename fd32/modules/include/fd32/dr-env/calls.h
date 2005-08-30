#ifndef __FD32_CALLS__
#define __FD32_CALLS__
/* Prototypes of the FD32 kernel calls... */

int  message(char *fmt,...) __attribute__((format(printf,1,2)));
void fd32_abort(void);
void restore_sp(int res);
int mem_get_region(DWORD base, DWORD size);
DWORD mem_get(DWORD amount);
int mem_free(DWORD base, DWORD size);
void idt_place(BYTE num,void (*handler)(void));
DWORD gdt_read(WORD sel,DWORD *lim,BYTE *acc,BYTE *gran);

int fd32_allocate_descriptors(WORD NumSelectors, WORD *BaseSelector);
int fd32_free_descriptor(WORD Selector);
WORD fd32_get_selector_increment_value();
int fd32_get_segment_base_address(WORD Selector, DWORD *BaseAddress);
int fd32_set_segment_base_address(WORD Selector, DWORD BaseAddress);
int fd32_set_segment_limit(WORD Selector, DWORD Limit);
int fd32_set_descriptor_access_rights(WORD Selector, WORD Rights);
int fd32_create_alias_descriptor(WORD Selector, WORD *NewSelector);
int fd32_get_descriptor(WORD Selector, WORD BufferSelector, DWORD BufferOffset);
int fd32_set_descriptor(WORD Selector, WORD BufferSelector, DWORD BufferOffset);

int vm86_callBIOS(int service,X_REGS16 *in,X_REGS16 *out,X_SREGS16 *s);
#endif
