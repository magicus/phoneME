/*-----------------------------------------------------------------------------
 *  VVENC.H
 *
 *  DESCRIPTION
 *      Defines structures and function prototypes for the interface
 *      between the coder and the Video Compression Stream Handler.
 * 
 *  SEE ALSO
 *              H.261 standard.         
 *
 * ***** WARNING ***** WARNING ***** WARNING ***** WARNING ***** WARNING ***** WARNING *****
 *      DO NOT USE TYPE "int" IN THIS FILE. IT IS SHARED BY 32-BIT AND 16-BIT CODE.
 *
 *  Author:     Mary Deshon     7/15/93
 *  Inspector:  <<not inspected yet>>
 *  Revised:
 *      02/12/97    se          Add dwBframe to VvDecoderStats
 *      02/06/97    awanka      Added DLL entry point def's for audio resampler
 *      01/31/97    se          Add reducedResUpdate to PICTURE_DESCR and FrameParams
 *      12/30/96    jsalman     Added DLL entry point def's for Siren decoder
 *      12/19/96    se          Add decodingInProgress to PICTURE_DESCR
 *      11/18/96    D Blair     Add forceMbIntra to H261EncoderParams for forcing intra coding of macroblocks
 *      11/02/96    se          Support PB-frames
 *      07/19/96    md          Add rows and cols to PICTURE_DESCR
 *      05/29/96    md          Add dwFirstBadgob to VvDecoderStats
 *      04/18/96    wolfe       Added G.723 function entry point constants
 *      12/19/95    md          Split out prototypes into another header file
 *      11/27/95    md          Add advancedPred to H261EncoderParams
 *      06/25/95    S Ericsson  Modified PICTURE_DESCR for H.263
 *      03/14/95    md          Minor changes to support record and playback.
 *      10/17/94    md          Bump up size of the bitstream to 48K.
 *      09/28/94    md          Added H261PreviewParams structure.
 *      09/22/94    md          Split OpenEncoder and OpenDecoder call up.
 *      09/21/94    md          Modified structures to support histogramming of raw luma data
 *      09/15/94    md          Added PreProcess entry and replaced PictureToDIB with DecPictureToDIB and EncPictureToDIB
 *      09/12/94    Bruder      Added pei and pspare to frame parms. Added tTimestamp to PICTURE
 *      09/09/94    md          Added gamma, mirror, and rotate to H261EncoderParams.
 *      09/02/94    md          Code smartscreen in slices.
 *      06/06/94    M Deshon    PredSel weights.
 *      04/07/94    S Ericsson  Label DCT stats array
 *      04/06/94    M Deshon    Added DCT stats array.
 *      03/28/94    M Deshon    Changes for bit-rate control.
 *      02/28/94    M Deshon    Different "type" fields for x any y motion vector maps.
 *      02/24/94    M Deshon    Added MEST_MAP type.
 *      02/23/94    M Deshon    Moved DECODER_MAP define from h261dec.c into this file.
 *      01/24/94    M Deshon    Fixed bug in MBMap structure. I had used type "int" but this is a
 *                              no-no for header files that are shared by 16 and 32 bit code !!!
 *      01/24/94    M Deshon    Moved some prototypes from this file into h261func.h
 *      01/04/94    M Deshon    Changed targetBits to qTargetBits ( for vsvcoder.c compatibility). 
 *      12/13/93    M Deshon    Fixed bug #83: alignment of PICTURE structure. (Also: re-aligned tabs !!??!!). 
 *      11/14/93    M Deshon    Added minQuant element to H261EncoderParams structure  
 *      11/04/93    J Bruder    Added monochrome to H261EncoderParams and assed VLV4
 *      11/01/93    J Bruder    Added lowRes to H261EncoderParams  
 *      10/08/93    J Bruder    Added Constant for Captivator (VLV3)     
 *      09/30/93    M Deshon    Added BCH DLL identifiers
 *      08/06/93    M Deshon    Added MbInfo struct
 *      08/02/93    M Deshon    Changed proto for VvDecode
 *      07/29/93    M Deshon    Added xSize,ySize and picture_layout to PICTURE struct
 *                              Added pOffset to COMPONENT struct
 *      07/28/93    M Deshon    Changed proto for InitTFiltTable
 *                              Added enum for TFilt ID.
 *      07/25/9     M Deshon    Moved COMPONENT, PICTURE, and PIC_DESCR from
 *                              h261type.h to this file. Changed them to be
 *                              usable by the 16-bit world. i.e. no "ints"
 *                              allowed and variable pointer definition.
 *      07/18/93    M Deshon    Function proto changes
 *
 *      (c) 1993,1997, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

#ifndef _INC_VVENC
#define _INC_VVENC      1

#include "machine.h"
#include "h261type.h" 

//#define VvDbgMsg( msg )       VvDbg( msg )
#define VvDbgMsg( msg )
extern void VvDbg ( char *msg );
 
// DLL entry point assignments
#define _DLL_FRAME_UNPACK           1       // Test
#define _DLL_CONVERT_YUVTORGB       2       // Test
#define _DLL_OPEN_ENCODER           3
#define _DLL_GET_ENCODER_PTR        4
#define _DLL_SET_ENCODER_PARAMS     5
#define _DLL_SET_PREVIEW_PARAMS     6
#define _DLL_PREPROCESS             7
#define _DLL_ENCODE                 8
#define _DLL_RECON                  9
#define _DLL_CLOSE_ENCODER          10
#define _DLL_OPEN_DECODER           11
#define _DLL_GET_DECODER_PTR        12
#define _DLL_SET_DECODER_PARAMS     13
#define _DLL_DECODE                 14
#define _DLL_CLOSE_DECODER          15
#define _DLL_GET_ENCODER_STATS      16
#define _DLL_BCH_ENCODE             17
#define _DLL_BCH_DECODE             18
#define _DLL_ENC_PICTURE_TO_DIB     19
#define _DLL_DEC_PICTURE_TO_DIB     20
#define _DLL_ENC_GET_MAP            21
#define _DLL_DEC_GET_MAP            22
#define _DLL_INIT_G723_TABLES       23
#define _DLL_INIT_G723_DECODER      24
#define _DLL_G723_DECODE            25
#define _DLL_SIREN_DEC_TABLES_INIT  26
#define _DLL_SIREN_DECODER_INIT     27
#define _DLL_SIREN_DECODER          28
#define _DLL_RESAMPLE_SETUP         29
#define _DLL_RESAMPLE_OPERATE       30
#define _DLL_RESAMPLE_UNSETUP       31
#define _DLL_DEC_SCALEPICTURETODIB  32
#define _DLL_NEXT_FUNCTION          33

/* Temporal filter function id.s        */
enum { Identity, NonLinear };

/* MbInfo */
typedef struct {
    S16         predBits;               // Number of predicted bits per macroblock
    S16         actYBits;               // Number of actual bits per macroblock (Y component)
    S16         actCbBits;              // Number of actual bits per macroblock (Cb component)
    S16         actCrBits;              // Number of actual bits per macroblock (Cr component)
    S32         mse;                    // mse or var measure
    S16         qStep;                  // Quantizer step used
    S16         codingType;             // Intra, Inter, etc.
    S16         cFlag;                  // True if any luma blocks to be coded
    S16         dummy;
} MbInfo;
 
/* FrameInfo */
typedef struct {
    S32         logSum;                 // Sum of variance or mse for coded blocks
    S32         codedBlkCnt;            // Number of macroblocks to be coded
    S32         qTargetBits;            // Goal Bits for q-step selection
} FrameInfo; 

/* COMPONENT - Image component descriptor */
typedef struct {
    S32         nhor;           // Active pels per row
    S32         nvert;          // Number of rows
    S32         hoffset;        // Bytes between rows i.e. active + non-active
    S32         pOffset;        // Bytes between adjacent pels. If this is not 0 or 1,
                                // then the array is not packed. This is here to handle
                                // pictures at the application level. 
    U32         ptrAlias;       // 16:16 alias to ptr (decoder only)
    PIXEL       P32     ptr;    // Data
} COMPONENT;

/* PICTURE - Frame descriptor */
typedef struct {
    S32             color;          // 1 = Color is valid, 0 = luma only
    S32             vXoffset;       // X offset for placing video data 0,8,16..
    S32             vYoffset;       // Y offset for placing video data
    S32             xSize   ;       // Active pels per row
    S32             ySize;          // Active rows
    S16             picLayout;      // Picture layout e.g. S422
    S16             dummy;          // Dummy for alignment
    COMPONENT       y;
    COMPONENT       cb;
    COMPONENT       cr;
    TIME32          tTimestamp;     // Timestamp
} PICTURE;

/* PICTURE_DESCR - Descriptor for Picture layer information */
#define MAX_PEI_COUNT       4
typedef struct {
    S32             codingMethod;   // 0 = H.261, 1 = H.263
    S32             splitscreen;
    S32             doccamera;
    S32             fp_release;
    S32             format;
    S32             rows;
    S32             cols;
    S32             hi_res;
    S32             tr;
    S32             trPrev;         // Temp Reference for prev. P-frame
    S32             ptype;
    S32             interFrame;     // 0 = INTRA picture, otherwise INTER picture
    S32             unrestrictedMv; // 0 = off, otherwise on
    S32             advancedPred;   // 0 = off, otherwise on
    S32             syntax_basedAC; // 0 = off, otherwise on
    S32             PBframeMode;    // 0 = off, otherwise on
    S32             reducedResUpdate;// 0 = off, otherwise QUANT for Reduced-res. Update mode
    S32             deblockingFilterMode;// 0 = off, otherwise on
    S32             advancedIntraMode;// 0 = off, otherwise on
    S32             tempRefBframe;  // Present if PB-frame; add these 3 bits to TR for
                                    // prev. P-frame to get TR for B-frame
    S32             dbQuant;        // quantization info for B-pictures (2 bits)
    S32             peiCount;
    S32             cpm;            // continuous presence multipoint.
    S32             decodingInProgress; // Set to FALSE if feeding new bitstream to decoder
                                    // VvDecode will set it to TRUE if decoder "timed out",
                                    // i.e., didn't finish decoding.  VvDecode needs to be
                                    // called repeatedly until returning FALSE
    U8              pSpare[MAX_PEI_COUNT]; // ;;;
} PICTURE_DESCR;

/***** WARNING WARNING WARNING ******/
/*** This structure must be the same as the top of H261EncoderParams ****/
typedef struct prevparms far *H261PreviewParamsPtr;
typedef struct prevparms {
    S32     rows;           // Number of luma rows in a video frame
    S32     cols;           // Number of luma cols in a video frame
    S32     topSkip;        // Number of rows to discard at the top of the frame
    S32     botSkip;        // Number of rows to discard at the bottom of the frame
    S32     vidType;        // Type of captured video eg S422, YUV9, ... 
    S32     enhancerMode;   // Image enhancer mode. 
    S32     lowRes;         // Low resolution mode.
    S32     monochrome;     // only code luminance component
    S32     gamma;          // gamma by which signal is already predistorted (100 means none, 220 means 2.2).
    S32     mirror;         // True (= non-zero) if previewed image needs to be mirrored (does not affect coded image).
    S32     rotate;         // True (= non-zero) if image needs to be rotated 180 degrees.
    S32     percentile;     // Indicates which percentile we use for gain control. e.g. 50 indicates the median
} H261PreviewParams;

/* The H261EncoderParameters struct passes information from
 * the video stream handler to the H261 encoder.
*/

/***** WARNING WARNING WARNING ******/
/*** The top of this structure must match H261PreviewParams ****/
typedef struct encparms far *H261EncoderParamsPtr;
typedef struct encparms {
    S32     rows;           // Number of luma rows in a video frame
    S32     cols;           // Number of luma cols in a video frame
    S32     topSkip;        // Number of rows to discard at the top of the frame
    S32     botSkip;        // Number of rows to discard at the bottom of the frame
    S32     vidType;        // Type of captured video eg S422, YUV9, ... 
    S32     enhancerMode;   // Image enhancer mode. 
    S32     lowRes;         // Low resolution mode.
    S32     monochrome;     // only code luminance component
    S32     gamma;          // gamma by which signal is already predistorted (100 means none, 220 means 2.2).
    S32     mirror;         // True (= non-zero) if previewed image needs to be mirrored (does not affect coded image).
    S32     rotate;         // True (= non-zero) if image needs to be rotated 180 degrees.
    S32     percentile;     // Indicates which percentile we use for gain control. e.g. 50 indicates the median
    S32     qTargetBits;    // This is the number of bits the coder should try to produce for
                            // the current frame. We will use R(video) * 1/Target_Frame_Rate.
                            // Typically targetBits is within +/- 20% of actual bits acheived.
    S32     minBits;        // Minimum number of bits to generate for the current
                            // frame. Used to avoid under-run condition. 
    S32     maxBits;        // Maximum number of bits to generate for current frame.
    S32     maxQuant;       // Maximum quantizer step to use.
    S32     minQuant;       // Minimum quantizer step to use.
    S32     dbQuant;        // B-frame uses QUANT * (DBQUANT+5)/4; range: 0-3
    S32     K3;             // MahKeeNak controls:
    S32     KAll;
    S32     Min3;
    S32     MinAll;
    S32     wght1;          // Weights for coding control
    S32     wght2;
    S32     codingMethod;   // 0 = H.261, 1 = H.263
    S32     advancedPred;   // 0 = off, otherwise on
    S32     unrestrictedMv; // 0 = off, otherwise on
    S32     syntax_basedAC; // 0 = off, otherwise on
    S32     PBframeMode;    // 0 = off, otherwise on
    S32     sendGobHeaders; // 0 = off, otherwise on
    U32     bScale;         // scale in dibconvert
    ROI_RECT roiRect;       // Region of interest
    U32     forceMbIntra;   // Threshold for forcing intra coding of macroblocks (useful for lossy transmission)
} H261EncoderParams;

/* Temporary definition */
typedef struct {
    long    rows;               // Number of rows in a frame    
    long    cols;               // Number of columns in a frame. For Spigot this should
                                // include y,u,v  e.g. 320 for 1/4
    short   topSkip;            // Number of lines to skip on top of video
    short   botSkip;            // Number of lines to skip on bottom of video.
    short   vidType;            // Indicates video capture board type.
    short   dummy;
    S32     bNewDIB;
    S32     xCur;
    S32     yCur;
    U32     histVal;            // Xth percentile (X is specified by H261EncoderParams.percentile).
    S32     histCount;          // Number of samples in lumaHist.
    S32     lumaHist[256];      // Holds histogram of raw luma data
    unsigned char far *dataPtr1; // Points to start of pixel data (not S422 marker)
    unsigned char far *dataPtr; // Points to start of pixel data (not S422 marker)
} VIDEOFrame;

/* Length of statistics arrays */
#define NUM_TIMERS          7
#define NUM_DCT_STATS       4

typedef struct FrameParams far *FrameParamsPtr;
typedef struct FrameParams {
    U32     data;                 /* frame data */
    U32     data1;                /* frame data */
    S16     fastUpdate;           /* fast upd. */
    S16     ptypeFlags;           /* ptype */
    S16     tempRef;              /* temp. ref. */
    S16     dummy;
    U32     firstBit;             /* pos of first bit */
    S32     effFactor;            /* efficiency factor */
    S32     qTargetBits;          /* quantizer target bits */
    S32     advancedIntraMode;    /* 0=off, otherwise on */
    S32     deblockingFilterMode; /* 0=off, otherwise on */
    S32     reducedResUpdate;     /* 0=off, otherwise QUANT for Reduced-res. Update mode */
    S32     prevReducedResMode;   /* Save reducedResUpdate so that we can disable PB-frames next time */
    S32     bNewDIB;
    U32     fpbDone;              /* indicates that an entire frame has been coded ( smartscreen is done in slices ) */
    S32     xCur;
    S32     yCur;
    S32     fastUpdateGOB;
    S32     fastUpdateFirstGOB;
    S32     fastUpdateNumberGOBs;
    TIME32  time[NUM_TIMERS];   /* encoder times  */
    S32     DCTStats[NUM_DCT_STATS];    /* DCT Stats  */
    U32     peiCount;           
    S16     selectedQuant;
    S16     timeStampBframe;    // Temp ref for B-frame;
    U8      pSpare[MAX_PEI_COUNT];
} FrameParams;
            
/* Label time array */
#define TFLT        0
#define IVAR        1
#define MEST        2
#define PSEL        3
#define QSEL        4
#define DCTC        5
#define VLCC        6

/* Label DCT stats array */
#define DCT_STAT_SKIPPED    0
#define DCT_STAT_ZERO       1
#define DCT_STAT_1ST_THREE  2
#define DCT_STAT_MORE       3

/* Macroblock "map" types and definition */
#define DECODER_MAP     1   // decMB.type 
#define MEST_MAP_X      2   // Motion estimation map type
#define MEST_MAP_Y      3   // Motion estimation map type

typedef struct {
    S32     format;         // QCIF of CIF. Set by user.   
    S32     type;           // 1 = decoder map (others TBD). Set by user.
    U8      data[396];      // Array of data. 1 byte per macroblock. Declare
                            // for CIF and use subset for QCIF.  
} MBMap;
 
/*
 * bit index -- 16:16 byte offset:bit number 
 */
typedef union VvBitIndex FAR * lpVvBitIndex;
typedef union VvBitIndex {
    U32 l;
    struct ww {
        U16 bit;
        U16 byte;
    } ww;
} VvBitIndex;

/* Decoder statistics */
typedef struct VvDecoderStats FAR * lpVvDecoderStats;
typedef struct VvDecoderStats {
    S32 dwStartcodes;
    S32 dwGobs;
    S32 dwBadgobs;
    S32 dwFirstBadgob;
    S32 dwUpdateGobs;
    S32 dwIntraFrame;
    S32 dwPsc;
    VvBitIndex pscIndex;    /* picture start code index */
    S32 dwBframe;           /* Report whether B-frame is returned from VvDecode */
} VvDecoderStats;

/* Possible video board vendors types */ 
enum { S422, YVU9, VLV3, VLV4, DIB };   // S422 = Video Spigot,  YVU9 = Intel Smart Recorder
                                        // VLV3 = Captivator 4:1:1, VLV4 = Captivator Mono
                                        // DIB = Device Independent Bitmap

/* Misc defines*/
#ifndef TRUE
#define FALSE                       0
#define TRUE                        1
#endif

#define NO_ERROR                    0L
#define UNKNOWN_VIDTYPE             1
#define VIDTYPE_NOT_SUPPORTED_YET   2
//#define MAX_BITSTR_SIZE             32768
#define MAX_BITSTR_SIZE             49512

/* VvOpenEncoder and VvCloseEncoder     defines */
#define MAX_NUM_ENCODERS    2

#ifndef AP_VIDEORESIZING
#define MAX_NUM_DECODERS    2
#else
#define MAX_NUM_DECODERS    20
#endif

/* VvOpenEncoder and VvCloseEncoder     error codes */
#define MAX_ENCODERS_OPEN       MAX_NUM_ENCODERS + 10
#define FAILED_MALLOC           MAX_NUM_ENCODERS + 11
#define NO_ENCODERS_OPEN        MAX_NUM_ENCODERS + 12
#define MAX_DECODERS_OPEN       MAX_NUM_ENCODERS + 13
#define NO_DECODERS_OPEN        MAX_NUM_ENCODERS + 14
#define UNKNOWN_PICTURE_FORMAT  MAX_NUM_ENCODERS + 15
        
#endif
