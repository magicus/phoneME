/*-----------------------------------------------------------------------------
 *  os.h
 *  $Header: /Projects/ICM/ver2.0/ACM/include/os.h 29    10/17/97 4:39p Dblair $
 *
 *  NAME 
 *   Operating-system-dependent definitions.
 *      This version supports Win 3.1, which uses several sub-environments:
 *          32-bit Ring 0 VxD code          #define VXD (on cmdline)
 *          16-bit Ring 3 code              #define WIN16 (done here)
 *          32-bit Watcom Ring 3 code       #define __WINDOWS_386__ (cmdline)
 *
 *   It also supports OS/2 Warp             #define _OS2 (on cmdline)
 *   and windows NT                         #define _NTWIN
 *   and 32 bits in general                 #define FLAT_OS
 *   NOTE: for OS/2 you need to define both _OS2 and FLAT_OS
 *
 *  SYNOPSIS
 *    #include "os.h"
 *     ...
 *
 *  DESCRIPTION   
 *
 *  Author:     Ollie Jones
 *  Inspector:
 *  Revised (most recent first):
 *  11/02/97  aw            defined FOR_WINDOWS for watcom
 *  12/30/96  jb             removed LIVE_CONNECT variable from Mac build
 *  12/12/96  oj            Add LIVE_CONNECT variable
 *  06/06/96  tkent         Add Macintosh support.
 *  012/02/96 bruder        Don't define NOLOCAL and NOPRIVATE for VxDs by default.  
 *                          This can cause defects in programs which use static data/functions.
 *  06/14/95 chet           OS/2 implementations of VIVODLL and INTERRUPT_xxx macros
 *  04/19/95 bruder         Added macros for flat audio pointers.
 *  01/12/95 chet           Moved all OS/2 extensions to OS2EXT.H
 *  12/21/94 chet           Added _OS2 support
 *  12/14/94 jb           Added mcLpEchoData support
 *  11/28/94 wc             Added support for external name mangling.
 *  09/27/94 wc             Changed INI file name to vivo320.ini
 *  06/20/94 mjf            Added support for flat model operating systems.
 *  6/1/94   Bruder         Added movieman support
 *  05/20/94 wolfe          Added General Section Hdr VIVOPROFILE String
 *  04/27/94 John Bruder    chanhed lp32EchoSuppressor to lpFlatEchoSuppressor and
 *                          added other macros for echo audio/echo suppression.
 *  04/14/94 David Markun markun@world.std.com   Support VxD version.
 *  03/04/94  oj            Added VIVOPROFILE definition.
 *
 *  (c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#ifndef _OS_H_
#define _OS_H_

//#define XP_UNIX
//#define _WIN32

#if defined(XP_UNIX)
#define FOR_UNIX 1
#endif

#if defined(__MC68K__) || defined(__powerc)
#define FOR_MAC 1
#endif

#if defined(__MWERKS__) && defined(_WIN32)
// Compiling for Windows using the Mac Metrowerks compiler. This compiler doesn't
// support inline assembly.
#define FOR_WINMW
#endif

#if defined(_WIN32) || defined(FOR_MAC) || defined(FOR_UNIX)
#define  FLAT_OS
#endif

//Defineing LIVE_CONNECT caused problems w/ Mac build.
#if defined(_WIN32) || defined(__powerc) || defined(FOR_UNIX) 
// #if defined(_WIN32) || defined(FOR_UNIX) 
#if !defined (WIN95_VXD) && !defined (VVWEB_OCX)
#define LIVE_CONNECT
#endif
#endif

#ifdef FOR_MAC

#include <Types.h>
#include <Memory.h>

#define UNREFERENCED_PARAMETER(x)

#define PASCAL
#define __declspec(X)
#define far
#define wsprintf sprintf

#define min(a,b) (((a) < (b) ? (a) : (b)))
#define max(a,b) (((a) > (b) ? (a) : (b)))

#ifdef _DEBUG
#define OutputDebugString(X) DebugStr((StringPtr) X)
#else
#define OutputDebugString(X) 
#endif

#elif defined (FOR_UNIX)

#define ASSERT assert
#define PASCAL
#define __declspec(X)
#define far
#define wsprintf sprintf

#define strcmpi strcasecmp
#define _fmemset memset
#define _fmemcpy memcpy
#define min(a,b) (((a) < (b) ? (a) : (b)))
#define max(a,b) (((a) > (b) ? (a) : (b)))

#define TRACE0

typedef long LONG;
typedef long LRESULT;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef char *LPSTR;
typedef unsigned char BYTE;
typedef unsigned char *LPBYTE;
typedef unsigned int UINT;    //Don't use these in structs! (16/32 bit diff)
typedef void         *LPVOID;

#else

// If not compiling as a VXD (with /DVXD on cmdline) OR some flat model OS.
#if !defined(VXD) && !defined(FLAT_OS)
#ifndef __WINDOWS_386__ //...if not compiling with Watcom /zw
#define WIN16       //...then it must be 16-bit Windows.
#include "mangle.h"  // Hide external references.
#endif
#endif

#if defined(_WIN32) || defined(WIN16) || defined(__WATCOMC__) || defined(__WINDOWS_386__)
#define FOR_WINDOWS
#endif

/*
 * tell compiler to hush warnings about double slash comments.
 */
#if !defined(_OS2) && !defined(FOR_UNIX)
#pragma warning (disable: 4001)
#endif

/*
 * get necessary header files,
 * unless the using program has already done so.
 */
#ifdef VXD      
#ifndef _VXD_H_
#include "vxd.h"
#endif
#endif //VXD

/**********************************************************
 * here we make the decision of whether we are in  
 * a Windows or OS/2 environment.  If we are in Windows,
 * we bring in the Win 3.x environment.  For OS/2 we load
 * <os2.h> and all its minions...
 **********************************************************/

#if !defined (_INC_WINDOWS) && !defined (_OS2) && !defined (WIN95_VXD) && !defined (FOR_UNIX)
#ifndef STDAFX_H
// If we haven't included MFC, we need to include windows.h
#include <windows.h>
#endif
#endif

// ******** Begin OS/2 definitions *********

#if defined (_OS2) 
#if !defined (OS2_INCLUDED)
#define INCL_PM
#define INCL_TYPES
#define INCL_ERRORS
#include <os2.h>
// Watcom 10a doesn't have this, so make sure its around...
#ifndef OS2DEF_INCLUDED
#include <os2def.h>
#endif
#endif

// bring in all the rest of the OS/2 Warp Extensions
#include "os2ext.h"

#endif  // defined _OS2 - back to commonality...

// ****** end of OS/2 Modifications *********

#ifdef WIN95_VXD        //Central place for VToolsD VXD's for WIN95
#ifndef __vtoolsc_h_ 
#include <vtoolsc.h>
#endif//ndef __vtoolsc_h_
#define GLOBALHANDLE U32
#define HWND         U32
#define CALLBACK
#define LPDWORD      U32 *
#define DWORD U32
typedef char *LPSTR;
typedef unsigned int UINT;    //Don't use these in structs! (16/32 bit diff)
typedef void         *LPVOID;
#define assert Assert //Use inferior VToolsD assert for now...
#ifdef DEBUG
#define DoAssert(c) Assert(c)
#else//ndef DEBUG
#define DoAssert(c) (c)
#endif//ndef DEBUG
#endif
#endif // FOR_MAC

/*
 * OS-Dependent macro for getting procedure pointers
 */
#define VVPROCPTR(name) (name)

/*
 * OS-dependent macro for definitions of global (API) functions
 */

/*
 * use VIVOAPI to declare and define a Vivo function
 */
#if defined(VXD) || defined(FLAT_OS)

#define VIVOAPI //Nothing
#else
#define VIVOAPI __far __pascal
#endif

#if defined(_OS2)
#define EXPORT _Export
#elif defined(FLAT_OS)
#define EXPORT
#else
#define EXPORT __export
#endif

/*    Use both VIVOLIBAPI and VIVODLL to declare and define a 
   function exported from a Vivo DLL. Here is an example

   VIVOLIBAPI void VIVODLL foo(int i);
   
   When this function is compiled under Win16 , the function 
   expands to 
   
      void __export __loadds __far __pascal foo(int i);

   When this function is compiled under OS2, the function
   expands to
      void foo(int i);
      
   When this function is compiled under Win32, the function
   expands to either

      __declspec(dllexport) void foo(int i);  or

      __declspec(dllimport) void foo(int i);

   depending on the function is used to create a DLL or an EXE.
           
   To create a DLL, we must #define _VIVOLIBAPI_ before include
   os.h so _VIVOLIBAPI expends to __declspec(dllexport).
   To create an EXE, we don't need to #define _VIVOLIBAPI_.
*/
#if defined(_WIN32) && !defined (WIN95_VXD)
#if !defined (_VIVOLIBAPI_)
#define  VIVOLIBAPI     __declspec(dllimport)
#else
#define VIVOLIBAPI      __declspec(dllexport)
#endif
#define __export      
#define _export
#define _loadds
#define __huge        
#else 
#define  VIVOLIBAPI
#endif

/*
 * use VIVODLL to declare and define a function exported from a Vivo DLL.
 * For further information, see "Microsoft C/C++ Programming Techniques, 
 * Section 4.4 entitled Customizing Memory Models.  We're using
 * the /Aw model for DLLs.
 */
#ifndef VIVODLL
#if defined(VXD) || defined(FLAT_OS)
#define VIVODLL //Nothing
#else
#define VIVODLL __export __loadds __far __pascal
#endif//VxD
#if defined(_OS2)
#undef VIVODLL
#define VIVODLL _System
#endif
#endif//ndef VIVODLL
/*
 * Use INTERRUPT_HANDLER to force ISR code in a DLL into a
 * fixed segment.  Use INTERRUPT_DATA to force data areas for
 * ISRs into a fixed data segment.  When using these two, ensure
 * your *.DEF file contains lines like these:
 *   SEGMENTS 'CODE_FIXED' PRELOAD FIXED    
 *   SEGMENTS 'DATA_FIXED' PRELOAD FIXED
 */

#ifdef WIN16
#define INTERRUPT_HANDLER __based (__segname("CODE_FIXED"))
#define INTERRUPT_DATA  __based (__segname("DATA_FIXED"))
//Convenient names for modules needing additional fixed segments
#define CODE_FIXED_2 __based (__segname("CODE_FIXED_2"))
#define CODE_FIXED_3 __based (__segname("CODE_FIXED_3"))
#define CODE_FIXED_4 __based (__segname("CODE_FIXED_4"))
#define CODE_FIXED_5 __based (__segname("CODE_FIXED_5"))
#define CODE_FIXED_6 __based (__segname("CODE_FIXED_6"))
#elif defined(_OS2)
// CSet doesn't seem to provide this useful and necessary feature!
#define INTERRUPT_HANDLER
#define INTERRUPT_DATA
#define _dllentry
#define Int1
//Convenient names for modules neeeding additional fixed segments
#define CODE_FIXED_2
#define CODE_FIXED_3
#define CODE_FIXED_4
#define CODE_FIXED_5
#define CODE_FIXED_6
#elif defined(VXD) || defined(FLAT_OS)
#define INTERRUPT_HANDLER //Nothing
#define INTERRUPT_DATA //Nothing
#define CODE_FIXED_2 //Nothing
#define CODE_FIXED_3 //Nothing
#define CODE_FIXED_4 //Nothing
#define CODE_FIXED_5 //Nothing
#define CODE_FIXED_6 //Nothing
#endif//def VXD

/* Use VIVOVXD to declare and define a Vivo VxD function */
#if defined(WIN95_VXD)
#define VIVOVXD _cdecl
#else
#define VIVOVXD
#endif


//Don't define NOLOCAL and NOPRIVATE for VxDs by default.  This can cause
//defects in programs which use static data/functions.
//#if defined(VXD) || defined(WIN95_VXD)
//#define NOLOCAL     //Make LOCAL procs visible for Softice debugging
//#define NOPRIVATE   //Make PRIVATE data items visible for Softice debugging
//#endif
/*
 * Use LOCALPROC as a modifier to indicate that a particular
 * function definition/declaration isn't visible outside the
 * present compilation unit.
 */
#define LOCALPROC static 
#ifdef NOLOCAL  //Useful for debugging with Softice/W
#undef LOCALPROC
#define LOCALPROC  //Not static, so debugger can see the name.
#endif

//Use PRIVATE as a modifier for variables not visible outside compilation unit
#define PRIVATE static 
#ifdef NOPRIVATE    //Useful for debugging with global-symbol-only debuggers
#undef PRIVATE
#define PRIVATE//Nothing    -- not static
#endif

/*
 * definition of file name for .ini file for loading settable parameters.
 *
 * (in other OSs, the meaning of this changes.  In Windows, it's
 * used as a filename in GetPrivateProfileInt, from stream.c)
 */
#define VIVOPROFILE "vivo324.ini"
#define VIVOHEADER "Vivo324"
                            
/*
 * Definition of All-Purpose Section for getting 
 * initialization parameters from VIVOPROFILE
 */
#define GENERALPROFILE  "general"

//Pointer conversion macros used for writing code shared between WIN16 and VXD
#ifdef VXD  
//These may *not* be used at interrupt time.
#define VvLpShadowFmLp VvLpSelOffFmLpFlat//Get me sel:off for "other" ptr
#define PFMP1616(zType,p)   ((zType)(P32LinFmP32SelOff((void*)(p))))//Cvt 16:16 to 0:32
//These are *safe* at interrupt time
#define  mcLpStream  lpStreamFlat;
#define mcLpContext  lpContextFlat;
#define mcLpData lpDataFlat
#define mcLpHeader lpHeaderFlat //Use Flat version of lpHeader
#define CtxSwanFmPStm(pstm) ((lpVvSwanContext)((pstm)->dwUserDataVxd))
#define mcLpLinearData          lpLinearDataFlat
#define mcLpEchoData           lpEchoDataFlat
#define mcLpFillBuffer         lpFillBufferFlat
#define mclpEchoSuppressor      lpFlatEchoSuppressor
#define mclpAttenuationTable    lpFlatAttenuationTable
#define mclpInputGainTable      lpFlatInputGainTable 
#define mclpLineGainTable       lpFlatLineGainTable 
#define mclpCenterClipperTable  lpFlatCenterClipperTable
#define mclpuLawToaLawTable     lpFlatuLawToaLawTable
#define mclpaLawTouLawTable     lpFlataLawTouLawTable
#define mclpLinearTouLawTable   lpFlatLinearTouLawTable
#define mclpuLawToLinearTable   lpFlatuLawToLinearTable
// This is the callback to the Ring-0 Movieman audio 
#define mclpCallAddress         dwRing0Callback
#define mcSyncAudio             SyncRing0Audio  
#define mcReadAudio             ReadRing0Audio
#define mcWriteAudio        WriteRing0Audio
#define mcLpMvManAudioParms     lpFlatMvManAudioParms
#define mcLpSilence           lpFlatSilence
#define mcLpGarbage           lpFlatGarbage            
#elif defined WIN16
#define  mcLpStream  lpStream;
#define mcLpContext  lpContext;
//These may *not* be used at interrupt time.
#define VvLpShadowFmLp VvLpFlatFmLpSelOff//Get me flat for "other" kinda ptr
//These are *safe* at interrupt time
#define PFMP1616(zType,p)   ((zType)(p))    //Simply use sel:off as is
#define mcLpData lpData
#define mcLpFillBuffer         lpFillBuffer
#define mcLpHeader lpHeader //Use 16:16 version of lpHeader
#define CtxSwanFmPStm(pstm) ((lpVvSwanContext)((pstm)->dwUserData))
#define mcLpLinearData         lpLinearData
#define mcLpEchoData        lpEchoData
#define mclpEchoSuppressor      lpEchoSuppressor
#define mclpAttenuationTable    lpAttenuationTable
#define mclpInputGainTable      lpInputGainTable 
#define mclpLineGainTable       lpLineGainTable 
#define mclpCenterClipperTable  lpCenterClipperTable
#define mclpuLawToaLawTable     lpuLawToaLawTable
#define mclpaLawTouLawTable     lpaLawTouLawTable
#define mclpLinearTouLawTable   lpLinearTouLawTable
#define mclpuLawToLinearTable   lpuLawToLinearTable
        
// This is the callback to the Ring-3 Movieman audio  
#define mclpCallAddress       fpRing3Callback
#define mcSyncAudio         SyncRing3Audio
#define mcReadAudio        ReadRing3Audio
#define mcWriteAudio       WriteRing3Audio
#define mcLpMvManAudioParms lpMvManAudioParms
#define mcLpSilence         lpSilence
#define mcLpGarbage         lpGarbage 
#elif defined(FLAT_OS)

#define mcLpStream           lpStream;
#define mcLpContext             lpContext;        
#define VvLpShadowFmLp(x)       (x)
#define PFMP1616(zType, p)      ((zType)(p))
#define mcLpData                lpData
#define mcLpFillBuffer          lpFillBuffer
#define mcLpHeader              lpHeader
#define CtxSwanFmPStm(pstm)     ((lpVvSwanContext)((pstm)->dwUserData))
#define mcLpLinearData          lpLinearData 
#define mcLpEchoData            lpEchoData
#define mclpEchoSuppressor      lpEchoSuppressor
#define mclpAttenuationTable    lpAttenuationTable
#define mclpInputGainTable      lpInputGainTable 
#define mclpLineGainTable       lpLineGainTable 
#define mclpCenterClipperTable  lpCenterClipperTable
#define mclpuLawToaLawTable     lpuLawToaLawTable
#define mclpaLawTouLawTable     lpaLawTouLawTable
#define mclpLinearTouLawTable   lpLinearTouLawTable
#define mclpuLawToLinearTable   lpuLawToLinearTable

// This is the callback to the Ring-3 Movieman audio  
#define mclpCallAddress           fpRing3Callback
#define mcSyncAudio              SyncRing3Audio
#define mcReadAudio              ReadRing3Audio
#define mcWriteAudio         WriteRing3Audio
//#define mcLpMvManAudioParms       lpMvManAudioParms
#define mcLpSilence              lpSilence
#define mcLpGarbage              lpGarbage 
#endif

//Use Flatten macros in 16-bit code only; they flatten ptr in place 
#ifdef WIN16
#define McFlattenIfUsingVxd(p1616) (VvTUsingVxd()/*If using Vxd*/\
    && McFlattenPtr(p1616))/*Make ptr flat*/\
/* Else leave ptr untouched*/
#define McFlattenPtr(p1616) \
    (*(void far* far*)&(p1616)=VvLpFlatFmLpSelOff(p1616))/*Make ptr flat*/

#elif defined(FLAT_OS)
#define McFlattenIfUsingVxd(x)
#define McFlattenPtr(x)

#endif

//Environment assertion macros help keep us from trying to build
//  environment-specific modules in environments where they don't make sense
//If you write environment-specific modules, put these assertions in.
#ifdef WIN16
#define AssertWin16() //Nothing
#define AssertWindows() // Nothing
#define AssertWatcom32() (HeyThisShouldOnlyBeBuiltWatcom32!!)
#define AssertVxd() (HeyThisShouldOnlyBeBuiltVxd!!)
#define AssertMac() (HeyThisShouldOnlyBeBuiltMac!!)
#define AssertNotMac() // Nothing
#elif defined _WIN32
#define AssertWin16() (HeyThisShouldOnlyBeBuiltWin16)
#define AssertWindows() // Nothing
#define AssertWatcom32() (HeyThisShouldOnlyBeBuiltWatcom32!!)
#define AssertVxd() (HeyThisShouldOnlyBeBuiltVxd!!)
#define AssertMac() (HeyThisShouldOnlyBeBuiltMac!!)
#define AssertNotMac() // Nothing
#elif defined VXD
#define AssertWin16() (HeyThisShouldOnlyBeBuiltWin16)
#define AssertWindows() (HeyThisShouldOnlyBeBuildWindows!!)
#define AssertWatcom32() (HeyThisShouldOnlyBeBuiltWatcom32!!)
#define AssertVxd() //Nothing
#define AssertMac() (HeyThisShouldOnlyBeBuiltMac!!)
#define AssertNotMac() // Nothing
#elif defined __WINDOWS_386__
#define AssertWin16() (HeyThisShouldOnlyBeBuiltWin16)
#define AssertWindows() / Nothing
#define AssertWatcom32() //Nothing
#define AssertVxd() (HeyThisShouldOnlyBeBuiltVxd!!)
#define AssertMac() (HeyThisShouldOnlyBeBuiltMac!!)
#define AssertNotMac() // Nothing
#elif defined FOR_MAC
#define AssertWin16() (HeyThisShouldOnlyBeBuiltWin16)
#define AssertWindows() (HeyThisShouldOnlyBeBuildWindows!!)
#define AssertWatcom32() (HeyThisShouldOnlyBeBuiltWatcom32!!)
#define AssertVxd() (HeyThisShouldOnlyBeBuiltVxd!!)
#define AssertMac() // Nothing
#define AssertNotMac() (HeyThisShouldBeNonMac!!)
#elif defined _NTWIN
#endif

#if defined( _NTWIN) || defined(_WIN32) || defined(FOR_MAC)

#define _fmemcpy memcpy
#define _fmemset memset
#define _fmemcmp memcmp
#define _fstrncpy strncpy
#define _fstrcpy strcpy
#define _fstrlen strlen
#define _fstrchr strchr

#endif

#endif//ndef _OS_H_



