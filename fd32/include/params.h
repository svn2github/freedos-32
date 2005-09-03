#ifndef __PARAMS_H__
#define __PARAMS_H__

typedef void (*param_proc_t)(char *value);

char *get_param(char *name);
void add_param(char *name, param_proc_t proc);
void set_param(char *name, char *value);

/* Add a param with its process procedure and
	then call set_param when a new value got */

#endif
