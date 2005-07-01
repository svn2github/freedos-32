
typedef void (*param_proc_t)(char *value);

char *get_param(char *name);
void set_param(char *name, char *value, param_proc_t proc);
