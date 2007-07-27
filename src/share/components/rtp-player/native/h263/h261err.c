/*-----------------------------------------------------------------------------
 *  H261ERR.C
 *
 *  DESCRIPTION
 *      h261err.c - Display error messages in Windows message box
 *
 *
 *  Author:     Staffan Ericsson    7/19/93
 *  Inspector:  <<not inspected yet>>   
 *  Revised:
 *  8/15/94     Change from MessageBox to OutputDebugString
 *  
 *  (c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#define FOR_UNIX

#ifndef FOR_UNIX
#include <windows.h>
#endif

#include "dllindex.h"
#include "h261func.h"
#include <stdio.h>

/* Comment this next line out to never see the error messages! */
/* Until we have a better way to log, lets just comment this
   out for UNIX */
#ifndef FOR_UNIX
#define _USE_OUTPUTDBGSTR_
#endif

//  H261ErrMsg - Display error message
extern void H261ErrMsg( char s[] )
{

#ifdef _USE_OUTPUTDBGSTR_
    OutputDebugString ( "Error Message from Video Codec" );
    OutputDebugString ( s );
#else
    fprintf(stderr, "Error message from Video Codec: %s \n", s);
#endif
    return;
}


