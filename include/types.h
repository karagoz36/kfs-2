/*
** types.h — the kernel's own basic types.
**
** The kernel is built freestanding (-nostdlib), so standard headers such as
** <stdint.h> are unavailable. On i386 (32-bit protected mode) the sizes are
** fixed: char 1, short 2, int 4 bytes.
*/

#ifndef TYPES_H
#define TYPES_H

/* Sizes are what the hardware expects: a scancode is one byte, a VGA cell is
** two, an address is four. */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef signed int         int32_t;

/* A pointer is 4 bytes on i386, hence size_t = unsigned int */
typedef unsigned int       size_t;

/* Minimal boolean; C99 <stdbool.h> is not available */
typedef uint8_t            bool_t;
#define TRUE  1
#define FALSE 0

#define NULL ((void *)0)

#endif
