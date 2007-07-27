/*-----------------------------------------------------------------------------
 *  VVDBG.C
 *
 *  DESCRIPTION
 *		Windows specific debug output function.
 * 
 *
 *	Author:		Mary Deshon		7/15/93
 *	Inspector:	<<not inspected yet>>
 *	Revised:
 *
 *	(c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

#define FOR_UNIX

#ifndef FOR_UNIX
#include <windows.h>
#endif

#include "machine.h"

extern void VvDbg ( char *msg );
extern void VvDbg ( char *msg )
{
  /* Comment this out for UNIX until we have established how to handle debug */
#ifndef FOR_UNIX
    OutputDebugString ( msg );
#endif

}

