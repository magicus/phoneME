/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/*---------------------------------------------------------------------------*
   File:    gsm_api.h

   Classification:  IBM confidential


   Copyright:       (C) Copyright IBM Corp.
                    Haifa Research Laboratory
                    Audio/Video Group.

   Contact:         Gilad Cohen
                    e-mail: giladc@haifa.vnet.ibm.com
                    LN: Gilad Cohen/IBMHAIFA@IBMHAIFA

   Date:            November 1998

   Version:         3.1

   Description:     definition of the ANSI-C floating point GSM API
                    and general help file.

   Remarks:         IMPORTANT !! For those of you who intend to use
                    Microsoft Visual C++ Ver 5.x, make sure the latest
		    Visual service pack is installed (ver 3+). Otherwise,
		    it won't work. There is a bug in their optimizer,
		    which may result in a run time crash. It seems the
		    service packed solved the problem. Just in case,
		    I strongly recommend using version 4.x instead. I have
                    not yet tested this software under version 6.x

   History:

   Additions made in version 3.1 (11/11/98) Shay Ben-David
   * added support for MS GSM bit packing   (encoder)
   * added support for MS GSM bit unpacking (decoder)



   Additions made in version 3.0 (27/10/98)
   * Old bitstream packing methods and macros were removed.
   * 4 bit parity code was removed.
   * New RTP (SUN) hard coded packing was inserted
   * Optional HotMedia packing was inserted.
   * Encoder bug fix in second residual decimation (thanks to Shay Ben-David !)
   * Cleanup: deleting unused variables, tables & structures

   Additions made in version 2.3 (12/7/98)
   * An epsilon value was added to all samples ofter the Preprocess
   function in the encoder. Before, when encoding zeros, it was 10
   times slower. The "EPSILON" was already defined before.


   Additions made in version 2.2 (23/6/98)
   * API for enabling/disabling DTX/VAD was inserted in the encoder


   Additions made in version 2.1 (24/11/97)
   * Bitstream packing in "GSM_SIMPLE_BIT_PACKING" mode was changed. It was
     found out the the MWAVE writes it's words in AIX format (big endian), so
	 to be interoperable, the first 32 bytes where swaped. The last byte
	 remaind the same. The change was done only to the two bitstream packing
	 tables (encoder+decoder), and not to the code itself.

   Additions made in version 2.0 (12/10/97)
   * The bitstream packing has changed to 33 'unsigned char' bytes.
   * GSM_BTS_SWAP_BYTES was therefore canceled.
   * All warnings were removed.
   * File-to-file executables which used the DLL where generated and the DLL
     tested.

   Additions made in version 1.2 (9/10/97)
   * Initialization of lastSID is the decoder. If was previously set to zero.

   Additions made in version 1.1:
   * The "gsm_decoder_close" and "gsm_encoder_close" APIs have been added.
   * Two functions had the same name in the encoder and decoder (DLL
     compilation problem), so their names have been changed.
   * GSMFrameType returned while encoding.
   * Frame type counting and printout in the encoder file-to-file main.
   * SID Truncated mode.
   * Forcing generation of comfort noise in the decoder.
   * Additional itegration information has been added to the gsm_api.h file.



*---------------------------------------------------------------------------*/

/*

 General Description
 -------------------

 ANSI-C floating point GSM codec. Compresses a frame of 160 samples (20 msec) into
 260 bits (without the four control bits), which gives a rate of 13K bits per second.
 With the four contorl bits, each frame is compressed into 264 bits (==33 bytes), which
 gives a bitrate of 13.2 kbs.

 See the main functions in the "gsm_enc_main.c" and "gsm_dec_main.c" for an
 example of realtime API usage.

 Makefiles should be generated according to the following file assignment:


  1. Library compilation (encoder+decoder DLL) should include the following:

  gsm_enc.c
  gsm_dec.c

  Dependencies:

  gsm_api.h
  gsm_dec.h
  gsm_enc.h
  gsm_mac.h

  Remember to undefine the "NOT_A_WIN_DLL" during the compilation.

  2. Encoder file-to-file executable compilation should include:

  gsm_enc.c
  gsm_enc_main.c (API usage example)

  Dependencies:

  gsm_api.h
  gsm_mac.h
  gsm_enc.h


  3. Decoder file-to-file executables compilation should include:


  gsm_dec.c
  gsm_dec_main.c (API usage example)

  Dependencies:

  gsm_api.h
  gsm_mac.h
  gsm_dec.h


 Microsoft Visual C++ Makefiles
 ------------------------------

  The following MSVC++ Ver. 4.2 makefiles are supplied:

  gsmenc.mak - File-to-file encoder (not using any DLLs).

  gsmdec.mak - File-to-file decoder (not using any DLLs).

  gsmdll.mak - Building an encoder plus decoded DLL.

  gsmdllenc.mak - File-to-file encoder, which uses the DLL.

  gsmdlldec.mak - File-to-file decoder, which uses the DLL.

*/


/* Definition for C++ integration */

#ifdef __cplusplus
  extern "C" {
#endif


/*
 DLL compilation
 ---------------

 Each API function definition is preceded with the APICALL string.
 In case the source files should be compiled with other source files,
 (i.e. not as a DLL), APICALL should be defined as null. In this case,
 leave the definition "NOT_A_WIN_DLL" as given here.

 If you wish to compile the source files as a DLL, comment-out the
 line defining "NOT_A_WIN_DLL". In this case, the proper preceding
 string defined by "APICALL" will be assigned to each API function
 definition.
*/

//sbd: do not export functions
#define NOT_A_WIN_DLL


#ifdef NOT_A_WIN_DLL
  #define APICALL
#else
   #ifdef WIN32
      #define APICALL _declspec(dllexport)
   #else
      #define APICALL WINAPI _export
   #endif
#endif


/*
 Bit Packing
 -----------
 A single bitstream frame contains a total of 264 bits (33 bytes).
 260 bits contain the actual codec parameters and 4 more bits are
 contorl bits.
 In the diagram below, MSB == Most Significant Bit, and
 LSB == Least Significan Bit.

 Packing is available in one of the following two options:

 1. Standard SUN/RTP packing (if GSM_HOTMEDIA_BIT_PACKING is not defined)

 	BYTE 1  :  (MSB) 1101.... (LSB)
	BYTE 2  :  (MSB) ........ (LSB)
	.
	.
	BYTE 32 :  (MSB) ........ (LSB)
	BYTE 33 :  (MSB) ........ (LSB)

	The dots (...) present the codec parameter bits and the (1101) is
	the four RTP control bits (value of "13" - 13kbs ?).

 2. If GSM_HOTMEDIA_BIT_PACKING is defined, only the first byte is
    packed differently:

 	BYTE 1  :  (MSB) ....0100 (LSB)

	The dots (...) present the codec parameter bits and the (0100) is
	the four control bits (value of "4"), which overlap the MRTC coder
	control bits. This bit packing is a proprietary packing used in the
        IBM HotMedia projects (http://w3.software.ibm.com/net.media/). There
        is no need to use this bit packing for other applications.
*/
//#define GSM_HOTMEDIA_BIT_PACKING


/*
 SID Frames
 ----------

 Usualy, SID (Silence Insertion Descriptor) frames are send in two
 cases: (1) after the speech has ended, one SID frame is send which
 holds the background noise information for the decoder comfort noise
 generator. (2) During non-speech section, an SID is sent regularly
 every 8 frames (not like in the G723 !), in order to update the
 comfort noise.

 If the follwoing "SID_TRUNCATED_MODE" is defined, no SID frames will
 be send during non-speech sections. The single SID frame after each
 speech burst will, of course, be sent.

 If, on the other hand, you would like to send SID frames, but would
 like to decrease their
 rate, change the definition of "UPDATE_RATE"
 from 8 to something larger (gsm_enc.h). No changes need to be
 made in the decoder source files.


#define SID_TUNCATED_MODE
*/



/*
 The following are some general constants of the coder, useful
 when calling some API's. See the file-to-file main functions
 for examples.

 DO NOT CHANGE ANY OF THESE VALUES !!!
*/
#define GSM_ENCODER_INPUT_SIZE  160    /* 160 samples per frame */
#define GSM_ENCODER_OUTPUT_SIZE 33     /* 33 bytes bitstream */

#define GSM_DECODER_INPUT_SIZE  33
#define GSM_DECODER_OUTPUT_SIZE 160



/*
 All the API functions return one of the following return-codes,
 preceded by the "GSM_" string.
*/
typedef enum gsm_return_code {

    GSM_OK ,                 /* The function operated correctly          */

    GSM_ERR,                 /* A general error in function operation
                                has accrued.                               */

    GSM_MEMORY_ERROR         /* Memory allocation error for the "open" API functions */

}  GSMReturnCode;


/*
 The encoder can work in two modes, the regular (standard) mode, or a
 low complexity mode (running about 30% faster), where a 1:2 decimated
 pitch search is used. The "gsm_encoder_set_complexity" function can
 set the mode to either one. If not used, the default will be the
 regular (standard) mode, which is the recommended one.
*/
typedef enum gsm_complexity_mode{
      GSM_REGULAR ,         /* As in the standard (default) */
      GSM_LIGHT             /* Low complexity version ("light version") */
} GSMComplexityMode;

/*
 The encoder can work in two modes, the regular (standard) mode, with
 Voice Activity Detection (VAD), or the non-standard mode, without
 VAD.
*/
typedef enum gsm_vad_mode{
	  GSM_VAD_ENABLE ,  /* As in the standard (default) */
      GSM_VAD_DISABLE   /* No VAD */
} GSMVADMode;


/*
  The encoder classifies each input frame to one of the first three definitions,
  which can be observed by the application.

  The decoder receives one of the last two defintions, forcing generation of
  comfort noise or regular decodeing.
*/
typedef enum gsm_frame_type {


    GSM_SPEECH_FRAME,     /* (ENCODER) Current frame holds full speech inforamtion   */

    GSM_SID_FRAME,        /* (ENCODER) Current frame holds a Silence Insertion Descriptor,
                             i.e., the frame is a new silence frame, and the
							 encoder is sending the decoder information about the
							 comfort noise to be generated */

    GSM_SILENCE_FRAME,    /* (ENCODER) The frame holds silence (or background noise). In
	                         this case, all the 260 bitstream bits are zero */


    GSM_FORCE_SILENCE_FRAME, /* (DECODER) If this is passed to the decoder, silence frame
							    is inforced, resulting in a generation of comfort noise,
								and disregarding the bitstream buffer */
    GSM_REGULAR_FRAME        /* (DECODER) If this is passed to the decoder, the information
							    in the bitstream is decoder in the regular way, which means
							    a full speech frame or comfort noise will be generated
								according to the bitstream buffer */

}  GSMFrameType;



/*********************************************************************************
 *                                                                               *
 *                                                                               *
 *                         ENCODER API FUNCTIONS                                 *
 *                                                                               *
 *                                                                               *
 *********************************************************************************/

/*
   Allocate Memory For Encoder

   Returns GSM_OK or GSM_MEMORY_ERROR
*/
GSMReturnCode APICALL gsm_encoder_open(void **enc_state);

/*
   Set Encoder Complexity

   Returns GSM_OK
*/
GSMReturnCode APICALL gsm_encoder_set_complexity(void          *enc_state,
                                                 GSMComplexityMode  mode);

/*
   Set VAD mode (enable/disable)

   Returns GSM_OK
*/
GSMReturnCode APICALL gsm_encoder_set_vad_mode(void       *enc_state,
                                               GSMVADMode  mode);

/*
   Encode a Single Frame

   gsm_frame_type will retun one of the following three: GSM_SPEECH_FRAME,
   GSM_SID_FRAME or GSM_SILENCE_FRAME.

   Returns GSM_OK
*/
GSMReturnCode APICALL gsm_encode_frame(void   *enc_state,
                                       short  *input_samples,
                                       unsigned char  *output_bits,
				       GSMFrameType *gsm_frame_type);

/*
   Encode two GSM frames (320 samples) and pack them in MS GSM format (65 bytes)

   gsm_frame_type will retun one of the following three: GSM_SPEECH_FRAME,
   GSM_SID_FRAME or GSM_SILENCE_FRAME.

   Returns GSM_OK
*/
GSMReturnCode APICALL gsm_encode_frame_ms(void   *enc_state,
                                       short  *input_samples,
                                       unsigned char  *output_bits,
				       GSMFrameType *gsm_frame_type);

/*
   Free Decoder Allcated Memory

   Returns GSM_OK or GSM_ERR
*/
GSMReturnCode APICALL gsm_encoder_close(void **dec_state);


/*********************************************************************************
 *                                                                               *
 *                                                                               *
 *                         DECODER API FUNCTIONS                                 *
 *                                                                               *
 *                                                                               *
 *********************************************************************************/

/*
   Allocate Memory For Decoder

   Returns GSM_OK or GSM_MEMORY_ERROR
*/
GSMReturnCode APICALL gsm_decoder_open(void **dec_state);


/*
   Decode a Single Frame

   gsm_frame_type must be GSM_REGULAR_FRAME or GSM_FORCE_SILENCE_FRAME.

   Returns GSM_OK
*/
GSMReturnCode APICALL gsm_decode_frame(void   *dec_state,
                                        unsigned char  *input_bits,
					short  *output_samples,
					GSMFrameType gsm_frame_type);
/*
   Decode two frames packed in MicroSoft gsm format

   gsm_frame_type must be GSM_REGULAR_FRAME or GSM_FORCE_SILENCE_FRAME.

   Returns GSM_OK
*/
GSMReturnCode APICALL gsm_decode_frame_ms(void   *dec_state,
                                        unsigned char  *input_bits,
					short  *output_samples,
					GSMFrameType gsm_frame_type);

/*
   Decode a Single Frame, the frame is already unpacked to gsm paramters

   gsm_frame_type must be GSM_REGULAR_FRAME or GSM_FORCE_SILENCE_FRAME.

   Returns GSM_OK
*/

GSMReturnCode APICALL gsm_decode_frame_parameters(void   *dec_state,
                                        unsigned long  *input_paramters,
                                        short  *output_samples,
				        GSMFrameType gsm_frame_type);



/*
   Free Decoder Allcated Memory

   Returns GSM_OK or GSM_ERR
*/
GSMReturnCode APICALL gsm_decoder_close();


/* Definition for C++ integration */
#ifdef __cplusplus
}
#endif

