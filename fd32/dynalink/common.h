#ifndef __COMMON_H__
#define __COMMON_H__

int common_relocate_section(struct kern_funcs *kf, DWORD base,
	struct table_info *tables, int n, struct section_info *s, int sect,
	struct symbol_info *syms, struct symbol *import);

DWORD common_import_symbol(struct kern_funcs *kf,
	int n, struct symbol_info *syms, char *name, int *sect);

DWORD common_load_executable(struct kern_funcs *kf, int file,
	struct table_info *tables, int n, struct section_info *s, DWORD *size);

DWORD common_load_relocatable(struct kern_funcs *kf, int f,
	struct table_info *tables, int n, struct section_info *s, DWORD *size);

void common_free_tables(struct kern_funcs *kf,
	struct table_info *tables, struct symbol_info *syms, struct section_info *scndata);


#endif /* __COMMON_H__ */
