#include <ll/i386/hw-data.h>
#include <ll/stdlib.h>
#include <ll/string.h>
#include "params.h"

/* NOTE: UNUSED
typedef enum param_type
{
  INT_8_T = 0,
  INT_16_T,
  INT_32_T,
  INT_64_T,
  UINT_8_T,
  UINT_16_T,
  UINT_32_T,
  UINT_64_T,
  STR_T,
  NONE = -1,
} param_type_t;
*/

typedef struct fd32_param
{
  char *name;
  char *value;
  param_proc_t proc;
} fd32_param_t;

static fd32_param_t param_table[8] = {
  0
};
#define PARAM_TABLE_SIZE (sizeof(param_table)/sizeof(fd32_param_t))


char *get_param(char *name)
{
  DWORD i;
  
  for (i = 0; i < PARAM_TABLE_SIZE; i++)
  {
    if (param_table[i].name != NULL) {
      if (strcmp(name, param_table[i].name) == 0) {
        return param_table[i].value;
      }
    }
  }
  
  return 0;
}

void set_param(char *name, char *value, param_proc_t proc)
{
  DWORD i, sel_i;
  
  /* Search for the param, if not found, choose a empty entry */
  for (i = 0, sel_i = PARAM_TABLE_SIZE; i < PARAM_TABLE_SIZE; i++)
  {
    if (param_table[i].name != NULL) {
      if (strcmp(name, param_table[i].name) == 0) {
        sel_i = i;
        break;
      }
    } else {
      sel_i = i;
    }
  }
  
  if (sel_i < PARAM_TABLE_SIZE) {
    if (param_table[sel_i].name == NULL)
      param_table[sel_i].name = name;
    if (value != NULL)
      param_table[sel_i].value = value;
    if (proc != NULL)
      param_table[sel_i].proc = proc;

    /* Process the value */
    if (value != NULL && param_table[sel_i].proc != NULL)
      param_table[sel_i].proc(value);
  }
}
