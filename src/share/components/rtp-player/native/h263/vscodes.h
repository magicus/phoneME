/*-----------------------------------------------------------------------------
 *  vscodes.h
 *  $Id: vscodes.h 1.99 1995/07/13 15:00:02 george Exp $
 *
 *  NAME  
 *    VSCODES -- code symbols for Vivo streams.
 *
 *  SYNOPSIS
 *    #include "vscodes.h"
 *     ...
 *
 *  DESCRIPTION:
 *    This file holds the constants used throughout the VISION project.
 *    These constants include STREAM ID numbers, value codes (i.e., what
 *    kind of picture, kind of camera, etc.), parameter ID numbers, and 
 *    stream state values.  The following are rough breakdowns of the 
 *    ranges for each:
 *
 *		*******************************************
 *		!!!!!!WARNING THIS TABLE IS OUT OF DATE!!!!
 *		*******************************************
 *      WHAT                    LOW     HIGH    COUNT
 *      ====                    ====    ====    =====
 *      STREAM IDs              7E00    7E2F      48
 *      Value Codes             7E30    7E5F      48
 *      Parameter IDs           7E60    7FCF     464
 *        General (All Streams) 7E60    7E91      50
 *        Audio Stream          7E92    7EA0      15
 *        Transmit Stream       7EA1    7EBF      30
 *        Source Stream         7EC0    7ECE      15
 *        Sink Stream           7ECF    7EDD      15
 *        Coder/Encoder Streams 7EDE    7F05      40
 *        Smart Screen Stream   7F06    7F0F      10
 *        Echo Suppressor       7F10    7F2D      30
 *        Session Manager       7F2E    7F3C      15
 *        Communication Manager 7F3D    7F64      40
 *        H.DLL MUX/DEMUX       7F65    7F78      20
 *        FREE SPACE            7F79    7FCF      96
 *      Stream States           7FD0    7FDF      16
 *      Stream Messages         7FE0    7FFF      32
 *  NOTE: If you change any of these values, please update this table.
 *
 *  SEE ALSO
 *    Video For Windows DK, MMDK, etc.
 *
 *
 *  Author:     Ollie Jones
 *  Inspector:
 *  Revised (most recent first):
 * 05/01/96 md		VVS_SEND_GOB_HEADERS.
 * 02/25/96 md		VVS_ADVANCED_PRED & VVS_UNRESTRICTED_MV.
 * 01/13/95 bruder	changed movieman streamn types to audio compression and decompression.
 * 01/05/96 chet    defines for H.245 Protocol Manager
 * 12/28/95 Crofton Added VVS_AUDIO_IO stream
 * 10/11/95 Bruder  Added VVS_COMM_MNGR_LCL_CMD and VVS_COMM_MNGR_REM_CMD
 * 10/09/95 bruder  added VVS_SOURCE_DIALOG, VVS_DISPLAY_DIALOG, and VVS_FORMAT_DIALOG.
 * 09/12/95 bruder  Merged Missouri changes.
 * 09/05/95 chet    Added H.223 Adaptation Layer Stream IDs (VVS_ALn(X|R)_STREAM)
 * 07/25/95 jb      Merged Brazil changes
 * 05/05/95 md      Add VVS_MODEM_HARD_LOOP.
 * 05/03/95 md      Add modem xmit/recv stream constants. Add VVS_MODEM_SOFT_LOOP.
 * 06/12/95 bruder  added VVS_COMM_MNGR_REMOTE_COUNTRY
 * 05/03/95 wolfe   BUG#2277 - Added VVS_OUTDIB_FORMAT to decoder params
 * 04/12/95 wolfe   BUG #2138 - Added SessMngr param for setting directory
 * 03/16/95 chet    Added 2 parameters for OS/2 to switch display windows
 * 03/14/95 md      Parameters to support record/playback
 * 02/22/95 md      Add decoder BCH enable/disable
 * 02/14/95 md      Add record and playback stream types
 * 01/06/95 db      Add parameters for line status/LAN
 * 10/25/94 md      Add parameters for manual exposure 
 * 09/16/94 md      Add INTENSITY_PERCENTILE 
 * 09/15/94 oj      Add lighting 
 * 09/09/94 oj      Add VVS_GAMMA, VVS_UPSIDE_DOWN, VVS_MIRROR
 * 09/05/94 md      Added VVS_FORMAT
 * 08/25/94 Bruder  Added VVS_LIP_SYNC
 * 08/09/94 dbrown  Added VVS_COMM_MNGR_INCOMING_CALL for business card xfer
 * 07/22/94 dbrown  Added VVS_XMIT_CONN_CONFIRMS
 *  07/09/94 md     Added VVS_VIDEO_HOLD
 * 07/08/94 wolfe   Added VVS_SES_MNGR_LOOPBACK_LEVEL
 * 07/06/94 Bruder  Added VVS_AUDIO_DELAY_FLUFF and VVS_AUDIO_PRIVACY.                    
 * 07/06/94 md      VVS_ENCODE_BAL_DISPLAY for coder stream.
 * 06/29/94 wc      Add VVS_CONFIGURE to invoke video driver config dlg
 * 06/27/94 md      VVS_FRAMESDISPLAYED for decoder stream.
 * 06/14/94 dbrown     Renumbered to provide XMIT Stream w/more codes. VVS_TRANSFER_MODE &
 *                VVS_XMIT_MCU_MODE. VVS_COMM_MNGR_CAUSE_CODE.
 * 06/14/94 md      VVS_WGHT1 and VVS_WGHT2 for coder stream.     
 * 06/06/94 gh      VVS_SMTSCR_CAPTURE_DIB_INTERVAL, VVS_SMTSCR_CAPTURE_CURSOR_INTERVAL      
 * 06/04/94 Bruder     VVS_MVMAN_IN_STREAM and VVS_MVMAN_OUT_STREAM
 *  06/02/94 md        Added support for MovieMan.
 *  06/02/94 dbrown   H221 NonStandard Command parameters.
 *  05/20/94 md        VVS_DISPLAY_FREEZE and VVS_DISPLAY_DIB for decoder stream.
 *  05/16/94 dbrown   VVS_AUD_MODE for A-Law/MU-Law differentiation
 *  05/16/94 wolfe    Added VirCOMMport VxD signalling IDs 
 *                    WM_POSTED_BY_VFAST_VXD and WM_POSTED_BY_VVCOMM
 *  05/12/94 dbrown   Control for Restricted/Unrestricted ISDN calling
 *  05/09/94 md       Controls for MahKeeNak DCT
 *  04/25/94 gh       Added SMTSCR_USAGE parameter    
 *  04/12/94 gh       Added DrawDIBDraw Timing Parameters       
 *  04/08/94 wolfe    Added H.DLL parameter for setting complement stream
 *  04/07/94 wolfe    Added parameters for H.DLL and gave H.DLL MUX/DEMUX
 *                    10 more parameter spaces for future expansion
 *  04/04/94 wolfe    More room for stream parameters, added VVS_NO_MESSAGE
 *                    Cleaned up & added sections for various streams.
 *  03/23/94 Bruder   Added VVS_AUD_SRC/SINK streams.
 *  03/20/94 md       Added parameters for bit-rate control.
 *  03/08/94 dbrown   Consolidated CommMgr codes freeing up #124-128, inclusive
 *  03/01/94 md       Added VVS_MEST_MAP      
 *  02/22/94 Bruder   Added VVS_ECHO_SYNC_ERROR      
 *  02/16/94 gh       Added code for Viewer pseudo-stream       
 *  02/14/94 wolfe    Added codes for MUX/DEMUX of H.DLL
 *  02/11/94 oj       Added BMACIO stream types, related parameters
 *  02/04/94 dbrown   Added VVS_COMM_MNGR parameters for non-standard capabilities exchanges
 *  02/02/94 dbrown   Added VVS_COMM_MNGR parameters
 *  12/29/93 Bruder   Added echo suppression parameters
 *  12/27/93 wolfe    Added VVS_NULL stream state type, renumbered states
 *                    Changed stream # ordering for automated STARTing
 *  12/17/93 md       Added more coder stream params.
 *  12/17/93 OJ       Added SWAN audio I/O stream ids.
 *  12/09/93 Bruder   Added output circular buffer param    
 *  12/02/93 gerry    Added Session Manager param        
 *  11/23/93 md       Added SmartScreen Params
 *  11/14/93 md       Added VVS_QUANTIZER_MIN
 *  11/08/93 Bruder   Added VVS_MONOCHROME and VVS_LAYOUT_VLV4
 *  11/03/93 wolfe    Added support for source/sink streams
 *  11/02/93 jb       Added VVS_LOWRES
 *  10/08/93 jb       Added define for Captivator 
 *  09/27/93 md       added defines for audio loop-back
 *  09/19/93 md       added parameters specific to audio input stream
 *  09/15/93 md       added bits-per-frame
 *  09/06/93 oj       work on timing features (bug 18)
 *  08/11/93 oj       added VVS_SPECIALFRAME to help with tuning support  
 *  08/02/93 deshon   added TEE format for layouts
 *
 *  (c) 1993, Vivo Software, Inc.  All rights reserved 
 -----------------------------------------------------------------------------*/ 

#ifndef _VSCODES_H_
#define _VSCODES_H_

/* stream types (usable as WM_USER messages if need be) */
#define VVS_BASE            0x7E00 
#define VVS_FIRST_STREAM    (VVS_BASE + 1)
#define VVS_VID_PREVIEW     (VVS_BASE + 1)
#define VVS_VID_DISPLAY     (VVS_BASE + 2)
#define VVS_VID_DECOMPRESS  (VVS_BASE + 3)
#define VVS_VID_COMPRESS    (VVS_BASE + 4)
#define VVS_VID_IN          (VVS_BASE + 5)
#define VVS_AUD_OUT         (VVS_BASE + 6)
#define VVS_AUD_IN          (VVS_BASE + 7)
#define VVS_TIMER           (VVS_BASE + 8)
#define VVS_MUX_XMIT        (VVS_BASE + 9)
#define VVS_MUX_RECV        (VVS_BASE + 10) 

#define VVS_COMPRESSED_BUF  (VVS_BASE + 11)
#define VVS_RECEIVED_BUF    (VVS_BASE + 12) 

/* Used to identify wave audio input and output stream	*/
#define VVS_WAVE_IN       (VVS_BASE + 13)
#define VVS_WAVE_OUT      (VVS_BASE + 14)

/* Source and Sink Streams */
#define VVS_SOURCE_STREAM   (VVS_BASE + 15)
#define VVS_SINK_STREAM     (VVS_BASE + 16)

/* SmartScreen Stream */
#define VVS_SMTSCR          (VVS_BASE + 17)

/* Session Manager Stream */
#define VVS_SES_MNGR_STREAM (VVS_BASE + 18)

/* Echo Cancellation Tx and Rx Streams */
#define VVS_ECHO_TX			(VVS_BASE + 19)
#define VVS_ECHO_RX			(VVS_BASE + 20)

/* Communications Manager */
#define VVS_COMM_MNGR_STREAM (VVS_BASE + 21)

/* BMAC (ISDN B-channel media access) IO stream */
#define VVS_BMAC_IO_STREAM   (VVS_BASE + 22)

/* H.DLL Multiplexor and Demultiplexor streams */
#define VVS_HDLL_MUX_STREAM   (VVS_BASE + 23)
#define VVS_HDLL_DEMUX_STREAM (VVS_BASE + 24)  

/* REMOTE VIEWER pseudo-display stream */
#define VVS_REMOTE_STREAM    (VVS_BASE + 25) 

#define VVS_AUD_SRC_STREAM   (VVS_BASE + 26)
#define VVS_AUD_SINK_STREAM  (VVS_BASE + 27)

/* audio encode and decode streams	*/
#define VVS_AUDIO_ENC		(VVS_BASE + 28)
#define VVS_AUDIO_DEC		(VVS_BASE + 29)

/* NetLayer Client/Gateway streams */
#define VVS_NET_CLIENT_STREAM   (VVS_BASE + 30)
#define VVS_NET_GATEWAY_STREAM  (VVS_BASE + 31)

/* Record and Playback */
#define VVS_RECORD_STREAM       (VVS_BASE + 32)
#define VVS_PLAYBACK_STREAM     (VVS_BASE + 33)

/* Record and Playback */
#define VVS_XMT_MLP_STREAM      (VVS_BASE + 34)
#define VVS_RCV_MLP_STREAM      (VVS_BASE + 35)

/* H.223 Adaptation Layer Streams (Xmit & Recv) */
#define VVS_AL1X_STREAM         (VVS_BASE + 36)
#define VVS_AL1R_STREAM         (VVS_BASE + 37)
#define VVS_AL2X_STREAM         (VVS_BASE + 38)
#define VVS_AL2R_STREAM         (VVS_BASE + 39)
#define VVS_AL3X_STREAM         (VVS_BASE + 40)
#define VVS_AL3R_STREAM         (VVS_BASE + 41)

/* H.223 MUX streams */
#define VVS_H223_MUX_XMIT       (VVS_BASE + 42)
#define VVS_H223_MUX_RECV       (VVS_BASE + 43)

/* H.245 Control Channel Streams */
#define VVS_H245X_STREAM         (VVS_BASE + 44)
#define VVS_H245R_STREAM         (VVS_BASE + 45)

/* H.245 Protocol Manager Stream */
#define VVS_PROTOCOL_MNGR_STREAM (VVS_BASE + 46)

#define VVS_ESS1888_IN	(VVS_BASE + 47)
#define VVS_ESS1888_OUT	(VVS_BASE + 48)

#define VVS_ATT_IN		(VVS_BASE + 49)
#define VVS_ATT_OUT		(VVS_BASE + 50)


#define VVS_LAST_STREAM          (VVS_BASE + 50)


/***** NO MORE THAN 47 STREAM TYPES (VVS_BASE + 47) *****/

/* various value-code names */
#define VVS_CODE_BASE       VVS_BASE + 0x30 
/* data layouts (memory formats as captured) */

#define VVS_LAYOUT_PICTURE  (VVS_CODE_BASE + 0)
#define VVS_LAYOUT_S422     (VVS_CODE_BASE + 1)
#define VVS_LAYOUT_YVU9     (VVS_CODE_BASE + 2)
#define VVS_LAYOUT_LUMA     (VVS_CODE_BASE + 3)
#define VVS_LAYOUT_TEE      (VVS_CODE_BASE + 4)
    /* see below for additional layouts */

/* source selections -- values of VVS_CAMERA */
#define VVS_CAMERA_MAIN     (VVS_CODE_BASE + 5)
#define VVS_CAMERA_DOC      (VVS_CODE_BASE + 6)
#define VVS_CAMERA_AUX      (VVS_CODE_BASE + 7)
#define VVS_CAMERA_SPLIT    (VVS_CODE_BASE + 8)
#define VVS_VCR             (VVS_CODE_BASE + 9)

/* resolutions */
#define VVS_NORMALRES       (VVS_CODE_BASE + 10) 
/* high resolution (Annex D to H.261) */
#define VVS_HIRES           (VVS_CODE_BASE + 11)

/* Layout for Captivator    */
#define VVS_LAYOUT_VLV3     (VVS_CODE_BASE + 12)        // Color 4:1:1
#define VVS_LAYOUT_VLV4     (VVS_CODE_BASE + 13)        // Monochrome

/* Layout for SmartScreen    */
#define VVS_LAYOUT_DIB     (VVS_CODE_BASE + 14)
/* Layout for MovieMan    */
#define VVS_LAYOUT_MOVMAN  (VVS_CODE_BASE + 15)
/* Layout for BtV    */
#define VVS_LAYOUT_BTV    (VVS_CODE_BASE + 16)
//#define VVS_LAYOUT_YVU9   (VVS_CODE_BASE + 17)
#define VVS_LAYOUT_YUV9   (VVS_CODE_BASE + 18)
#define VVS_LAYOUT_YUY2   (VVS_CODE_BASE + 19)
#define VVS_WINDOW_HANDLE (VVS_CODE_BASE + 20)        // VidCap and VidDisp hwnd
/* DIBs forming a video stream (WEB Producer)*/
#define VVS_LAYOUT_DIB_VIDEO     (VVS_CODE_BASE + 21)

/***** NO MORE THAN 47 VALUE-CODE NAMES (VVS_C5DE_BASE + 47) *****/


/* stream parameter names */   
#define VVS_PARAM_BASE      VVS_BASE + 0x60 

#define VVS_LOAD_MWAVE_TIMER_TASK (VVS_PARAM_BASE + 0)
/* the state of the stream (see VVS_STATE_BASE codes) */
#define VVS_STATE           (VVS_PARAM_BASE + 1)
/* the frame time in microseconds */ 
#define VVS_FRAME_TIME      (VVS_PARAM_BASE + 2)
/* the delay time in microseconds */
#define VVS_DELAY_TIME      (VVS_PARAM_BASE + 3)
/* the maximum processing time in microseconds  */
#define VVS_PROC_TIME       (VVS_PARAM_BASE + 4)
/* the total buffer time in microseconds */
#define VVS_TOTAL_TIME      (VVS_PARAM_BASE + 5)
/* format of stream: BAS_VICAP_CIF, BAS_VICAP_QCIF, BAS_AUCAP_ULAW...*/
#define VVS_CAP             (VVS_PARAM_BASE + 6)
/* number of rows in captured video or DIB */
#define VVS_ROWS            (VVS_PARAM_BASE + 7)
/* number of columns in captured video Or DIB */
#define VVS_COLS            (VVS_PARAM_BASE + 8)
/* buffer size in bytes */
#define VVS_BUFSIZE         (VVS_PARAM_BASE + 9)
/* buffer count */
#define VVS_BUFCOUNT        (VVS_PARAM_BASE + 10)
/* buffer count */
#define VVS_FRAMECOUNT      (VVS_PARAM_BASE + 11)
/* timestamp */
#define VVS_TIMESTAMP       (VVS_PARAM_BASE + 12)
/* frame loss */
#define VVS_FRAMELOSS       (VVS_PARAM_BASE + 13)
/* audio volume  -- range 0xffff to zero */
#define VVS_VOLUME          (VVS_PARAM_BASE + 14)
/* audio mute -- nonzero means muted */
#define VVS_MUTE            (VVS_PARAM_BASE + 15)
/* stream from which this stream gets its input */
#define VVS_INPUTSTREAM     (VVS_PARAM_BASE + 16)
/* stream to which this stream sends its output */
#define VVS_OUTPUTSTREAM    (VVS_PARAM_BASE + 17)
/* coder state */
#define VVS_CODER_STATE     (VVS_PARAM_BASE + 18)
/* frame layout (as captured off the video capture board) */
#define VVS_LAYOUT          (VVS_PARAM_BASE + 19)
/* nominal bit rate, bits per second */
#define VVS_BITRATE         (VVS_PARAM_BASE + 20) 
/* camera or other media source VVS_CAMERA_MAIN, VVS_CAMERA_DOC...*/
#define VVS_SOURCE          (VVS_PARAM_BASE + 21) 

/* parameters for video codec stream handlers */
/* set FAST_UPDATE in video encoder stream to request intraframe */
#define VVS_FAST_UPDATE     (VVS_PARAM_BASE + 22)
/* set FREEZE_PICTURE_RELEASE in video encoder stream to send FPR to far end */
#define VVS_FREEZE_PICTURE_RELEASE (VVS_PARAM_BASE + 23)
/* set FREEZE_PICTURE in video decoder to freeze picture; reset when FPR arrives */
#define VVS_FREEZE_PICTURE  (VVS_PARAM_BASE + 24)
/* values: VVS_NORMALRES, VVS_HIRES (Annex D) */
#define VVS_RES             (VVS_PARAM_BASE + 25)
/* encoder max quantizer value (tuning parameter) */
#define VVS_QUANTIZER_MAX   (VVS_PARAM_BASE + 26) 
/* decoder number of start codes processed */
#define VVS_STARTCODES      (VVS_PARAM_BASE + 27)
/* decoder number of GOBs processed */
#define VVS_GOBS            (VVS_PARAM_BASE + 28)
/* decoder number of erroneous GOBs skipped */
#define VVS_BADGOBS         (VVS_PARAM_BASE + 29)
/* stream to which this stream sends its output */
#define VVS_DISPLAYSTREAM   (VVS_PARAM_BASE + 30)
/* bits processed */
#define VVS_BITSPROCESSED   (VVS_PARAM_BASE + 31)
/* bits-per-second (actual bitrate, derived from timestamp */
#define VVS_BITSPERSECOND   (VVS_PARAM_BASE + 32)
/* special-frame -- frame number to get special handling */
#define VVS_SPECIALFRAME    (VVS_PARAM_BASE + 33) 

/* timing-instrumentation parameters */
#define VVS_TIME1           (VVS_PARAM_BASE + 34)
#define VVS_TIME2           (VVS_PARAM_BASE + 35)

/* bits-per-frame */
#define VVS_BITSPERFRAME    (VVS_PARAM_BASE + 36)

/* all streams / queue fullness indicators */
#define VVS_RET_Q_COUNT     (VVS_PARAM_BASE + 37)
#define VVS_OUT_Q_COUNT     (VVS_PARAM_BASE + 38)

#define VVS_LOWRES          (VVS_PARAM_BASE + 39) 
#define VVS_MONOCHROME      (VVS_PARAM_BASE + 40)
/* Tell the driver to start the configure dialog box */
/* This is a boolean.  If you set it, it invokes the config dlg. */
#define VVS_CONFIGURE       (VVS_PARAM_BASE + 41)

/* Parameters for manual exposure control*/
#define VVS_CAMERA_AGC      (VVS_PARAM_BASE + 42)
#define VVS_CAMERA_SHUTTER  (VVS_PARAM_BASE + 43)
#define VVS_CAMERA_GAIN     (VVS_PARAM_BASE + 44)
#define VVS_VIDEO_ENHANCER  (VVS_PARAM_BASE + 45)

/* Enable and  disable holding bits in bistream arena until entire frame is decoded. */
#define VVS_DECODER_ENABLE_BS_HOLD  (VVS_PARAM_BASE + 46)
/* Set display state */
#define VVS_DECODER_DISPLAY_STATE   (VVS_PARAM_BASE + 47)
/* Byte offset to bitstream receive index */
#define VVS_DECODER_RECV_BYTE (VVS_PARAM_BASE + 48)

/* Left room for up to (VVS_PARAM_BASE + 49) general parameters */

/* Audio Stream Parameters */
#define VVS_AUD_PARAMETERS  (VVS_PARAM_BASE + 50)
#define VVS_SILENT          (VVS_AUD_PARAMETERS + 0)
#define VVS_LEVEL           (VVS_AUD_PARAMETERS + 1)
#define VVS_CODING          (VVS_AUD_PARAMETERS + 2)
#define VVS_BITS_PER_SAMPLE (VVS_AUD_PARAMETERS + 3)
#define VVS_FREQ_RANGE      (VVS_AUD_PARAMETERS + 4)
#define VVS_SINK            (VVS_AUD_PARAMETERS + 5)
#define VVS_AUDIO_CAPS       (VVS_AUD_PARAMETERS + 6)
#define VVS_AUDIO_DELAY_FLUFF (VVS_AUD_PARAMETERS + 7)
#define VVS_AUDIO_PRIVACY   (VVS_AUD_PARAMETERS + 8)
#define VVS_LIP_SYNC        (VVS_AUD_PARAMETERS + 9)
#define VVS_GAIN            (VVS_AUD_PARAMETERS + 10)
#define VVS_PRE_ECHO_GAIN   (VVS_AUD_PARAMETERS + 11)
#define VVS_START_TIME      (VVS_AUD_PARAMETERS + 12)

/* Left 15 spaces for audio stream parameters */


/* Xmit stream */
#define VVS_XMIT_PARAMETERS     (VVS_PARAM_BASE + 65)
#define VVS_AUDIOSTREAM         (VVS_XMIT_PARAMETERS + 0)
#define VVS_VIDEOSTREAM         (VVS_XMIT_PARAMETERS + 1)
#define VVS_DATASTREAM          (VVS_XMIT_PARAMETERS + 2)
#define VVS_AUDIO_BITRATE       (VVS_XMIT_PARAMETERS + 3)
#define VVS_VIDEO_BITRATE       (VVS_XMIT_PARAMETERS + 4)
#define VVS_DATA_BITRATE        (VVS_XMIT_PARAMETERS + 5)
#define VVS_AUDIO_UNDERRUN      (VVS_XMIT_PARAMETERS + 6)
#define VVS_VIDEO_UNDERRUN      (VVS_XMIT_PARAMETERS + 7)
#define VVS_DATA_UNDERRUN       (VVS_XMIT_PARAMETERS + 8)
#define VVS_MUX_XMIT_AUD_MODE_REQ   (VVS_XMIT_PARAMETERS + 9)
#define VVS_MUX_XMIT_DATA_MODE_REQ  (VVS_XMIT_PARAMETERS + 10)
#define VVS_MUX_XMIT_VID_MODE_REQ   (VVS_XMIT_PARAMETERS + 11)
#define VVS_AUDIO_MODE          (VVS_XMIT_PARAMETERS + 12)
#define VVS_VIDEO_MODE          (VVS_XMIT_PARAMETERS + 13)
#define VVS_TRANSFER_MODE       (VVS_XMIT_PARAMETERS + 14)
#define VVS_XMIT_MCU_MODE       (VVS_XMIT_PARAMETERS + 15)
#define VVS_XMIT_CONN_CONFIRMS  (VVS_XMIT_PARAMETERS + 16)
#define VVS_MLP_STREAM          (VVS_XMIT_PARAMETERS + 17)
#define VVS_MUX_XMIT_HMLP_MODE_REQ   (VVS_XMIT_PARAMETERS + 18)
#define VVS_MUX_XMIT_LMLP_MODE_REQ   (VVS_XMIT_PARAMETERS + 19)
#define VVS_H_MLP_MODE          (VVS_XMIT_PARAMETERS + 20)
#define VVS_L_MLP_MODE          (VVS_XMIT_PARAMETERS + 21)
#define VVS_T120_MODE_REQ       (VVS_XMIT_PARAMETERS + 22)
#define VVS_T120_MODE           (VVS_XMIT_PARAMETERS + 23)
#define VVS_BACKLOG             (VVS_XMIT_PARAMETERS + 24)
/* Left 30 spaces for XMIT stream parameters */


/* Source stream parameters */
#define VVS_SRC_PARAMETERS      (VVS_PARAM_BASE + 95)
#define VVS_SRC_PATTERN_SIZE    (VVS_SRC_PARAMETERS + 0)
#define VVS_SRC_PATTERN         (VVS_SRC_PARAMETERS + 1)
#define VVS_SRC_COUNT           (VVS_SRC_PARAMETERS + 2)
#define VVS_SRC_FILENAME        (VVS_SRC_PARAMETERS + 3)
#define VVS_SRC_COMM_NAME       (VVS_SRC_PARAMETERS + 4)
#define VVS_SRC_COMM_SETUP      (VVS_SRC_PARAMETERS + 5)
#define VVS_SRC_OUT_Q_CNT       (VVS_SRC_PARAMETERS + 6)
#define VVS_SRC_RET_Q_CNT       (VVS_SRC_PARAMETERS + 7)
#define VVS_SRC_MSG_NUM         (VVS_SRC_PARAMETERS + 8)
#define VVS_SRC_FLAGS           (VVS_SRC_PARAMETERS + 9)

/* Left 15 parameter spaces for Source stream */

/* Sink stream parameters */
#define VVS_SINK_PARAMETERS     (VVS_PARAM_BASE + 110)
#define VVS_SINK_PATTERN_SIZE   (VVS_SINK_PARAMETERS + 0)
#define VVS_SINK_PATTERN        (VVS_SINK_PARAMETERS + 1)
#define VVS_SINK_GOOD_COUNT     (VVS_SINK_PARAMETERS + 2)
#define VVS_SINK_BAD_COUNT      (VVS_SINK_PARAMETERS + 3)
#define VVS_SINK_FILENAME       (VVS_SINK_PARAMETERS + 4)
#define VVS_SINK_COMM_NAME      (VVS_SINK_PARAMETERS + 5)
#define VVS_SINK_COMM_SETUP     (VVS_SINK_PARAMETERS + 6)
#define VVS_SINK_MSG_NUM        (VVS_SINK_PARAMETERS + 7)
#define VVS_SINK_FLAGS          (VVS_SINK_PARAMETERS + 8)

/* Left 15 parameter spaces for Sink stream */


/* Parameters for CODER and ENCODER */
#define VVS_CODER_PARAMETERS    (VVS_PARAM_BASE + 125)

/* encoder min quantizer value (tuning parameter) */
#define VVS_QUANTIZER_MIN       (VVS_CODER_PARAMETERS + 0)

/* Coder streams misc */
#define VVS_BITRATEK            (VVS_CODER_PARAMETERS + 1)
#define VVS_MAX_BITS            (VVS_CODER_PARAMETERS + 2)
#define VVS_TARGET_BITS         (VVS_CODER_PARAMETERS + 3)
#define VVS_EFF_FACTOR          (VVS_CODER_PARAMETERS + 4)

/* BitRate control*/
#define VVS_RESERVE_BITS        (VVS_CODER_PARAMETERS + 5)
#define VVS_QUANT_BITS          (VVS_CODER_PARAMETERS + 6)
#define VVS_CAPTURE_BITS        (VVS_CODER_PARAMETERS + 7)
#define VVS_BOTTOM_BITS         (VVS_CODER_PARAMETERS + 8)
#define VVS_BUFFER_BITS         (VVS_CODER_PARAMETERS + 9)
#define VVS_QTARGET_BITS        (VVS_CODER_PARAMETERS + 10)

/* for Motion Estimation tuning maps */
#define VVS_MEST_MAP            (VVS_CODER_PARAMETERS + 11)
 
/* far-end resolution capabilities */
#define VVS_LOW_RES_CAP         (VVS_CODER_PARAMETERS + 12)
#define VVS_MED_RES_CAP         (VVS_CODER_PARAMETERS + 13) 
#define VVS_HIGH_RES_CAP        (VVS_CODER_PARAMETERS + 14)

/* Controls for MahKeeNak DCT */ 
#define VVS_K3                  (VVS_CODER_PARAMETERS + 15)
#define VVS_KALL                (VVS_CODER_PARAMETERS + 16)
#define VVS_MIN3                (VVS_CODER_PARAMETERS + 17)
#define VVS_MINALL              (VVS_CODER_PARAMETERS + 18)
 
/* Left a total of 40 spaces for coder and decoder parameters */
#define VVS_DISPLAY_FREEZE      (VVS_CODER_PARAMETERS + 19)
#define VVS_DISPLAY_DIB         (VVS_CODER_PARAMETERS + 20)
#define VVS_WGHT1               (VVS_CODER_PARAMETERS + 21)
#define VVS_WGHT2               (VVS_CODER_PARAMETERS + 22)
#define VVS_FRAMESDISPLAYED     (VVS_CODER_PARAMETERS + 23)
#define VVS_ENCODE_BAL_DISPLAY  (VVS_CODER_PARAMETERS + 24)
#define VVS_VIDEO_HOLD          (VVS_CODER_PARAMETERS + 25)
#define VVS_FORMAT              (VVS_CODER_PARAMETERS + 26)
#define VVS_GAMMA_PREDISTORTED  (VVS_CODER_PARAMETERS + 27)
#define VVS_UPSIDE_DOWN         (VVS_CODER_PARAMETERS + 28)
#define VVS_MIRROR              (VVS_CODER_PARAMETERS + 29)
#define VVS_LIGHTING            (VVS_CODER_PARAMETERS + 30)
#define VVS_INTENSITY_PERCENTILE (VVS_CODER_PARAMETERS + 31)
#define VVS_DECODER_BCH         (VVS_CODER_PARAMETERS + 32)
#define VVS_OUTDIB_FORMAT       (VVS_CODER_PARAMETERS + 33)
#define VVS_WHITE_BALANCE       (VVS_CODER_PARAMETERS + 34)
#define VVS_SOURCE_DIALOG       (VVS_CODER_PARAMETERS + 35)
#define VVS_FORMAT_DIALOG       (VVS_CODER_PARAMETERS + 36)     
#define VVS_DISPLAY_DIALOG      (VVS_CODER_PARAMETERS + 37) 
#define VVS_CAPTURE_WINDOW    (VVS_CODER_PARAMETERS + 38)
#define VVS_CAPTURE_SCALE     (VVS_CODER_PARAMETERS + 39)

/* Smart Screen Source: 0 = file, 1 = screen, 2 = pattern */
#define VVS_SMTSCR_PARAMETERS   (VVS_PARAM_BASE + 165)
#define VVS_SMTSCR_SRC          (VVS_SMTSCR_PARAMETERS + 0)
#define VVS_SMTSCR_DIB          (VVS_SMTSCR_PARAMETERS + 1)
#define VVS_SMTSCR_HWND         (VVS_SMTSCR_PARAMETERS + 2)
#define VVS_SMTSCR_GETDIB       (VVS_SMTSCR_PARAMETERS + 3)
#define VVS_SMTSCR_USAGE        (VVS_SMTSCR_PARAMETERS + 4)  
#define VVS_SMTSCR_CAPTURE_DIB_INTERVAL   (VVS_SMTSCR_PARAMETERS + 5)  
#define VVS_SMTSCR_CAPTURE_CURSOR_INTERVAL   (VVS_SMTSCR_PARAMETERS + 6)   

/* Steal some SmartScreen prarmeters for H.263 - md 2/24/96 */
#define VVS_ADVANCED_PRED		(VVS_SMTSCR_PARAMETERS + 7)
#define VVS_URESTRICTED_MV		(VVS_SMTSCR_PARAMETERS + 8)
#define VVS_SEND_GOB_HEADERS    (VVS_SMTSCR_PARAMETERS + 9)

// Echo Suppression Parameters
#define VVS_ECHO_PARAMETERS     (VVS_PARAM_BASE + 175)
//Overlap G723 w/ echo.  G723 used for audout, echo used for audin.

#define VVS_G723_OK             (VVS_ECHO_PARAMETERS + 0)
#define VVS_G723_CONCEAL        (VVS_ECHO_PARAMETERS + 1)
#define VVS_G723_BAD_BITSTREAM  (VVS_ECHO_PARAMETERS + 2)                   
#define VVS_G723_PACKBITS_ERROR (VVS_ECHO_PARAMETERS + 3)                
#define VVS_G723_UNSUPPORTED_MODE (VVS_ECHO_PARAMETERS + 4)
 
#define VVS_FAST_INC            (VVS_ECHO_PARAMETERS + 0)
#define VVS_SLOW_INC            (VVS_ECHO_PARAMETERS + 1)
#define VVS_LEVEL_INC           (VVS_ECHO_PARAMETERS + 2)
#define VVS_NOISE_INC           (VVS_ECHO_PARAMETERS + 3)
#define VVS_THRESHOLD           (VVS_ECHO_PARAMETERS + 4)
#define VVS_ECHO_VOLUME         (VVS_ECHO_PARAMETERS + 5)
#define VVS_ECHO_GAIN           (VVS_ECHO_PARAMETERS + 6)
#define VVS_ECHO_RANGE          (VVS_ECHO_PARAMETERS + 7)

/* echo suppression--audin stream only!  */
#define VVS_ECHO_INPUT_GAIN     (VVS_ECHO_PARAMETERS + 8)
#define VVS_ECHO_LINE_GAIN      (VVS_ECHO_PARAMETERS + 9)
#define VVS_ECHO_MIC_BIAS       (VVS_ECHO_PARAMETERS + 10)
#define VVS_ECHO_XMIT_LEVEL     (VVS_ECHO_PARAMETERS + 11)
#define VVS_ECHO_SPEAKER_LEVEL  (VVS_ECHO_PARAMETERS + 12)

/* more echo suppression stuff  */
#define VVS_ECHO_CENTER_CLIPPER (VVS_ECHO_PARAMETERS + 13)
#define VVS_ECHO_TIME           (VVS_ECHO_PARAMETERS + 14)
#define VVS_MWAVE_TIMER_FUZZ    (VVS_ECHO_PARAMETERS + 15)
#define VVS_ECHO_ATTENUATION    (VVS_ECHO_PARAMETERS + 16)
#define VVS_ECHO_ON             (VVS_ECHO_PARAMETERS + 17)
#define VVS_MWAVE_TIMER         (VVS_ECHO_PARAMETERS + 18)
#define VVS_ECHO_LINE_BIAS      (VVS_ECHO_PARAMETERS + 19)
#define VVS_ECHO_OUTPUT_NOISE   (VVS_ECHO_PARAMETERS + 20)
#define VVS_ECHO_INPUT_NOISE    (VVS_ECHO_PARAMETERS + 21)
#define VVS_ECHO_OUTPUT_LEVEL   (VVS_ECHO_PARAMETERS + 22)
#define VVS_ECHO_INPUT_LEVEL    (VVS_ECHO_PARAMETERS + 23)
//#define VVS_ECHO_SYNC_ERRORS    (VVS_ECHO_PARAMETERS + 24)
#define VVS_VA_SLEEP_COUNT          (VVS_ECHO_PARAMETERS + 24)
//#define VVS_VA_MAG_PRED_COEFFS        (VVS_ECHO_PARAMETERS + 25)
#define VVS_VA_FALL_TIME        (VVS_ECHO_PARAMETERS + 25)
#define VVS_VA_MAG_SPEECH           (VVS_ECHO_PARAMETERS + 26)
//#define VVS_VA_SLEEP_COEFF_THRESH (VVS_ECHO_PARAMETERS + 27)
#define VVS_VA_RISE_TIME            (VVS_ECHO_PARAMETERS + 27)
#define VVS_VA_SLEEP_SPEECH_THRESH  (VVS_ECHO_PARAMETERS + 28)
#define VVS_VA_ACTIVE_SPEECH_THRESH (VVS_ECHO_PARAMETERS + 29)
#define VVS_VA_STATE                (VVS_ECHO_PARAMETERS + 30)
//#define VVS_VA_MAG_PRED_POLES     (VVS_ECHO_PARAMETERS + 30)
//#define VVS_VA_MAG_PRED_ZEROS     (VVS_ECHO_PARAMETERS + 31)
/* OK to go over 30 since overlap is w/ ses manager and */
/* will never ask any other stream for VVS_VA_MAG_PRED_ZEROS*/ 
/* Left 30 parameter spaces for echo suppression */

/* Session Manager Stream Parameters */
#define VVS_SESMNGR_PARAMETERS      (VVS_PARAM_BASE + 205)
#define VVS_SES_MNGR_COMM_MNGR      (VVS_SESMNGR_PARAMETERS + 0)
#define VVS_SES_MNGR_NEXT_ACTION    (VVS_SESMNGR_PARAMETERS + 1)
#define VVS_SES_MNGR_CURR_STATE     (VVS_SESMNGR_PARAMETERS + 2)
#define VVS_SES_MNGR_STREAMS        (VVS_SESMNGR_PARAMETERS + 3)
#define VVS_SES_MNGR_STREAMS_MAX    (VVS_SESMNGR_PARAMETERS + 4)
#define VVS_SES_MNGR_STREAMS_MIN    (VVS_SESMNGR_PARAMETERS + 5)
#define VVS_SES_MNGR_DMINITED       (VVS_SESMNGR_PARAMETERS + 6)
#define VVS_SES_MNGR_COMPRSRC       (VVS_SESMNGR_PARAMETERS + 7)
#define VVS_SES_MNGR_LOOPBACK_LEVEL (VVS_SESMNGR_PARAMETERS + 8)
#define VVS_SES_MNGR_SET_CUR_DIR    (VVS_SESMNGR_PARAMETERS + 9)
#define VVS_SES_MNGR_INSTANCE       (VVS_SESMNGR_PARAMETERS + 10)
#define VVS_SES_MNGR_CODEC_LOOP     (VVS_SESMNGR_PARAMETERS + 11)
#define VVS_SES_MNGR_VIDEO_LOOP     (VVS_SESMNGR_PARAMETERS + 12)
#define VVS_TOTAL_ALLOCS			(VVS_SESMNGR_PARAMETERS + 13)
#define VVS_TOTAL_LOCKED_ALLOCS     (VVS_SESMNGR_PARAMETERS + 14)

/* Left 15 parameter spaces for Session Manager */


/* Communication Manager Stream Parameters */
#define VVS_COMMMNGR_PARAMETERS      (VVS_PARAM_BASE + 220)
#define VVS_COMM_MNGR_CALL_STATE     (VVS_COMMMNGR_PARAMETERS + 0)
#define VVS_COMM_MNGR_CALL_STATE_REQ (VVS_COMMMNGR_PARAMETERS + 1)
#define VVS_COMM_MNGR_LOCAL_ADDR     (VVS_COMMMNGR_PARAMETERS + 2)
#define VVS_COMM_MNGR_REMOTE_ADDR    (VVS_COMMMNGR_PARAMETERS + 3)
#define VVS_COMM_MNGR_COMM_BW_REQ    (VVS_COMMMNGR_PARAMETERS + 4)
#define VVS_COMM_MNGR_REMOTE_SYSTEM  (VVS_COMMMNGR_PARAMETERS + 5)
#define VVS_COMM_MNGR_ISDN_DEBUG_56  (VVS_COMMMNGR_PARAMETERS + 6)
#define VVS_COMM_MNGR_LCL_AUD_CAP    (VVS_COMMMNGR_PARAMETERS + 7)
#define VVS_COMM_MNGR_LCL_DATA_CAP   (VVS_COMMMNGR_PARAMETERS + 8)
#define VVS_COMM_MNGR_LCL_TRANS_CAP  (VVS_COMMMNGR_PARAMETERS + 9)
#define VVS_COMM_MNGR_LCL_VID_CAP    (VVS_COMMMNGR_PARAMETERS + 10)
#define VVS_COMM_MNGR_LCL_H230_CMD   (VVS_COMMMNGR_PARAMETERS + 11)
#define VVS_COMM_MNGR_LCL_CMD        (VVS_COMM_MNGR_LCL_H230_CMD)
#define VVS_COMM_MNGR_REM_AUD_CAP    (VVS_COMMMNGR_PARAMETERS + 12)
#define VVS_COMM_MNGR_REM_DATA_CAP   (VVS_COMMMNGR_PARAMETERS + 13)
#define VVS_COMM_MNGR_REM_TRANS_CAP  (VVS_COMMMNGR_PARAMETERS + 14)
#define VVS_COMM_MNGR_REM_VID_CAP    (VVS_COMMMNGR_PARAMETERS + 15)
#define VVS_COMM_MNGR_REM_H230_CMD   (VVS_COMMMNGR_PARAMETERS + 16)
#define VVS_COMM_MNGR_REM_CMD        (VVS_COMM_MNGR_REM_H230_CMD)
#define VVS_COMM_MNGR_RX_MODE_REQ    (VVS_COMMMNGR_PARAMETERS + 17)
#define VVS_COMM_MNGR_RX_MODE        (VVS_COMMMNGR_PARAMETERS + 18)
#define VVS_COMM_MNGR_TX_MODE_REQ    (VVS_COMMMNGR_PARAMETERS + 19)
#define VVS_COMM_MNGR_TX_MODE        (VVS_COMMMNGR_PARAMETERS + 20)
#define VVS_COMM_MNGR_LCL_NONSTD_CMD (VVS_COMMMNGR_PARAMETERS + 21)
#define VVS_COMM_MNGR_REM_NONSTD_CMD (VVS_COMMMNGR_PARAMETERS + 22)
#define VVS_COMM_MNGR_CAUSE_CODE     (VVS_COMMMNGR_PARAMETERS + 23)
#define VVS_COMM_MNGR_INCOMING_CALL  (VVS_COMMMNGR_PARAMETERS + 24)
#define VVS_COMM_MNGR_LINE_STATUS    (VVS_COMMMNGR_PARAMETERS + 25)
#define VVS_COMM_MNGR_SPID_1         (VVS_COMMMNGR_PARAMETERS + 26)
#define VVS_COMM_MNGR_SPID_2         (VVS_COMMMNGR_PARAMETERS + 27)
#define VVS_COMM_MNGR_REASON_MSG     (VVS_COMMMNGR_PARAMETERS + 28)
#define VVS_COMM_MNGR_VERSION_ID     (VVS_COMMMNGR_PARAMETERS + 29)
#define VVS_COMM_MNGR_LCL_MLP_CAP    (VVS_COMMMNGR_PARAMETERS + 30)
#define VVS_COMM_MNGR_LCL_HMLP_CAP   (VVS_COMMMNGR_PARAMETERS + 31)
#define VVS_COMM_MNGR_LCL_DATAPP_CAP (VVS_COMMMNGR_PARAMETERS + 32)
#define VVS_COMM_MNGR_REM_MLP_CAP    (VVS_COMMMNGR_PARAMETERS + 33)
#define VVS_COMM_MNGR_REM_HMLP_CAP   (VVS_COMMMNGR_PARAMETERS + 34)
#define VVS_COMM_MNGR_REM_DATAPP_CAP (VVS_COMMMNGR_PARAMETERS + 35)
#define VVS_COMM_MNGR_REMOTE_COUNTRY (VVS_COMMMNGR_PARAMETERS + 36)
#define VVS_COMM_MNGR_INQUEUE        (VVS_COMMMNGR_PARAMETERS + 37)
#define VVS_COMM_MNGR_OUTQUEUE       (VVS_COMMMNGR_PARAMETERS + 38)
#define VVS_COMM_MNGR_MODEM_OVERRUN  (VVS_COMMMNGR_PARAMETERS + 39)
#define VVS_COMM_MNGR_MODEM_CTSTO    (VVS_COMMMNGR_PARAMETERS + 40)
#define VVS_COMM_MNGR_MODEM_OTHER    (VVS_COMMMNGR_PARAMETERS + 41)
#define VVS_COMM_MNGR_SOFT_LOOP      (VVS_COMMMNGR_PARAMETERS + 42)
#define VVS_COMM_MNGR_HARD_LOOP      (VVS_COMMMNGR_PARAMETERS + 43)
#define VVS_COMM_MNGR_DIALOG_STRING  (VVS_COMMMNGR_PARAMETERS + 44)
#define VVS_MODEM_DIALOG_STRING      VVS_COMM_MNGR_DIALOG_STRING 
#define VVS_COMM_MNGR_DIALOG_STRING  (VVS_COMMMNGR_PARAMETERS + 44)
#define VVS_COMM_MNGR_BITCOUNT2      (VVS_COMMMNGR_PARAMETERS + 45)
#define VVS_COMM_MNGR_PRECONFIG_LINE (VVS_COMMMNGR_PARAMETERS + 46)
#define VVS_COMM_MNGR_GET_LINE_NAME  (VVS_COMMMNGR_PARAMETERS + 47)
#define VVS_COMM_MNGR_GET_LINE_ICON  (VVS_COMMMNGR_PARAMETERS + 48)
#define VVS_COMM_MNGR_UNDERRUNS      (VVS_COMMMNGR_PARAMETERS + 49)
#define VVS_COMM_MNGR_TXPOLLS        (VVS_COMMMNGR_PARAMETERS + 50)
#define VVS_COMM_MNGR_TAPI_HANDLE    (VVS_COMMMNGR_PARAMETERS + 51)
#define VVS_COMM_MNGR_MODE      (VVS_COMMMNGR_PARAMETERS + 52)
 
/* Left 55 parameter spaces for Communication Manager */

/* H.245 Protocol Manager Stream Parameters - mapped on top of Comm Mgr Params */
#define VVS_STATE_CHANGE_REQ		 VVS_COMM_MNGR_CALL_STATE_REQ
#define VVS_H245_PROTOCOL_STATE      (VVS_STATE_CHANGE_REQ+1)
#define VVS_H245_PROTOCOL_FAILED     (VVS_STATE_CHANGE_REQ+2)
#define VVS_H245_LAST_ERROR          (VVS_STATE_CHANGE_REQ+3)
#define VVS_SET_STRING_RECV_BUFFER	 (VVS_STATE_CHANGE_REQ + 4)
#define VVS_SEND_STRING				 (VVS_STATE_CHANGE_REQ + 5)
#define VVS_H245_PROTOCOL_LOOP_STATE (VVS_STATE_CHANGE_REQ + 6)
#define VVS_GET_FILE_TRANSFER		 (VVS_STATE_CHANGE_REQ + 7)
#define VVS_PERFORM_FILE_TRANSFER	 (VVS_STATE_CHANGE_REQ + 8)
 


/* H.DLL MUX/DEMUX stream parameters */
#define VVS_HDLL_PARAMETERS          (VVS_PARAM_BASE + 275)
#define VVS_HDLLMUX_BITRATE_REQ      (VVS_HDLL_PARAMETERS + 0)
#define VVS_HDLLMUX_BLOCKBYTES       (VVS_HDLL_PARAMETERS + 1)
#define VVS_HDLLMUX_FRAMEBYTES       (VVS_HDLL_PARAMETERS + 2)
#define VVS_HDLLMUX_ERRORS           (VVS_HDLL_PARAMETERS + 3)

#define VVS_HDLLDEMUX_BITRATE_REQ    (VVS_HDLL_PARAMETERS + 4)
#define VVS_HDLLDEMUX_BLOCKBYTES     (VVS_HDLL_PARAMETERS + 5)
#define VVS_HDLLDEMUX_FRAMEBYTES     (VVS_HDLL_PARAMETERS + 6)
#define VVS_HDLLDEMUX_ERRORS         (VVS_HDLL_PARAMETERS + 7)

#define VVS_HDLL_LAST_ERROR          (VVS_HDLL_PARAMETERS + 8)
#define VVS_HDLL_COMPLEMENT_STREAM   (VVS_HDLL_PARAMETERS + 9)

/* Left 15 parameter spaces for H.DLL MUX/DEMUX */

/* REMOTE DISPLAY STREAM PARAMETERS */
#define VVS_REMOTE_STREAM_PARAMETERS (VVS_PARAM_BASE + 290)
#define VVS_REMOTE_STREAM_DIB_TIMECOUNT         (VVS_REMOTE_STREAM_PARAMETERS + 0)
#define VVS_REMOTE_STREAM_DIB_TIMEDRAW          (VVS_REMOTE_STREAM_PARAMETERS + 1)
#define VVS_REMOTE_STREAM_DIB_TIMEDECOMPRESS    (VVS_REMOTE_STREAM_PARAMETERS + 2) 
#define VVS_REMOTE_STREAM_DIB_TIMEDITHER        (VVS_REMOTE_STREAM_PARAMETERS + 3)     
#define VVS_REMOTE_STREAM_DIB_TIMESTRETCH       (VVS_REMOTE_STREAM_PARAMETERS + 4)
#define VVS_REMOTE_STREAM_DIB_TIMEBLT           (VVS_REMOTE_STREAM_PARAMETERS + 5)    
#define VVS_REMOTE_STREAM_DIB_TIMESETDIBITS     (VVS_REMOTE_STREAM_PARAMETERS + 6)   

/* Left 15 parameter spaces for REMOTE DISPLAY */                
                      
/* REMOTE DISPLAY STREAM PARAMETERS */
#define VVS_NET_LAYER_PARAMETERS (VVS_PARAM_BASE + 305)
#define VVS_NET_LAYER_STATE                     (VVS_NET_LAYER_PARAMETERS + 0)

/* Left 15 parameter spaces for NET_LAYER */                
                      
/* Record & Playback */
#define VVS_RECORD_PARAMETERS   (VVS_PARAM_BASE + 320)
#define VVS_AUD_FILENAME        (VVS_RECORD_PARAMETERS + 0)
#define VVS_VID_FILENAME        (VVS_RECORD_PARAMETERS + 1)
#define VVS_VID_OUTPUTSTREAM    (VVS_RECORD_PARAMETERS + 2)
#define VVS_VID_INPUTSTREAM     (VVS_RECORD_PARAMETERS + 3)
#define VVS_DECODER_BITSLEFT    (VVS_RECORD_PARAMETERS + 4)

/* 10 spaces for RECORD */                

/* OS/2 display stream window setup/switching */
#define VVS_OS2_DISPLAY_PARAMETERS (VVS_PARAM_BASE + 330)

/* v passed in is new display HWND, v==0 turns display into sink */
#define VVS_OS2_DISPLAY_CLIENT_WND (VVS_OS2_DISPLAY_PARAMETERS + 0)
/* v is window to receive DIVE messages */
#define VVS_OS2_DISPLAY_FRAME_WND  (VVS_OS2_DISPLAY_PARAMETERS + 1)
/* 2 spaces used for OS/2 display parameters */

/* 18 spaces for H.223 Adaptation Layer (AL) parameters */
#define VVS_AL_PARAMETERS           (VVS_PARAM_BASE     + 332)

#define VVS_AL_HEADER_BYTES         (VVS_AL_PARAMETERS + 0) 
#define VVS_AL_PERFORM_RETRANSMIT   (VVS_AL_PARAMETERS + 1)
#define VVS_AL_REQUEST_RETRANSMIT   (VVS_AL_PARAMETERS + 2)
#define VVS_AL_TOTAL_RCV_I_PDU      (VVS_AL_PARAMETERS + 3)
#define VVS_AL_TOTAL_RCV_SREJ       (VVS_AL_PARAMETERS + 4)
#define VVS_AL_TOTAL_RCV_DRTX       (VVS_AL_PARAMETERS + 5)
#define VVS_AL_TOTAL_XMIT_SREJ      (VVS_AL_PARAMETERS + 6)
#define VVS_AL_TOTAL_XMIT_DRTX      (VVS_AL_PARAMETERS + 7)
#define VVS_AL_CURRENT_DELAY_BITS   (VVS_AL_PARAMETERS + 8)
#define VVS_AL_CURRENT_HELD_FRAMES  (VVS_AL_PARAMETERS + 9)
#define VVS_AL_MAX_RETRY_FRAMES     (VVS_AL_PARAMETERS + 10)
#define VVS_AL_TOTAL_XMIT_PDU       (VVS_AL_PARAMETERS + 11)
#define VVS_AL_AVE_TIME_SINCE_CAP   (VVS_AL_PARAMETERS + 12)
#define VVS_AL_MAX_TIME_SINCE_CAP   (VVS_AL_PARAMETERS + 13)
#define VVS_AL_MIN_TIME_SINCE_CAP   (VVS_AL_PARAMETERS + 14)
#define VVS_AL_AVE_ROUND_TRIP       (VVS_AL_PARAMETERS + 15)
#define VVS_AL_MAX_ROUND_TRIP       (VVS_AL_PARAMETERS + 16)
#define VVS_AL_MIN_ROUND_TRIP       (VVS_AL_PARAMETERS + 17)

#define VVS_NEXT_PARAMETER_BLOCK (VVS_PARAM_BASE + 350)

/* Parameter defn's for AL2 stream - note overlap with RECORD 
 * These belong in VSCODES as soon as they can be added there
 */
#define VVS_AL2_PARAMETERS            (VVS_PARAM_BASE + 325)
#define VVS_AL2_USE_SEQ_NUM         (VVS_AL2_PARAMETERS + 0)
#define VVS_AL2_TOTAL_RECV_PDU      (VVS_AL2_PARAMETERS + 1)
#define VVS_AL2_TOTAL_XMIT_PDU      (VVS_AL2_PARAMETERS + 2)
#define VVS_AL2_BAD_CRC_RECV        (VVS_AL2_PARAMETERS + 3)
#define VVS_AL2_RECV_PEND_PDU       (VVS_AL2_PARAMETERS + 4)

/***** NO MORE THAN 368 PARAMETERS (VVS_CODE_BASE + 368) *****/
/***** (VVS_BASE + 0x1D0) - (VVS_BASE + 0x60) = 0x170 = 368 ****/

/* Stream States */
#define VVS_STATE_BASE         VVS_BASE + 0x1D0
#define VVS_NULL               (VVS_STATE_BASE + 1)
#define VVS_UNINITIALIZED      (VVS_STATE_BASE + 2)
#define VVS_UNPREPARED         (VVS_STATE_BASE + 3)
#define VVS_UNPREPARED_WAITING (VVS_STATE_BASE + 4)
#define VVS_IDLE               (VVS_STATE_BASE + 5)
#define VVS_ACTIVE             (VVS_STATE_BASE + 6)   

/* messages for dynamic message allocator */
#define VVS_MESSAGE_BASE     VVS_BASE + 0x1E0
#define VVS_NO_MESSAGE       VVS_MESSAGE_BASE  
#define VVS_WRDCH_MSG        (VVS_MESSAGE_BASE + 1)
// GAM062595 - Add hooks for WinISDN messages
#define VVS_WinISDN_MSG      (VVS_MESSAGE_BASE + 2)
#define VVS_MESSAGE_COUNT   0x1F


/* IDs for messages from Virtual COMM Port VxDs 
 * (VVSERIAL.386 and VBUFF.386)
 */
#define WM_POSTED_BY_VFAST_VXD    (WM_USER + 1997)
#define WM_POSTED_BY_VVCOMM       (WM_USER + 1998)

/* H.223 MUX messages */
#define VVS_H223MUX_MSG_BASE        (VVS_BASE + 0x280)
#define VVS_H223MUX_AL1X_STREAM     (VVS_H223MUX_MSG_BASE + 1)
#define VVS_H223MUX_AL1R_STREAM     (VVS_H223MUX_MSG_BASE + 2)

#define VVS_H223MUX_AL2X_STREAM     (VVS_H223MUX_MSG_BASE + 3)
#define VVS_H223MUX_AL2R_STREAM     (VVS_H223MUX_MSG_BASE + 4)

#define VVS_H223MUX_AL3X_STREAM     (VVS_H223MUX_MSG_BASE + 5)
#define VVS_H223MUX_AL3R_STREAM     (VVS_H223MUX_MSG_BASE + 6)

#define VVS_H223MUX_H245X_STREAM    (VVS_H223MUX_MSG_BASE + 7)
#define VVS_H223MUX_H245R_STREAM    (VVS_H223MUX_MSG_BASE + 8)

#define VVS_H223MUX_START           (VVS_H223MUX_MSG_BASE + 9)

#define VVS_H223_PDU_SIZE           (VVS_H223MUX_MSG_BASE +10)

/* Left 15 spaces for H223 MUX parameters */

/* Use these for parameters which are PRIVATE to a particular	*/
#define VVS_PRIV32_MSG_BASE			(VVS_BASE + 0x290)

#define VVS_PRIV32_MSG_END			(VVS_PRIV32_MSG_BASE + 100)

/* Left 100 spaces for Private parameters	*/

/* Audio Encode/Decode Stream Parameters	*/
#define VVS_AUD_MSG_BASE		(VVS_BASE + 0x2F5)
#define VVS_AUDIO_CODING_MODE	(VVS_PRIV32_MSG_BASE + 0)
#define VVS_MPMLQ_COUNT			(VVS_PRIV32_MSG_BASE + 1)
#define VVS_ACELP_COUNT			(VVS_PRIV32_MSG_BASE + 2)
#define VVS_SID_COUNT			(VVS_PRIV32_MSG_BASE + 3)
#define VVS_DELAY_JITTER		(VVS_PRIV32_MSG_BASE + 4)
#define VVS_INPUT_UNDERRUN		(VVS_AUD_MSG_BASE + 0)
#define VVS_OUTPUT_OVERRUN		(VVS_AUD_MSG_BASE + 1)

/* left 49 locs for audio parameters	*/
/* Audio Echo Cancellation	*/
#define VVS_AUD_ENC_MSG_BASE		(VVS_BASE + 0x327)

//Some Private parameters for the video coder and decoder.
#define VVS_AVE_FRAME_INTERVAL		(VVS_PRIV32_MSG_BASE + 0)

//Some Private parameters for the Timer Stream.
#define VVS_OUTPUT_STREAM2		(VVS_PRIV32_MSG_BASE + 0)
#define VVS_OUTPUT_STREAM3		(VVS_PRIV32_MSG_BASE + 1)
#define VVS_OUTPUT_STREAM4		(VVS_PRIV32_MSG_BASE + 2)
#define VVS_OUTPUT_STREAM5		(VVS_PRIV32_MSG_BASE + 3)
#define VVS_OUTPUT_STREAM6		(VVS_PRIV32_MSG_BASE + 4)
#define VVS_AVE_PERIOD			(VVS_PRIV32_MSG_BASE + 5)

//Some Private parameters for the ATT Audio I/O Stream.
#define VVS_AUDDRV_FRAMES_CAPTURED		(VVS_PRIV32_MSG_BASE + 0)
#define VVS_AUDDRV_FRAMES_PLAYED		(VVS_PRIV32_MSG_BASE + 1)
#define VVS_AUDDRV_BUFSIZE				(VVS_PRIV32_MSG_BASE + 2)
#define VVS_AUDDRV_DMASIZE				(VVS_PRIV32_MSG_BASE + 3)
#define VVS_AUDDRV_OUT_FLUFF			(VVS_PRIV32_MSG_BASE + 4)
#define VVS_AUDDRV_OUT_GET				(VVS_PRIV32_MSG_BASE + 5)
#define VVS_AUDDRV_OUT_PUT				(VVS_PRIV32_MSG_BASE + 6)
#define VVS_AUDDRV_IN_GET				(VVS_PRIV32_MSG_BASE + 7)
#define VVS_AUDDRV_IN_PUT				(VVS_PRIV32_MSG_BASE + 8)			

#endif 
