#ifndef STD_TYPES_H
#define STD_TYPES_H
#include <stdint.h>

typedef uint8_t Std_ReturnType;
typedef uint8_t boolean;

#ifndef E_OK
#define E_OK      ((Std_ReturnType)0U)
#endif
#ifndef E_NOT_OK
#define E_NOT_OK  ((Std_ReturnType)1U)
#endif
#ifndef TRUE
#define TRUE      ((boolean)1U)
#endif
#ifndef FALSE
#define FALSE     ((boolean)0U)
#endif
#endif
