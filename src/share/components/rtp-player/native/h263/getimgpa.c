/*-----------------------------------------------------------------------------
 *  GETIMGPA.C
 *
 *  Function:   getImgParms
 * 
 *  DESCRIPTION
 *      Calculates image parameters based on formatCap.
 *
 *  Author:     Mary Deshon     7/01/93
 *  Inspected:  <<not inspected yet>>
 *  Revised:
 *  10/20/96    S Ericsson      Removed unnecessary stuff
 *  07/22/96    S Ericsson      Added 4CIF and 16CIF; corrected maxsym (1 symbol/pixel)
 *  08/22/95    M Deshon        Use h263 number of gobs.
 *  07/11/95    S Ericsson      Added SQCIF case.
 *  12/10/93    S Ericsson      Extracted getImgParms() from "vvopcl.c"
 *
 *  (c) 1993-1997, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

/*  Header File Includes        */
#include "dllindex.h"
#include "h261func.h"
#include "h261defs.h"

short getImgParms( short formatCap, short *numGOBs, short *numMBs, long *imgSize,
                   long *lumaSize, long *chromaLineLength, long *chromaRows, long *maxsym )
{
    switch ( formatCap )
    {
    case SQCIF:
        *chromaLineLength = (long)64;
        *chromaRows = (long)48;
        *numGOBs  = 6;
        *numMBs   = (8*6);
        break;
    case QCIF:
        *chromaLineLength = (long)88;
        *chromaRows = (long)72;
        *numGOBs  = 9;
        *numMBs   = (11*9);
        break;
    case CIF:
        *chromaLineLength = (long)176;
        *chromaRows = (long)144;
        *numGOBs  = 18;
        *numMBs   = (22 * 18);
        break;
    case CIF4:
        *chromaLineLength = (long)352;
        *chromaRows = (long)288;
        *numGOBs  = 18;
        *numMBs   = 4 * 22*18;
        break;
    case CIF16:
        *chromaLineLength = (long)704;
        *chromaRows = (long)576;
        *numGOBs  = 18;
        *numMBs   = 16 * 22*18;
        break;
    default:
        return ( UNKNOWN_PICTURE_FORMAT );
        break;
    }
    *imgSize = *numMBs * (256 + 2 * 64);
    *lumaSize = *numMBs * 256;
    *maxsym = *imgSize;
    return ( 0 );
}
