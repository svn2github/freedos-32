#ifndef __COMMON_H__
#define __COMMON_H__

int common_relocate_section(struct kern_funcs *kf,
	DWORD base, DWORD bssbase, struct section_info *s,
	int sect, struct symbol_info *syms, struct symbol *import);

DWORD common_import_symbol(struct kern_funcs *kf,
	int n, struct symbol_info *syms, char *name, int *sect);

DWORD common_load_executable(struct kern_funcs *kf,
	int file, int n, struct section_info *s, int *size);

DWORD common_load_relocatable(struct kern_funcs *kf,
	int f, int n, struct section_info *s, int *size);

#endif /* __COMMON_H__ */
