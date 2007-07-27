/*-----------------------------------------------------------------------------
 *	H263PDEC.C
 *
 *	DESCRIPTION
 *		h263pdec.c - decoding support for some H.263+ extensions
 *
 *      Author:     David Blair    		03/04/97
 *      Inspector:  
 *	(c) 1997-98, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 
#include <string.h>
#include "dllindex.h"
#include "h261defs.h"
#include "h261func.h"
#include "h263plus.h"
#include <stdlib.h>
#include <stdio.h>

void PredBframePlus( MACROBLOCK_DESCR * mb,  // Macroblock to be predicted
                        PICTURE * prevPic,      // Prev. picture (forward pred)
                        PICTURE * nextPic,      // Next P-picture (backward pred)
                        PICTURE * Bpic          // Output picture where pred is placed
                        );

//  PredBframePlus - Form prediction for B-frame a la H.263+
void PredBframePlus( MACROBLOCK_DESCR * mb,  // Macroblock to be predicted
                        PICTURE * prevPic,      // Prev. picture (forward pred)
                        PICTURE * nextPic,      // Next P-picture (backward pred)
                        PICTURE * Bpic          // Output picture where pred is placed
                        )
{
    if(BFRAME_IS_BIDIRECTIONAL(mb))
    {
        PredBframe(mb,prevPic,nextPic,Bpic);
    }
    else
    {
        S8 saveMvX = mb->mv_x; 
        S8 saveMvY = mb->mv_y;
        U8 saveType = mb->mtype;
        mb->mv_x = mb->mvdB_x; 
        mb->mv_y = mb->mvdB_y;
        mb->mtype = MTYPE263_INTER;
        MotionComp263( mb, prevPic, Bpic );
        mb->mv_x = saveMvX;
        mb->mv_y = saveMvY;
        mb->mtype = saveType;

    }
}


//////////////////////////////////////////////////////////////////////////
// Deblocking Filter Mode Functions
//

static U8 ClipTableBase[288];
static U8 * ClipTable=NULL;

static void InitializeClipTable()
{
    int i;

    if(ClipTable) {
        return;
    } else {
        ClipTable = ClipTableBase + 16;
    }
    for(i = -16; i<=0; i++) ClipTable[i] = 0x00;
    for(i=1 ; i<256; i++) ClipTable[i] = (U8) i;
    for(i=256 ; i<272; i++) ClipTable[i] = 0xFF;
}

static S8 DiffCutoffTableBase[351]; // Range [-175, 175]
static S8 * DiffCutoffTable=NULL;

static void InitializeDiffCutoffTable(S32 qp)
{
    static int DiffTableQuantCache=0;
    int d, d2;

    if(DiffCutoffTable==NULL) {
        memset(DiffCutoffTableBase, 0, 351);
        DiffCutoffTable = DiffCutoffTableBase + 175;
    }

    if(DiffTableQuantCache == qp) return;

    for(d=0, d2=0; d2<=qp; d++, d2+=2) {
        DiffCutoffTable[d] = d;
        DiffCutoffTable[-d] = -d;
    }

    for(;d<=qp;d++) {
        DiffCutoffTable[d] = qp - d;
        DiffCutoffTable[-d] = -qp + d;
    }

    for(;d<=DiffTableQuantCache;d++) {
        DiffCutoffTable[d] = 0;
        DiffCutoffTable[-d] = 0;
    }

    DiffTableQuantCache = qp;
}

#ifdef _DEBUG
static PIXEL * MACROBLOCK_LUMA_PTR(PICTURE * p,MACROBLOCK_DESCR * mb) 
{
    return ((p)->y.ptr + 16*(mb)->x + 16*(mb)->y*(p)->y.hoffset);
}

static PIXEL * BlockLumaPtr(PICTURE * pic, MACROBLOCK_DESCR * mb, int blk)
{
    switch(blk)
    {
    case 0:
        return MACROBLOCK_LUMA_PTR(pic,mb);
    case 1:
        return MACROBLOCK_LUMA_PTR(pic,mb) + 8;
    case 2:
        return MACROBLOCK_LUMA_PTR(pic,mb) + 8*pic->y.hoffset;
    default:
        return MACROBLOCK_LUMA_PTR(pic,mb) + 8*(pic->y.hoffset + 1);
    }
}

static PIXEL * MacroBlockCrPtr(PICTURE * pic, MACROBLOCK_DESCR * mb)
{
    return pic->cr.ptr + 8*mb->x + 8*mb->y*pic->cr.hoffset;
}

static PIXEL * MacroBlockCbPtr(PICTURE * pic, MACROBLOCK_DESCR * mb)
{
    return pic->cb.ptr + 8*mb->x + 8*mb->y*pic->cb.hoffset;
}
#else

#define MACROBLOCK_LUMA_PTR(p, mb) \
    ((p)->y.ptr + 16*(mb)->x + 16*(mb)->y*(p)->y.hoffset)

#define BlockLumaPtr(pic, mb, blk) \
    (MACROBLOCK_LUMA_PTR((pic),(mb)) + 4*((blk)&2)*(pic)->y.hoffset + 8*((blk)&1))

#define MacroBlockCrPtr(pic, mb) \
    ((pic)->cr.ptr + 8*(mb)->x + 8*(mb)->y*(pic)->cr.hoffset)

#define MacroBlockCbPtr(pic, mb) \
    ((pic)->cb.ptr + 8*(mb)->x + 8*(mb)->y*(pic)->cb.hoffset)

#endif

static void ApplyVerticalDeblockingFilter( PIXEL * left, PIXEL * right, int offset)
{
    int i;
    left += 7;
    for(i=0; i<8; i++) {
	S32 d;
	S32 tmp = (3*left[-1] - 8*left[0] + 8*right[0] - 3*right[1]);
        d = DiffCutoffTable[tmp>>4];
        
        *left = ClipTable[*left + d];
        *right = ClipTable[*right - d];

        left += offset;
        right += offset;
    }
}

static void ApplyHorizontalDeblockingFilter( PIXEL * top, PIXEL * bottom, int offset)
{
    int i;
    PIXEL *next_to_top;
    PIXEL *next_after_bottom;
    top += 7*offset;
    next_to_top = top - offset;
    next_after_bottom = bottom + offset;

    for(i=0; i<8; i++) {
	S32 d;
	S32 tmp;
	tmp = (3*next_to_top[0] - 8*top[0] + 8*bottom[0] -
	       3*next_after_bottom[0]);
	
        d = DiffCutoffTable[tmp>>4];
        *top = ClipTable[*top + d];
        *bottom = ClipTable[*bottom - d];

        top += 1;
        next_to_top += 1;
        bottom += 1;
        next_after_bottom += 1;
    }
}



// This routine applies the H.263+ deblocking filter to a decoded picture.  Note
// That this routine is NOT H.263+ compliant because it applies the filter after the
// reconstructed picture has already been clipped to 0,255
void ApplyDeblockingFilter( PICTURE * pic, MACROBLOCK_DESCR * mb, S32 Bframe)
{
    int i,h,v;
    int numhor=pic->y.nhor>>4; // number of mb per row
    int numvert=pic->y.nvert>>4; // number of mb per col
    // int nummb = numhor*numvert;
    int off = pic->y.hoffset;
//#define DB_DO_TIMING
#ifdef DB_DO_TIMING
    static char foo[100];
    static LARGE_INTEGER tick,tock,freq;
    tick.HighPart = 0;
    tick.LowPart = 0;
    tock.HighPart = 0;
    tock.LowPart = 0;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&tick);
#endif

    if(ClipTable==NULL) InitializeClipTable();
   
    for(i=0,v=0; v<numvert; v++) {
        for(h=0; h<numhor; h++, i++) {
            int qp;

            if(mb[i].mtype == MTYPE_SKIP) {
                // Here we check to see if the macroblock below us is coded.  If it is then we need to
                // filter our bottom horizontal edges.   Note that we need to use the quant value of the
                // block below
                if(v<numvert-1 && mb[i+numhor].mtype != MTYPE_SKIP) {
                    qp = Bframe ? mb[i+numhor].Bquant : mb[i+numhor].quant;
                    InitializeDiffCutoffTable(qp);
                    ApplyHorizontalDeblockingFilter(BlockLumaPtr(pic,&mb[i],2), BlockLumaPtr(pic,&mb[i+numhor],0), off);
                    ApplyHorizontalDeblockingFilter(BlockLumaPtr(pic,&mb[i],3), BlockLumaPtr(pic,&mb[i+numhor],1), off);
                    ApplyHorizontalDeblockingFilter(MacroBlockCbPtr(pic, &mb[i]), MacroBlockCbPtr(pic,&mb[i+numhor]), pic->cb.hoffset);
                    ApplyHorizontalDeblockingFilter(MacroBlockCrPtr(pic, &mb[i]), MacroBlockCrPtr(pic,&mb[i+numhor]), pic->cr.hoffset);
                }

                // Here we check to see if the macroblock to the left of us is coded. If so do our left vertical
                // edges
                if(h && mb[i-1].mtype != MTYPE_SKIP) {
                    qp = Bframe ? mb[i-1].Bquant : mb[i-1].quant;
                    InitializeDiffCutoffTable(qp);
                    ApplyVerticalDeblockingFilter(BlockLumaPtr(pic,&mb[i-1],1), BlockLumaPtr(pic,&mb[i],0), off);
                    ApplyVerticalDeblockingFilter(BlockLumaPtr(pic,&mb[i-1],3), BlockLumaPtr(pic,&mb[i],2), off);
                    ApplyVerticalDeblockingFilter(MacroBlockCbPtr(pic, &mb[i-1]), MacroBlockCbPtr(pic,&mb[i]), pic->cb.hoffset);
                    ApplyVerticalDeblockingFilter(MacroBlockCrPtr(pic, &mb[i-1]), MacroBlockCrPtr(pic,&mb[i]), pic->cr.hoffset);
                }

            } else {

                qp = Bframe ? mb[i].Bquant : mb[i].quant;
                InitializeDiffCutoffTable(qp);


                // First do the first two horizontal edges
                ApplyHorizontalDeblockingFilter(BlockLumaPtr(pic,&mb[i],0), BlockLumaPtr(pic,&mb[i],2), off);
                ApplyHorizontalDeblockingFilter(BlockLumaPtr(pic,&mb[i],1), BlockLumaPtr(pic,&mb[i],3), off);
                // Now do the first two vertical edges; don't filter if the left edge is a picture edge
                if(h) {
                    ApplyVerticalDeblockingFilter(BlockLumaPtr(pic,&mb[i-1],1), BlockLumaPtr(pic,&mb[i],0), off);
                }
                ApplyVerticalDeblockingFilter(BlockLumaPtr(pic,&mb[i],0), BlockLumaPtr(pic,&mb[i],1), off);

                // Now do the bottom two horizontal edges; don't filter if we are at a bottom edge of the picture
                if(v < numvert-1) {
                    // Use quantization parameter for lower macroblock if it was coded
                    if(mb[i+numhor].mtype != MTYPE_SKIP) {
                        InitializeDiffCutoffTable(Bframe ? mb[i+numhor].Bquant : mb[i+numhor].quant);
                    }

                    ApplyHorizontalDeblockingFilter(BlockLumaPtr(pic,&mb[i],2), BlockLumaPtr(pic,&mb[i+numhor],0), 
                        off);
                    ApplyHorizontalDeblockingFilter(BlockLumaPtr(pic,&mb[i],3), BlockLumaPtr(pic,&mb[i+numhor],1), 
                        off);
                    
                    // restore quantization parameter of current macroblock
                    InitializeDiffCutoffTable(qp);
                }

                // Lastly do the lower two vertical edges; don't filter if the left edge is a picture edge
                if(h) {
                    ApplyVerticalDeblockingFilter(BlockLumaPtr(pic,&mb[i-1],3), BlockLumaPtr(pic,&mb[i],2), off);
                }
                ApplyVerticalDeblockingFilter(BlockLumaPtr(pic,&mb[i],2), BlockLumaPtr(pic,&mb[i],3), off);


                // Now apply the filters to the chroma
                if(v < numvert-1) {
                    ApplyHorizontalDeblockingFilter(MacroBlockCbPtr(pic, &mb[i]), MacroBlockCbPtr(pic,&mb[i+numhor]), pic->cb.hoffset);
                    ApplyHorizontalDeblockingFilter(MacroBlockCrPtr(pic, &mb[i]), MacroBlockCrPtr(pic,&mb[i+numhor]), pic->cr.hoffset);
                }
                if(h) {
                    ApplyVerticalDeblockingFilter(MacroBlockCbPtr(pic, &mb[i-1]), MacroBlockCbPtr(pic,&mb[i]), pic->cb.hoffset);
                    ApplyVerticalDeblockingFilter(MacroBlockCrPtr(pic, &mb[i-1]), MacroBlockCrPtr(pic,&mb[i]), pic->cr.hoffset);
                }
            }
        }
    }
#ifdef DB_DO_TIMING
    QueryPerformanceCounter(&tock);
    sprintf(foo, "DeblockingFilter = %d ms\n", (1000*(tock.LowPart - tick.LowPart))/freq.LowPart);
    OutputDebugString(foo);
#endif
}


//////////////////////////////////////////////////////////////////////////////
//  Deblocking filter for Reduced Resolution Update mode
//

static void vertFilterRRUmode( PIXEL * right, int offset, int len )
{
    int i, valLeft, valRight;

    for(i=0; i < len; i++) {
        valLeft = right[-1];
        valRight = right[0];
        right[-1] = (3 * valLeft + 1 * valRight + 2) >> 2;
        right[0]  = (1 * valLeft + 3 * valRight + 2) >> 2;
        right += offset;
    }
}

static void horFilterRRUmode( PIXEL * bottom, int offset, int len )
{
    int i, valTop, valBottom;
    PIXEL *top;
        
    top = bottom - offset;
    for(i=0; i < len; i++) {
        valTop = top[0];
        valBottom = bottom[0];
        top[0]    = (3 * valTop + 1 * valBottom + 2) >> 2;
        bottom[0] = (1 * valTop + 3 * valBottom + 2) >> 2;
        top += 1;
        bottom += 1;
    }
}



// This routine applies the Reduced-res. Update mode deblocking filter to a decoded picture.
extern void ReducedResDeblockingFilter( PICTURE * pic, MACROBLOCK_DESCR * mb )
{
    int i,h,v;
    int numhor = (pic->y.nhor + 16) >> 5;   // number of mb per row
    int numvert= (pic->y.nvert+ 16) >> 5;   // number of mb per col
    // int nummb = numhor*numvert;
    int off = pic->y.hoffset;
    PIXEL   *luma0;
    int     hSize, vSize, cOffset;
   
    for(i=0,v=0; v<numvert; v++) {
        vSize = 32;
        if (32 * v + 16 >= pic->y.nvert)
            vSize = 16;     // MB is truncated at the bottom
        for(h=0; h<numhor; h++, i++) {
            hSize = 32;
            if (32 * h + 16 >= pic->y.nhor)
                hSize = 16; // MB is truncated at the right
            luma0 = pic->y.ptr + 32 * h + 32 * v * off;
            cOffset = 16 * h + 16 * v * pic->cb.hoffset;

            // Filter horizontal edge inside block
            if (mb[i].mtype != MTYPE_SKIP  &&  vSize == 32) {
                horFilterRRUmode( luma0 + 16*off, off, hSize );
            }
            // Filter horizontal edge at the bottom of the block
            if (v < numvert-1  &&  
                (mb[i].mtype != MTYPE_SKIP  ||  mb[i+numhor].mtype != MTYPE_SKIP)) {
                horFilterRRUmode( luma0 + 32*off, off, hSize );
                horFilterRRUmode( pic->cb.ptr + cOffset + 16*pic->cb.hoffset,
                                  pic->cb.hoffset, hSize>>1 );
                horFilterRRUmode( pic->cr.ptr + cOffset + 16*pic->cr.hoffset,
                                  pic->cr.hoffset, hSize>>1 );
            }
            // Filter vertical edge at the left of the block
            if (h > 0  &&
                (mb[i].mtype != MTYPE_SKIP  ||  mb[i-1].mtype != MTYPE_SKIP)) {
                vertFilterRRUmode( luma0, off, vSize );
                vertFilterRRUmode( pic->cb.ptr + cOffset,
                                  pic->cb.hoffset, vSize>>1 );
                vertFilterRRUmode( pic->cr.ptr + cOffset,
                                  pic->cr.hoffset, vSize>>1 );
            }
            // Filter vertical edge inside block
            if (mb[i].mtype != MTYPE_SKIP  &&  hSize == 32) {
                vertFilterRRUmode( luma0 + 16, off, vSize );
            }
        }
    }
}


// Alternate Horizontal Scan Order
static int alt_hor_scan[64] = {  0, 1, 2, 3,10,11,12,13,
                                 4, 5, 8, 9,17,16,15,14,
                                 6, 7,19,18,26,27,28,29,
                                20,21,24,25,30,31,32,33,
                                22,23,34,35,42,43,44,45,
                                36,37,40,41,46,47,48,49,
                                38,39,50,51,56,57,58,59,
                                52,53,54,55,60,61,62,63};

// Alternate Vertical (MPEG-2) Scan Order
static int alt_ver_scan[64] = {  0, 4, 6,20,22,36,38,52,
                                 1, 5, 7,21,23,37,39,53,
                                 2, 8,19,24,34,40,50,54,
                                 3, 9,18,25,35,41,51,55,
                                10,17,26,30,42,46,56,60,
                                11,16,27,31,43,47,57,61,
                                12,15,28,32,44,48,58,62,
                                13,14,29,33,45,49,59,63};


static int MyZigZag[64] = {  0,  1,  5,  6,  14, 15, 27, 28,
                            2,  4,  7,  13, 16, 26, 29, 42,
                            3,  8,  12, 17, 25, 30, 41, 43,
                            9,  11, 18, 24, 31, 40, 44, 53,
                            10, 19, 23, 32, 39, 45, 52, 54,
                            20, 22, 33, 38, 46, 51, 55, 60,
                            21, 34, 37, 47, 50, 56, 59, 61,
                            35, 36, 48, 49, 57, 58, 62, 63
};
static int reorder[8] = {0,4,2,7, 1,5,3,6};
int ireorder[8] = {0,4,2,6, 1,5,7,3};

int inv_alt_hor_scan[64];
int inv_alt_hor_scan_no_reorder[64];
int alt_hor_to_zigzag[64];
int inv_alt_ver_scan[64];
int inv_alt_ver_scan_no_reorder[64];
int alt_ver_to_zigzag[64];
int zigzag_to_zigzag[64];

void InitAdvancedIntraTables()
{
    static int initialized=0;
    int index,i,j;

    if(initialized) return;
    initialized = 1;
    // Generate inverse scan order vectors
    for (index = 0; index < 64; index++) {
        i = reorder[index % 8];
        j = reorder[index / 8];
        inv_alt_ver_scan[ alt_ver_scan[index] ] = i*8 + j;
        inv_alt_hor_scan[ alt_hor_scan[index] ] = i*8 + j;

        inv_alt_ver_scan_no_reorder[ alt_ver_scan[index] ] = index;
        inv_alt_hor_scan_no_reorder[ alt_hor_scan[index] ] = index;
    }

    // Now do the scan's from alt hor and alt ver to zigzag.
    // We need to be careful because the above tables take into
    // account the reorderings that come from our DCT.  We don't
    // want to take that into account, so we apply the ireorder
    for(index = 0; index < 64; index++) {
        i = ireorder[ inv_alt_ver_scan[index]&7 ];
        j = ireorder[ inv_alt_ver_scan[index]>>3 ];

        alt_ver_to_zigzag[ index ] = MyZigZag[ i*8 + j ];
        
        i = ireorder[ inv_alt_hor_scan[index]&7 ];
        j = ireorder[ inv_alt_hor_scan[index]>>3 ];

        alt_hor_to_zigzag[ index ] = MyZigZag[ i*8 + j ];

        zigzag_to_zigzag[ index ] = index;
    }
}


