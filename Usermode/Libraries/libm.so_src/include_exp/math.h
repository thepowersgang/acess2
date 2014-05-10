/*
 * Acess2 C Library (Maths)
 * - By John Hodge (thePowersGang)
 *
 * math.h
 * - C99 Header
 */
#ifndef _LIBM__MATH_H_
#define _LIBM__MATH_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef float	float_t;
typedef double	double_t;


#define INFINITY	(*(float*)((uint32_t[]){0x78000000}))
#define NAN	(*(float*)((uint32_t[]){0x78000001}))

extern double	pow(double x, double y);
extern double	exp(double x);
extern double	log(double val);

#ifdef __cplusplus
}
#endif

#endif
