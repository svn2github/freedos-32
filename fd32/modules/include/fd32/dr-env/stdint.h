/* A simple stdint.h file for use in FreeDOS-32.
 * by Salvo Isaja, 2005/05/07
 */
#ifndef __STDINT_H
#define __STDINT_H

/* FIXME: The following types clash with oslib/ll/sys/types.h */
//typedef  signed char         int8_t;
//typedef  short               int16_t;
//typedef  int                 int32_t;
typedef  long long           int64_t;
typedef  unsigned char       uint8_t;
typedef  unsigned short      uint16_t;
typedef  unsigned            uint32_t;
typedef  unsigned long long  uint64_t;
#define INT8_MIN   (-0x80)
#define INT8_MAX   ( 0x7F)
#define INT16_MIN  (-0x8000)
#define INT16_MAX  ( 0x7FFF)
#define INT32_MIN  (-0x80000000)
#define INT32_MAX  ( 0x7FFFFFFF)
#define INT64_MIN  (-0x8000000000000000ll)
#define INT64_MAX  ( 0x7FFFFFFFFFFFFFFFll)
#define UINT8_MAX  (0xFFu)
#define UINT16_MAX (0xFFFFu)
#define UINT32_MAX (0xFFFFFFFFu)
#define UINT64_MAX (0xFFFFFFFFFFFFFFFFull)

#endif /* #ifndef __STDINT_H */
