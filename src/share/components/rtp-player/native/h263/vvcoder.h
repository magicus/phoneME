/*-----------------------------------------------------------------------------
 *  CODER.H
 *
 *      DESCRIPTION
 *              Header file for coder.c
 *
 *      Author:         Mary Deshon     7/08/93
 *      Inspector:      <<not inspected yet>>
 *      Revised:
 *  06/21/94 mjf    Null out WIN16/VxD pointer conversions.
 *
 *      (c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/
#ifndef _INC_CODER
#define _INC_CODER      1

#if defined (FLAT_OS) || defined(_WINNT_)

#       define makeFlat32(ptr) (ptr)
#       define makeAlias16(ptr) (ptr)
#       define makeHugeAlias16(ptr, size) (ptr)
#       define myFreeAlias16(ptr)
#       define myFreeHugeAlias16(ptr, size)

#else // (! FLAT_OS) && (! _WINNT_)

extern void far *makeFlat32( void *fp16 );
extern U32 makeAlias16( void *ptr );
extern U32 makeHugeAlias16( void *ptr , U32 size);
extern void myFreeAlias16( U32 fp16 );
extern void myFreeHugeAlias16( U32 fp16 , U32 size);

#endif // (! FLAT_OS) && (! _WINNT_)

#endif
