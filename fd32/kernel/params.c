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

static fd32_param_t param_table[8] /* = { {0} } */ ;
#define PARAM_TABLE_SIZE (sizeof(param_table)/sizeof(fd32_param_t))


char *get_param(char *name)
{
  DWORD i;
  
  for (i = 0; i < PARAM_TABLE_SIZE; i++)
    if (param_table[i].name != NULL)
      if (strcmp(name, param_table[i].name) == 0)
        return param_table[i].value;

  return NULL;
}

void add_param(char *name, param_proc_t proc)
{
  DWORD i;
  /* Search for a empty entry and add */
  for (i = 0; i < PARAM_TABLE_SIZE; i++)
    if (param_table[i].name == NULL) {
      param_table[i].name = name;
      param_table[i].proc = proc;
      break;
    }
}

void set_param(char *name, char *value)
{
  DWORD i;
  
  /* Search for the param, if not found, choose a empty entry */
  for (i = 0; i < PARAM_TABLE_SIZE; i++)
    if (param_table[i].name != NULL) {
      if (strcmp(name, param_table[i].name) == 0) {
        param_table[i].name = name;
        param_table[i].value = value;
        /* Process the value */
        if (value != NULL && param_table[i].proc != NULL)
          param_table[i].proc(value);
        break;
      }
    }
}
