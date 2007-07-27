/*-----------------------------------------------------------------------------
 *  DLLINDEX.H
 *
 *  DESCRIPTION
 * 
 *  SEE ALSO
 *      H.261 standard.     
 *
 *  NOTES
 *      1. For now, must have MAX_NUM_ENCODERS >= MAX_NUM_DECODERS
 *
 *  Author:     Mary Deshon     6/25/93
 *  Inspector:  <<not inspected yet>>
 *  Revised:
 *  01/03/98    S Ericsson  RTP support: added next_mba to H261Decoder;
 *                          added gobHeader to H261DecState.
 *  02/08/97    D Blair     Added deblockingFilterMode, advancedIntra Mode to H261PicDescriptor
 *  01/31/97    S Ericsson  Added reducedResUpdate to H261PicDescriptor
 *  12/19/96    S Ericsson  Doublebuffering in decoder; state vars. for dec. part of picture
 *  11/01/96    S Ericsson  Support PB-frames
 *  06/20/96    S Ericsson  Added prevBits/Quality/K to H261CodingControl
 *  09/13/95    md          Modifications to support GFID.
 *  07/03/95    S Ericsson  Modified H261PicDescriptor and H261Decoder structures for H.263
 *  03/14/95    md          Minor changes to support record and playback.
 *  02/22/95    G Su        Added vidType to H261CodingControl structure.
 *  01/16/95    M Deshon    Increase size of decoder status.
 *  11/15/94    M Deshon    Remove newOut from H261Encoder structure.
 *  09/26/94    M Deshon    Add coder state.
 *  09/12/94    J Bruder    Added PEI and PSPARE to h261PicDescriptor.
 *  07/28/94    M Deshon    Back to using a single decoder map.
 *  06/09/94    M Deshon        Added another decMB to H261Decoder struct.
 *  06/06/94    M Deshon        Added weight_1 & weight_2 to H261CodingControl struct.
 *  05/10/94    M Deshon        Added MahKeeNac controls to H261CodingControl struct.
 *  02/24/94    M Deshon        Added motion vector map structure to H261Encoder.
 *  11/16/93    J Bruder        Changed format to formatCap in H261Encoder and H261Decoder
 *  11/14/93    M Deshon        Added minQuant element to H261CodingControl structure.
 *  09/13/93    M. Deshon       Added FrameInfo2 to H261Stats
 *  08/26/93    M. Deshon       Added space for saving 8x8 block variances
 *  08/06/93    M. Deshon       Added MbInfo to H261Stats struct
 *  07/28/93    M. Deshon       Added format field to H261Decoder struct
 *  07/25/93    M. Deshon       Moved MAX_BITSTR_SIZE into vvenc.h
 *  07/22/93    S. Ericsson     Added to H261Stats
 *  07/18/93    M Deshon        Added BS_PTR for fixed bitstream start.
 *  07/16/93    S. Ericsson     Replaced some pointers with actual structures
 *  07/06/93    M Deshon        Modified H261Encoder and H261PicDescriptor structures
 *                              in H261Encoder and H261Decoder
 *
 *  (c) 1993-1998, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/

#ifndef _INC_DLLINDEX
#define _INC_DLLINDEX   1

#include "h261type.h"
#include "vvenc.h"
#include "vvenccod.h"   // Web project only

typedef enum coderState {
    framePreProcessed,
    frameCoding,
    frameReconstructed
} coderState;


/* The H261CodingControl struct passes bitrate control information from
 * "the outside world" to the H261 encoder.
*/
typedef struct {
    long    qTargetBits; // This is the number of bits the coder should try to produce for
                        // the current frame. We will use R(video) * 1/Target_Frame_Rate.
                        // Typically targetBits is within +/- 20% of actual bits acheived.
    long    minBits;    // Minimum number of bits to generate for the current
                        // frame. Used to avoid under-run condition. 
    long    maxBits;    // Maximum number of bits to generate for current frame.
    S32     effFactor;  // Efficiency factor (used in QuantSelect).
    short   maxQuant;   // Maximum quantizer step to use.
    short   minQuant;   // Minimum quantizer step to use.
    S32     K3;         // MahKeeNack controls:
    S32     KAll;
    S32     Min3;
    S32     MinAll;
    S32     weight_1;   // Tweak factors for coding type decision.
    S32     weight_2;
    S32     sendGobHeaders;
    S32     prevBits;       // Bits produced in last frame (used by QuantSelect)
    S32     prevQuality;    // Quant/"quality" in last frame (used by QuantSelect)
    double  prevK;          // K value computed in last frame (used by QuantSelect)
    unsigned short  vidType;    // Frame type
} H261CodingControl;


/* The H261Stats struct retains information the coder uses to
 *  regulate its output.
*/
typedef struct {
    MbInfo      *mi;                // Array of MbInfo structures
    FrameInfo   fi;                 // Frame information structure for tuning 
    long        prevTargetBits;     // Target bits for last frame
    long        prevTotalBits;      // Total bits generated for last frame
    long        *GOBError;          // Error for each GOB
    long        *GOBBits;           // Number of bits generated for each GOB
    long        frame_counter;      // Incremented after each picture to generate
                                    // "phases" for subsampling etc. No need to initialize. 
    short       *GOBQuant;          // Quantizer step used for each GOB 
} H261Stats;                
                                                                                

/* The H261Encoder struct defines the I/O and steady-state data
 * for the H261 encoder.
 * NOTE: This stucture is only passed as a pointer, so we don't worry about its size. md 02.24.94
*/
typedef struct h261enc far *H261EncoderPtr;

typedef struct h261enc {
    PICTURE             newIn;      // New input frame (second input frame if PBframeMode)
    PICTURE             oldIn;      // Old input frame/prediction/newOut
    PICTURE             oldOut;     // Old output frame
    PICTURE             B_In;       // First input frame (if PBframeMode)
    PICTURE             BPred;      // Temporary array used for B-frame encoding
    BS_PTR              bsStart;    // Points to start of bitstream 
    BS_PTR              bsEnd;      // Updated bitstream pointer
    H261CodingControl   cntrl;      // Coding control structure
    H261Stats           stats;      // Coder statistics
    GOB_DESCR           *gob;       // Array of GOB descriptors
    MACROBLOCK_DESCR    *mb;        // Array of MB descriptors
    long                *mbDiff;    // Sq. diff between newIn and oldIn for each MB
    long                *blkIntra;  // 8x8 block variances
    SYMBOL              *sym;       // Work array to hold Run-Level symbols
                                    // Possible to use same area for oldOut and sym 
                                    // (although sym is larger for worst-case)
    long                maxsym;     // Dimension of sym[]
    BS_PTR              bsAbs;      // Points to fixed bitstream area.
    short               formatCap;  // CIF or QCIF
    short               dummy;      // struct alignment 
    int                 ptype;      // PTYPE field of last bitstream
    int                 gfid;       // GFID field of last bitstream
    int                 trPrev;     // Hold on to Temp Reference for prev. picture
    enum coderState     state;      // coder state
    MBMap               mestMap[2]; // Map of motion vector macroblock types.
} H261Encoder; 


/* The H261PicDescriptor struct passes picture layer information from
 *  "the outside world" to the coder.
*/
typedef struct {
    unsigned short  temporalRef;    // Temporal reference. 5 bits indicating number of
                                    // frames "skipped" ( see H.261 standard )
    unsigned short  fastUpdate;     // Fast update request, 1 = do entire frame Intra
    unsigned short  splitScreen;    // 0 = off, 1 = on
    unsigned short  docCamera;      // 0 = off, 1 = on
    unsigned short  format;         // 0 = QCIF, 1 = CIF, 2 = other
    unsigned short  hiRes;          // 0 = on, 1 = off 
    unsigned short  codingMethod;   // 0 = H.261, 1 = H.263
    unsigned short  unrestrictedMv; // 0 = off, 1 = on
    unsigned short  advancedPred;   // 0 = off, 1 = on
    unsigned short  syntax_basedAC; // 0 = off, 1 = on
    unsigned short  PBframeMode;    // 0 = off, 1 = on
    unsigned short  reducedResUpdate;       // 0 = off, 1 = on
    unsigned short  deblockingFilterMode;   // 0 = off, 1 = on
    unsigned short  advancedIntraMode;      // 0 = off, 1 = on
    unsigned short  tempRefBframe;  // Present if PB-frame; add these 3 bits to TR for
                                    // prev. P-frame to get TR for B-frame
    unsigned short  dbQuant;        // Quantization info for B-pictures (2 bits)
    unsigned long   width;          // Width of luma 
    unsigned long   height;         // Height of luma
    unsigned long   peiCount;       // Number of p
	unsigned long	forcedUpdateThresh;		// Number of Inter Coded Frames before Intra update
    unsigned long   fastUpdateGOB;          // 1 = Fast Update a subset of GOBs
    unsigned long   fastUpdateFirstGOB;     // First GOB to intracode
    unsigned long   fastUpdateNumberGOBs;   // Number of GOB
    unsigned char   pSpare[MAX_PEI_COUNT];
} H261PicDescriptor;


// H261DecState contains state variables to support decoding in "chunks"
#define SC_MAX              (50)    // Max # of startcodes to process per call
typedef struct {
    BS_PTR  bspStartCode[SC_MAX];   // Position of startcodes ("start")
    int     gobNumber[SC_MAX];      // GOB numbers following each startcode ("gn")
    int     nStartCodes;            // Number of startcodes found ("num_sc")
    int     currentStartCode;       // Startcode currently processed ("sc")
    GOB_DESCR   currentGob; // Can refer to several GOBs, if GOB headers are left out ("gob")
    int     mbnum;          // internal variable in recon_gob
    int     col;            // internal variable in recon_gob
    int     i;              // internal variable in recon_gob
    int     actComp;        // Keeps track of amount of computations
    int     gobHeader;      // Indicates whether currentGob has a header
} H261DecState;


/* The H261Decoder struct defines the I/O for the H261 decoder.
 * NOTE: This stucture is only passed as a pointer, so we don't worry about its size. md 02.24.94
*/
#define DECODER_STATUS_SIZE 4
typedef struct { 
    BS_PTR              bsStart;    // Start of bitstream
    BS_PTR              bsEnd;      // End of bitstream
    BS_PTR              newBs;      // Returns next "bsStart"
    BS_PTR              bsAbs;      // Points to fixed bitstream area.
    BS_PTR              bsAbsEnd;   // Points to fixed bitstream end.
    MACROBLOCK_DESCR *  mb;         // Work array: describes each macroblock
    int                 maxMbnum;   // Dimension of mb[] array
    SYMBOL *            sym;        // Work array for symbol decoding
    int                 maxSym;     // Dimension of sym[] array
    H261DecState        state;      // State variables to support decoding with "timeout"
    int                 maxComp;    // Return when actComp exceeds this number
                                    // Set to zero to decode without "timeout"
    PICTURE             oldOut;     // Old output frame (predictor input)
    PICTURE             newOut;     // New output frame (second output if PBframeMode)
    PICTURE             B_Out;      // First output frame (if PBframeMode)
    PICTURE             prevOldOut; // Doublebuffering of output frames
    PICTURE *           pNewPic;    // Points to most recent output frame
    PICTURE *           pPrevPic;   // Points to previous output frame
    PICTURE             convertPic; // Used for any necessary upsampling after decoding
    PICTURE_DESCR       pic_layer;  // Returns Picture layer info
    S32                 next_gob;   // I/O: Next GOB to process. Search for PSC if 0.
    S32                 next_mba;   // I/O: Next macroblock address (Added 1/3/98 for RTP support)
    VvDecoderStats      status;     // Return status
    short               formatCap;  // CIF or QCIF 
    unsigned short      codingMethod;   // 0 = H.261, 1 = H.263
    S32                 PBframeCap; // If 0: don't reconstruct B frame (just parse the bits)
    S32                 pendingFrame;   // Set to 1 when returning B-frame, otherwise 0
    int                 ptype;      // PTYPE field of last bitstream
    int                 gfid;       // GFID field of last bitstream
    MBMap               decMB;      // Map of decoded macroblock types.
} H261Decoder; 


/* Use this frame definition until videoCompressionStreamHandler is written */
typedef struct frame far * FramePtr;

typedef struct frame {
    unsigned long   lpData;     // Point to data (cannot use * because of DLL hooey)
    unsigned long   width;      // Width of luma 
    unsigned long   height;     // Height of luma
    unsigned long   imgSize;    // Total Size of Image Data
    unsigned long   imgOffset;  // Offset to image data
    unsigned short  chromaDiv;  // To get size of chroma planes
    unsigned short  vidType;    // Frame type
} Frame;


/* Misc defines*/
#define NO_ERROR            0L
#define UNKNOWN_VIDTYPE     1
#define VIDTYPE_NOT_SUPPORTED_YET   2

/* VvOpenEncoder and VvCloseEncoder defines */
#define MAX_NUM_ENCODERS    2

#ifndef AP_VIDEORESIZING
#define MAX_NUM_DECODERS    2
#else
#define MAX_NUM_DECODERS    20
#endif

/* VvOpenEncoder and VvCloseEncoder error codes */
#define MAX_ENCODERS_OPEN       MAX_NUM_ENCODERS + 10
#define FAILED_MALLOC           MAX_NUM_ENCODERS + 11
#define NO_ENCODERS_OPEN        MAX_NUM_ENCODERS + 12
#define MAX_DECODERS_OPEN       MAX_NUM_ENCODERS + 13
#define NO_DECODERS_OPEN        MAX_NUM_ENCODERS + 14
#define UNKNOWN_PICTURE_FORMAT  MAX_NUM_ENCODERS + 15
                
#endif
