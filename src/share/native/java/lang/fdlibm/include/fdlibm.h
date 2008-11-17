/*
 * @(#)fdlibm.h	1.22 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.  
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER  
 *   
 * This program is free software; you can redistribute it and/or  
 * modify it under the terms of the GNU General Public License version  
 * 2 only, as published by the Free Software Foundation.   
 *   
 * This program is distributed in the hope that it will be useful, but  
 * WITHOUT ANY WARRANTY; without even the implied warranty of  
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU  
 * General Public License version 2 for more details (a copy is  
 * included at /legal/license.txt).   
 *   
 * You should have received a copy of the GNU General Public License  
 * version 2 along with this work; if not, write to the Free Software  
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  
 * 02110-1301 USA   
 *   
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa  
 * Clara, CA 95054 or visit www.sun.com if you need additional  
 * information or have any questions. 
 *
 */

/*
 * And see what the platform endianness is
 */
#include "javavm/include/porting/doubleword.h"
#include "javavm/include/porting/endianness.h"

/*
 * Include the mapping of names to CVM names, and the setting of the
 * mode to IEEE.  
 */
#include "jfdlibm.h"

typedef union {
    double d;
    int    i[2];
} CVMdiUnion;

#if (CVM_DOUBLE_ENDIANNESS == CVM_LITTLE_ENDIAN)
#define __HI(x) ((CVMdiUnion*)&x)->i[1]
#define __LO(x) ((CVMdiUnion*)&x)->i[0]
#define __HIp(x) ((CVMdiUnion*)x)->i[1]
#define __LOp(x) ((CVMdiUnion*)x)->i[0]
#else
#define __HI(x) ((CVMdiUnion*)&x)->i[0]
#define __LO(x) ((CVMdiUnion*)&x)->i[1]
#define __HIp(x) ((CVMdiUnion*)x)->i[0]
#define __LOp(x) ((CVMdiUnion*)x)->i[1]
#endif

#undef __P
#ifdef __STDC__
#define	__P(p)	p
#else
#define	__P(p)	()
#endif

/*
 * ANSI/POSIX
 */

extern int signgam;

#define	MAXFLOAT	((float)3.40282346638528860e+38)

enum CVMFdlibmVersion {CVMfdlibm_ieee = -1, CVMfdlibm_svid, CVMdlibm_xopen, CVMfdlibm_posix};

/*#define _LIB_VERSION_TYPE enum CVMFdlibmVersion*/
/*#define _LIB_VERSION CVMfdlibmVersion*/
#undef  _LIB_VERSION
#define _LIB_VERSION CVMfdlibm_ieee  /* always ieee for cvm */

/* if global variable _LIB_VERSION is not desirable, one may
 * change the following to be a constant by:
 *	#define _LIB_VERSION_TYPE const enum version
 * In that case, after one initializes the value _LIB_VERSION (see
 * s_lib_version.c) during compile time, it cannot be modified
 * in the middle of a program
 */
/*extern  _LIB_VERSION_TYPE  _LIB_VERSION;*/
#undef  _LIB_VERSION_TYPE
#define _LIB_VERSION_TYPE const enum version

#undef _IEEE_
#undef _SVID_
#undef _XOPEN_
#undef _POSIX_
#define _IEEE_  CVMfdlibm_ieee
#define _SVID_  CVMfdlibm_svid
#define _XOPEN_ CVMfdlibm_xopen
#define _POSIX_ CVMfdlibm_posix

struct CVMfdlibmException {
	int type;
	char *name;
	double arg1;
	double arg2;
	double retval;
};

#undef HUGE
#define	HUGE		MAXFLOAT

/*
 * set X_TLOSS = pi*2**52, which is possibly defined in <values.h>
 * (one may replace the following line by "#include <values.h>")
 */

#define X_TLOSS		1.41484755040568800000e+16

/* We need to #undef these because they conflict with math.h on win32 */
#undef	DOMAIN
#undef	SING
#undef	OVERFLOW
#undef	UNDERFLOW
#undef	TLOSS
#undef	PLOSS
#define	DOMAIN		1
#define	SING		2
#define	OVERFLOW	3
#define	UNDERFLOW	4
#define	TLOSS		5
#define	PLOSS		6

/*
 * ANSI/POSIX
 */
extern double acos __P((double));
extern double asin __P((double));
extern double atan __P((double));
extern double atan2 __P((double, double));
extern double cos __P((double));
extern double sin __P((double));
extern double tan __P((double));

extern double cosh __P((double));
extern double sinh __P((double));
extern double tanh __P((double));

extern double exp __P((double));
extern double frexp __P((double, int *));
extern double ldexp __P((double, int));
extern double log __P((double));
extern double log10 __P((double));
extern double modf __P((double, double *));

extern double pow __P((double, double));
extern double sqrt __P((double));

extern double ceil __P((double));
extern double fabs __P((double));
extern double floor __P((double));
extern double fmod __P((double, double));

extern double erf __P((double));
extern double erfc __P((double));
extern double gamma __P((double));
extern double hypot __P((double, double));
extern int isnan __P((double));
extern int finite __P((double));
extern double j0 __P((double));
extern double j1 __P((double));
extern double jn __P((int, double));
extern double lgamma __P((double));
extern double y0 __P((double));
extern double y1 __P((double));
extern double yn __P((int, double));

extern double acosh __P((double));
extern double asinh __P((double));
extern double atanh __P((double));
extern double cbrt __P((double));
extern double logb __P((double));
extern double nextafter __P((double, double));
extern double remainder __P((double, double));
#ifdef _SCALB_INT
extern double scalb __P((double, int));
#else
extern double scalb __P((double, double));
#endif

extern int matherr __P((struct CVMfdlibmException *));

/*
 * IEEE Test Vector
 */
extern double significand __P((double));

/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
extern double copysign __P((double, double));
extern int ilogb __P((double));
extern double rint __P((double));
extern double scalbn __P((double, int));

/*
 * BSD math library entry points
 */
extern double expm1 __P((double));
extern double log1p __P((double));

/*
 * Reentrant version of gamma & lgamma; passes signgam back by reference
 * as the second argument; user must allocate space for signgam.
 */
#ifdef _REENTRANT
extern double gamma_r __P((double, int *));
extern double lgamma_r __P((double, int *));
#endif	/* _REENTRANT */

/* ieee style elementary functions */
extern double __ieee754_sqrt __P((double));
extern double __ieee754_acos __P((double));
extern double __ieee754_acosh __P((double));
extern double __ieee754_log __P((double));
extern double __ieee754_atanh __P((double));
extern double __ieee754_asin __P((double));
extern double __ieee754_atan2 __P((double,double));
extern double __ieee754_exp __P((double));
extern double __ieee754_cosh __P((double));
extern double __ieee754_fmod __P((double,double));
extern double __ieee754_pow __P((double,double));
extern double __ieee754_lgamma_r __P((double,int *));
extern double __ieee754_gamma_r __P((double,int *));
extern double __ieee754_lgamma __P((double));
extern double __ieee754_gamma __P((double));
extern double __ieee754_log10 __P((double));
extern double __ieee754_sinh __P((double));
extern double __ieee754_hypot __P((double,double));
extern double __ieee754_j0 __P((double));
extern double __ieee754_j1 __P((double));
extern double __ieee754_y0 __P((double));
extern double __ieee754_y1 __P((double));
extern double __ieee754_jn __P((int,double));
extern double __ieee754_yn __P((int,double));
extern double __ieee754_remainder __P((double,double));
extern int    __ieee754_rem_pio2 __P((double,double*));
#ifdef _SCALB_INT
extern double __ieee754_scalb __P((double,int));
#else
extern double __ieee754_scalb __P((double,double));
#endif

/* fdlibm kernel function */
extern double __kernel_standard __P((double,double,int));
extern double __kernel_sin __P((double,double,int));
extern double __kernel_cos __P((double,double));
extern double __kernel_tan __P((double,double,int));
extern int    __kernel_rem_pio2 __P((double*,double*,int,int,int,const int*));
