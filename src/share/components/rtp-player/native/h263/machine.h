/*-----------------------------------------------------------------------------
 *  machine.h
 *
 *  NAME 
 *   Machine-dependent definitions.
 *   WIN16
 *   __WINDOWS_386__, VXD
 *   FLAT_OS
 *   _NTWIN
 *   _OS2
 *
 *  SYNOPSIS
 *    #include "machine.h"
 *     ...
 *
 *  DESCRIPTION   
 *
 *  Author:     Ollie Jones
 *  Inspector:
 *  Revised:
 *	01.27/97 jr		Added the VvAssert() macro
 *  01/20/96 aek put back definitions for BITMAPINFOHEADER and RGBQUAD
 *  02/02/96 Andreas Wanka added VC4++ support for Pentium FPU save/restore
 *  03/15/95 cjg     OS/2 only - removed EnterCritSec and DisableCritSec macros
 *                               Changed CLEAR_DIRECTION_FLAG macro
 *  02/16/95 cjg     moved DISABLE_INTERRRUPTS etc, here for OS/2 target
 *  12/01/94 cjg     Added _OS2 target
 *  06/20/94 mjf     Added support for flat model operating systems.
 *  04/18/94 wolfe   Added "defined" to "#elif VXD" on line 113 (for WATCOM)
 *  04/14/94 David Markun markun@world.std.com   Support VxD version.
 *  03/14/94 Bruder  define macro for clearing the direction flag.
 *  9/29/93 oj       fix bug 26; make sure interval computation wraps correctly;
 *                   change interval data type to a signed data type.
 *  10/12/93 oj      fix bug 42; add INTERRUPT_DISABLE and INTERRUPT_RESTORE macros
 *
 *  (c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#ifndef _MACHINE_H_
#define _MACHINE_H_

#ifndef _OS_H_
#include "os.h" //We need it so can tell ifdef WIN16
#endif

#define VvDELETE(OBJ) if (OBJ) { delete(OBJ); OBJ=0; }

// define the VvAssert() macro as appropriate for each platform
#ifdef _DEBUG

#ifdef FOR_MAC
#include <Assert.h>
#define VvAssert(expr)	Assert_(expr)
#else		// other platforms go here
#define VvAssert(expr)
#endif

#else	// #ifdef _DEBUG

#define VvAssert(expr)

#endif	// #ifdef _DEBUG

#ifdef FOR_MAC
#define PATH_SEPARATOR ":"
#endif
#ifdef FOR_WINDOWS
#define PATH_SEPARATOR "\\"
#endif
#ifdef FOR_UNIX
#define PATH_SEPARATOR "/"
#endif

#ifdef FOR_MAC
// Define basic Windows datatypes here
#define BOOL Boolean
#ifdef FAR
#undef FAR
#endif
#define FAR
typedef unsigned char * LPBYTE;
#endif

#ifdef FOR_UNIX
// Define basic Windows datatypes here
#define BOOL unsigned char

#ifdef FAR
#undef FAR
#endif
#define FAR

#ifndef NULL
#define NULL (0L)
#endif

#ifndef pascal
#define pascal
#endif

#endif



/* numeric types */
typedef signed char S8;
typedef unsigned char U8;

typedef short int S16;
typedef unsigned short int U16;

typedef long int S32;
typedef unsigned long int U32;
typedef float	FLOAT;

#ifdef FOR_MAC
typedef unsigned short WORD;
typedef S8 BYTE;
typedef unsigned long DWORD;
typedef S32 LONG;
//typedef unsigned long TIME32;
#endif

typedef S16 Word16;     // used by G723 audio codec
typedef S32 Word32;     // used by G723 audio codec

/* graphic types */
typedef U8       PIXEL; 

/* pointer types (here lie dragons!) */ 

#if defined( __WINDOWS_386__) || defined(__WATCOMC__)          //For Watcom 32-bit DLLs

#define P32NEAR *
#define HUGE32 *
#define P32 *
#define P48 _far *      
#define P16 (cant do this in 32-bit)

#elif defined(FLAT_OS)

#       ifdef __segname
#               undef __segname
#       endif
#       define __segname

// IBMC doesnt like this
//#     ifdef __export
//#             undef __export
//#     endif
//#     define __export

#       define P32NEAR *
#       define HUGE32 *
#       define P32 *
#       define P48 *
#       define P16 *

// IBMC doesnt like this
//#     ifdef far
//#             undef far
//#     endif
//#     define far

// IBMC doesnt like this
//#     ifdef _far
//#             undef _far
//#     endif
//#     define _far

#       ifdef FAR
#               undef FAR
#               define FAR
#       endif

#elif defined VXD                       //For Watcom 32-bit VXDs

#define P32NEAR *
#define HUGE32 *
#define P32 *
#define P48 __far *     // far and _far mean nothing, but __far means __far
#define P16 (cant do this in 32-bit)
//Undefine far, _far, and FAR
#ifdef far
        #undef far
#endif//far
#ifdef _far
        #undef _far
#endif//_far
#ifdef FAR
                #undef FAR
#endif
//Define far, _far, and FAR to nothing
#define far             //Nothing
#define _far    //Nothing
#define FAR             //Nothing

#elif defined WIN16                                             //For ordinary 16-bit Windows code

#define P32NEAR (cant do this in 16-bit) 
#define HUGE32 _huge *
#define P32 _far *
#define P48 (cant do this in 16-bit)
#define P16 *

#elif defined _OS2
    // nothing for OS2
#else
        #HEY, unknown environment type
#endif

/* Boolean types */
typedef U16 B16;


#define NATURAL_ALIGN 16
#define ALIGN(len,align) ((len)+(align)-1) & (~(align-1)) 

/* Metric types */
typedef U32 TIME32;         /* absolute time */
typedef S32 INTERVAL32;     /* interval time */
                            
#ifdef WIN16
        /*
         * interrupt handling macros; specific to win 16 / x86
         */
        #define DISABLE_INTERRUPTS(a) \
                _asm pushf\
                _asm pop a\
                _asm cli
        #define RESTORE_INTERRUPTS(a) {if (a & 0x200) {_asm sti}}


        #define CLEAR_DIRECTION_FLAG()  _asm cld  

#elif defined(_NTWIN)

        // Note:  NT critical sections allow singular resource access on a per
        // thread basis only, not per process.

#       define DISABLE_INTERRUPTS(a) VvEnterCriticalSection()
#       define RESTORE_INTERRUPTS(a) VvLeaveCriticalSection()
#       define _disable() VvEnterCriticalSection()
#       define _enable() VvLeaveCriticalSection()

#       ifdef _X86_
#               define CLEAR_DIRECTION_FLAG()  _asm cld
#       endif

#elif defined (WIN95_VXD)
        /*
         * interrupt handling macros specific to Ring 0 and MSVC compiler
		 *  We ignore the argument 'a'.
         */
        #define DISABLE_INTERRUPTS(a) \
                _asm pushfd\
                _asm cli
        #define RESTORE_INTERRUPTS(a) _asm popfd


        #define CLEAR_DIRECTION_FLAG()  _asm cld  


		/* functions to store and restore the FPU environment and 
		 * stack, we must care about it in the VxD */

		typedef U8 FPUCONTEXT[120] ; /* 94 for 16-bit segments and 108 for 32-bit segments */
		#define StoreFPUContextVC(LP) __asm \
		{\
		__asm fsave dword ptr [LP] \
		}

		#define RestoreFPUContextVC(LP) __asm \
		{\
		__asm frstor dword ptr [LP] \
		}

		//Get and clear the x86 (Task Switch Flag)	
		#define GetAndClearTsFlagVC(LP) __asm \
		{\
		__asm push eax \
		__asm mov eax, cr0 \
		__asm and eax, 8 \
		__asm mov dword ptr [LP], eax \
		__asm clts \
		__asm pop eax \
		}
		
		/* setting the TS bit provoques a NM interrupt on next floating point 
		 * instruction */
		#define RestoreTsFlagVC(LP) __asm \
		{\
		__asm push eax \
		__asm mov eax, cr0 \
		__asm or eax, dword ptr [LP] \
		__asm mov cr0, eax \
		__asm pop eax \
		}

#elif defined (_WIN32)

        /*
         * interrupt handling macros; HEY AREN"T THESE A NO-OP IN WIN32??
         */
        #define DISABLE_INTERRUPTS(a) \
                _asm pushf\
                _asm pop a\
                _asm cli
        #define RESTORE_INTERRUPTS(a) {if (a & 0x200) {_asm sti}}


        #define CLEAR_DIRECTION_FLAG()  _asm cld  

#elif defined VXD		//Watcom
        void PushFlagsAndCli(void);
        #pragma aux PushFlagsAndCli = "pushfd","cli";
        void PopFlags(void);
        #pragma aux PopFlags = "popfd";
        #define CLEAR_DIRECTION_FLAG() ClearDirectionFlag()
        #define DISABLE_INTERRUPTS(a) (a=a,PushFlagsAndCli()) //[Dummy ref a]
        #define RESTORE_INTERRUPTS(a) (a=a,PopFlags()) //[Dummy ref a]

#elif defined __WINDOWS_386__
        //Nothing for now; might could use the same as VXD

#elif defined _OS2
/* Note: this api blocks other threads in the process from
 * executing.  Its not clear this includes ISRs, so if this
 * proves ineffectual, we will add an HMTX to the QueueHeader
 */
#define DISABLE_INTERRUPTS(a) DosEnterCritSec()
#define RESTORE_INTERRUPTS(a) DosExitCritSec()
#define CLEAR_DIRECTION_FLAG() ClearDirectionFlag()

#elif defined _WIN32
#define FLAT_OS
#elif defined FOR_MAC
#define FLAT_OS
#elif defined FOR_UNIX
#define FLAT_OS
#else
        #HEY, unknown environment type

#endif

#if defined(FOR_MAC) || defined(FOR_UNIX)
typedef struct tagBITMAPINFOHEADER{
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER, FAR *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD;
typedef RGBQUAD FAR* LPRGBQUAD;


#endif

#endif // _MACHINE_H_
