/*
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */ 

#ifndef __JAVACALL_MULTIMEDIA_H
#define __JAVACALL_MULTIMEDIA_H

/**
 * @file javacall_multimedia.h
 * @ingroup JSR135 
 * @brief Javacall interfaces for JSR-135 MMAPI
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/** 
 * @defgroup JSR135 Multimedia API (JSR-135)
 * @ingroup JTWI
 *
 * <H2>Introduction</H2>
 * Mobile Media API (MMAPI) JSR-135 is a Java technology for multimedia playback and recording. 
 * The aim of this document is to provide an overview of the requirements for MMAPI, focusing on tone generation, audio playback and video playback.
 * 
 * Exact requirements can be found in the following specifications:
 * 
 * - MIDP 2.1 Specification (JSR-118)
 * - MMAPI 1.2 Specification (JSR-135)
 * - JTWI 1.0 Specification (JSR-185)
 * 
 * The specifications can be downloaded from http://www.jcp.org
 * 
 * <H2>MIDP 2.0 Specification Media Requirements</H2>
 * The MIDP 2.0 specification specifies the Audio Building Block which is a subset of the MMAPI specification. The requirements are as follows:
 * 
 * - MUST support Tone Generation in the media package.
 * - MUST support 8-bit, 8 KHz, mono linear PCM wav format IF any sampled sound support is provided.
 * - MAY include support for additional sampled sound formats.
 * - MUST support Scalable Polyphony MIDI (SP-MIDI) and SP-MIDI Device 5-to-24 Note Profile IF any synthetic sound support is provided.
 * - MAY include support for additional MIDI format different types of media. When a Player is created for a particular type, 
 * 
 * 
 * <H2>JTWI Media Requirements</H2>
 * The JTWI specification adds the following media requirements:
 * If MMAPI is implemented, then MMAPI version 1.1 should be adhered to.
 * 
 * - HTTP 1.1 must be supported for media file download for all supported media formats.
 * - A compliant device must implement the MIDI feature set specified in MMAPI (JSR135). MIDI file playback must be supported.
 * - MIDI Support for VolumeControl must be implemented.
 * - A compliant implementation that supports the video feature set and video image capture must support JPEG encoding in Video Snapshots.
 * - Tone sequence file format must be supported.
 *
 *  @{
 */

#include "javacall_defs.h" 

/**
* @defgroup jsr135MediaFormatDefs Multimedia Format types and functions
*
* @ingroup jsr135MediaFormat
* @{
*/
/** Audio formats section */
/** MPEG-1 layer 2, audio/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG1_LAYER2      "MPEG1_layer_2"
/** MPEG-1 layer 3, audio/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG1_LAYER3      "MPEG1_layer_3"
/** MP3pro, audio/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG1_LAYER3_PRO  "MPEG1_layer_3_pro"
/** MPEG-2 Advanced_Audio_Coding, audio/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG2_AAC         "MPEG2_AAC"
/** MPEG-4 High Efficiency Advanced Audio Coding, audio/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG4_HE_AAC      "MPEG4_HE_AAC"
/** 3GPP Enhanced aacPlus, audio/3gpp */
#define JAVACALL_MEDIA_FORMAT_ENHANCED_AAC_PLUS "Enhanced_AAC_Plus"
/** 3GPP Adaptive Multi Rate audio/amr */
#define JAVACALL_MEDIA_FORMAT_AMR               "AMR"
/** 3GPP Wide Band Adaptive Multi Rate, audio/amr-wb */
#define JAVACALL_MEDIA_FORMAT_AMR_WB            "AMR_WB"
/** 3GPP Wide Band Adaptive Multi Rate plus, audio/amr-wb+ */
#define JAVACALL_MEDIA_FORMAT_AMR_WB_PLUS       "AMR_WB_Plus"
/** ETSI GSM full-rate speech codec, audio/x-gsm */
#define JAVACALL_MEDIA_FORMAT_GSM               "GSM"
/** ETSI GSM enhanced full-rate speech codec, audio/x-gsmefr */
#define JAVACALL_MEDIA_FORMAT_GSM_EFR           "GSM_EFR"
/** The Electronic Industries Association (EIA) & 
 * Telecommunications Industry Association (TIA) standard IS-733
 * audio/qcelp */
#define JAVACALL_MEDIA_FORMAT_QCELP             "QCELP"
/** MIDI Manufacturers Association, audio/midi */
#define JAVACALL_MEDIA_FORMAT_MIDI              "MIDI"
/** MIDI Manufacturers Association, audio/spmidi */
#define JAVACALL_MEDIA_FORMAT_SP_MIDI           "SP_MIDI"
/** Microsoft, linear Pulse Coded Modulation, audio/x-wav */
#define JAVACALL_MEDIA_FORMAT_MS_PCM            "MS_PCM"
/** Microsoft, Adaptive Delta Pulse Coded Modulation, audio/x-wav */
#define JAVACALL_MEDIA_FORMAT_MS_ADPCM          "MS_ADPCM"
/** Yamaha, Adaptive Delta Pulse Coded Modulation, audio/x-wav */
#define JAVACALL_MEDIA_FORMAT_YAMAHA_ADPCM      "YAMAHA_ADPCM"
/** Basic audio, audio/basic */
#define JAVACALL_MEDIA_FORMAT_AU                "AU"
/** OGG Vorbis, audio/x-vorbis */
#define JAVACALL_MEDIA_FORMAT_OGG_VORBIS        "OGG_VORBIS"
/** RealAudio ver 8, audio/x-realaudio */
#define JAVACALL_MEDIA_FORMAT_REALAUDIO_8       "REALAUDIO_8"
/** AIFF uncompressed, audio/x-aiff */
#define JAVACALL_MEDIA_FORMAT_AIFF              "AIFF"
/** Windows Media Audio, audio/x-ms-wma */
#define JAVACALL_MEDIA_FORMAT_WMA_9             "WMA_9"
/** Audio tone, audio/x-tone-seq */
#define JAVACALL_MEDIA_FORMAT_TONE              "TONE" 

/** Video formats section */
/** Default MJPEG, video/mjpeg */
#define JAVACALL_MEDIA_FORMAT_MJPEG_DEFAULT     "MJPEG_DEFAULT"
/** ITU H.263, video/h263 */
#define JAVACALL_MEDIA_FORMAT_H263              "H263"
/** ITU H.264, video/h264 */
#define JAVACALL_MEDIA_FORMAT_H264              "H264"
/** MPEG-1, video/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG_1            "MPEG_1"
/** MPEG-2, video/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG_2            "MPEG_2"
/** MPEG-4 Simple Visual Profile, video/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG_4_SVP        "MPEG_4_SVP"
/** MPEG-4 Advanced Video Codec (ITU H.264), video/mpeg */
#define JAVACALL_MEDIA_FORMAT_MPEG_4_AVC        "MPEG_4_AVC"
/** RealVideo ver 8, video/vnd.rn-realvideo */
#define JAVACALL_MEDIA_FORMAT_REALVIDEO_8       "REALVIDEO_8"
/** Windows Media Video, video/x-ms-wm */
#define JAVACALL_MEDIA_FORMAT_WMV_9             "WMV_9"

/** Container formats section */
/** 3GPP format audio only, audio/3gp */
#define JAVACALL_MEDIA_FORMAT_AUDIO_3GPP        "AUDIO_3GPP"
/** 3GPP format video, video/3gp */
#define JAVACALL_MEDIA_FORMAT_VIDEO_3GPP        "VIDEO_3GPP"
/** Microsoft avi file format, video/avi */
#define JAVACALL_MEDIA_FORMAT_AVI               "AVI"
/** Apple Quicktime, video/quicktime */
#define JAVACALL_MEDIA_FORMAT_MOV               "MOV"

/** Flash video */
#define JAVACALL_MEDIA_FORMAT_FLV               "FLV"

/** Image formats section */
/** MPEG JPEG, image/jpeg */
#define JAVACALL_MEDIA_FORMAT_JPEG              "JPEG"
/** MPEG JPEG 2000, image/jpeg */ 
#define JAVACALL_MEDIA_FORMAT_JPEG2000          "JPEG2000"
/** TIFF Specification, image/tiff */
#define JAVACALL_MEDIA_FORMAT_TIFF              "TIFF"
/** RFC2083 - PNG (Portable Network Graphics) Specification, image/png */
#define JAVACALL_MEDIA_FORMAT_PNG               "PNG"
/** GIF Specification, image/gif */
#define JAVACALL_MEDIA_FORMAT_GIF               "GIF"
/** 24b RGB format, image/raw */
#define JAVACALL_MEDIA_FORMAT_RGB888            "rgb888"
/** 32b+alfa RGB format, image/raw */
#define JAVACALL_MEDIA_FORMAT_RGBA8888          "rgba8888"
/** BW raw format, image/raw */
#define JAVACALL_MEDIA_FORMAT_GRAY1             "gray1"
/** 8b monochrome raw format, image/raw */
#define JAVACALL_MEDIA_FORMAT_GRAY8             "gray8"

/** Device formats section */
/** Interactive tone, device/tone */
#define JAVACALL_MEDIA_FORMAT_DEVICE_TONE       "DEVICE_TONE"
/** Interactive midi, device/midi */
#define JAVACALL_MEDIA_FORMAT_DEVICE_MIDI       "DEVICE_MIDI"

/** Capture formats section */
/** Capture audio, capture/audio */
#define JAVACALL_MEDIA_FORMAT_CAPTURE_AUDIO     "CAPTURE_AUDIO"
/** Capture radio, capture/radio */
#define JAVACALL_MEDIA_FORMAT_CAPTURE_RADIO     "CAPTURE_RADIO"
/** Capture video, capture/video */
#define JAVACALL_MEDIA_FORMAT_CAPTURE_VIDEO     "CAPTURE_VIDEO"

/** RTP formats section */
#define JAVACALL_MEDIA_FORMAT_RTP_DVI4          "RTP_DVI4"
#define JAVACALL_MEDIA_FORMAT_RTP_G722          "RTP_G722"
#define JAVACALL_MEDIA_FORMAT_RTP_G723          "RTP_G723"
#define JAVACALL_MEDIA_FORMAT_RTP_G726_16       "RTP_G726_16"
#define JAVACALL_MEDIA_FORMAT_RTP_G726_24       "RTP_G726_24"
#define JAVACALL_MEDIA_FORMAT_RTP_G726_32       "RTP_G726_32"
#define JAVACALL_MEDIA_FORMAT_RTP_G726_40       "RTP_G726_40"
#define JAVACALL_MEDIA_FORMAT_RTP_G728          "RTP_G728"
#define JAVACALL_MEDIA_FORMAT_RTP_G729          "RTP_G729"
#define JAVACALL_MEDIA_FORMAT_RTP_G729D         "RTP_G729D"
#define JAVACALL_MEDIA_FORMAT_RTP_G729E         "RTP_G729E"
#define JAVACALL_MEDIA_FORMAT_RTP_GSM           "RTP_GSM"
#define JAVACALL_MEDIA_FORMAT_RTP_GSM_EFR       "RTP_GSM_EFR"
#define JAVACALL_MEDIA_FORMAT_RTP_L8            "RTP_L8"
#define JAVACALL_MEDIA_FORMAT_RTP_L16           "RTP_L16"
#define JAVACALL_MEDIA_FORMAT_RTP_LPC           "RTP_LPC"
#define JAVACALL_MEDIA_FORMAT_RTP_MPA           "RTP_MPA"
#define JAVACALL_MEDIA_FORMAT_RTP_PCMA          "RTP_PCMA"
#define JAVACALL_MEDIA_FORMAT_RTP_PCMU          "RTP_PCMU"
#define JAVACALL_MEDIA_FORMAT_RTP_QCELP         "RTP_QCELP"
#define JAVACALL_MEDIA_FORMAT_RTP_RED           "RTP_RED"
#define JAVACALL_MEDIA_FORMAT_RTP_VDVI          "RTP_VDVI"
#define JAVACALL_MEDIA_FORMAT_RTP_BT656         "RTP_BT656"
#define JAVACALL_MEDIA_FORMAT_RTP_CELB          "RTP_CELB"
#define JAVACALL_MEDIA_FORMAT_RTP_JPEG          "RTP_JPEG"
#define JAVACALL_MEDIA_FORMAT_RTP_H261          "RTP_H261"
#define JAVACALL_MEDIA_FORMAT_RTP_H263          "RTP_H263"
#define JAVACALL_MEDIA_FORMAT_RTP_H263_1998     "RTP_H263_1998"
#define JAVACALL_MEDIA_FORMAT_RTP_H263_2000     "RTP_H263_2000"
#define JAVACALL_MEDIA_FORMAT_RTP_MPV           "RTP_MPV"
#define JAVACALL_MEDIA_FORMAT_RTP_MP2T          "RTP_MP2T"
#define JAVACALL_MEDIA_FORMAT_RTP_MP1S          "RTP_MP1S"
#define JAVACALL_MEDIA_FORMAT_RTP_MP2P          "RTP_MP2P"
#define JAVACALL_MEDIA_FORMAT_RTP_BMPEG         "RTP_BMPEG"
#define JAVACALL_MEDIA_FORMAT_RTP_NV            "RTP_NV"

/** Unknown format */
#define JAVACALL_MEDIA_FORMAT_UNKNOWN           "UNKNOWN"
/** Unsupported format */
#define JAVACALL_MEDIA_FORMAT_UNSUPPORTED       "UNSUPPORTED"

/**
 * 
 * Media format type definition.
 */
typedef javacall_const_ascii_string javacall_media_format_type;

/** @} */

/**
 * 
 * Multimedia protocol types.
 */
#define JAVACALL_MEDIA_MEMORY_PROTOCOL          0x01    // playback from memory buffer or memory streaming
#define JAVACALL_MEDIA_FILE_LOCAL_PROTOCOL      0x02    // "file://" local content
#define JAVACALL_MEDIA_FILE_REMOTE_PROTOCOL     0x04    // "file://" via network
#define JAVACALL_MEDIA_HTTP_PROTOCOL            0x08    // "http://"
#define JAVACALL_MEDIA_HTTPS_PROTOCOL           0x10    // "https://"
#define JAVACALL_MEDIA_RTP_PROTOCOL             0x20    // "rtp://"
#define JAVACALL_MEDIA_RTSP_PROTOCOL            0x40    // "rtsp://"

/**
 * 
 * Special protocol types.
 */
#define JAVACALL_MEDIA_CAPTURE_PROTOCOL       0x8000    // "capture://"

/**
 * 
 * @brief Multimedia Controls, supported by native layer
 */
#define JAVACALL_MEDIA_CTRL_VOLUME              0x001
#define JAVACALL_MEDIA_CTRL_RECORD              0x002
#define JAVACALL_MEDIA_CTRL_METADATA            0x004
#define JAVACALL_MEDIA_CTRL_STOPTIME            0x008
#define JAVACALL_MEDIA_CTRL_VIDEO               0x010
#define JAVACALL_MEDIA_CTRL_FRAME_POSITIONING   0x020
#define JAVACALL_MEDIA_CTRL_TONE                0x040
#define JAVACALL_MEDIA_CTRL_MIDI                0x080
#define JAVACALL_MEDIA_CTRL_PITCH               0x100
#define JAVACALL_MEDIA_CTRL_RATE                0x200
#define JAVACALL_MEDIA_CTRL_TEMPO               0x400

/** @} */

/**
 * struct javacall_media_caps
 * @brief Multimedia capabilities of native platform
 *        for particular media format
 */
typedef struct {
    /** Media format */
    javacall_media_format_type          mediaFormat;
    /** Content types for the media format, separated by space */
    javacall_const_ascii_string         contentTypes;

    /**  Bitmask of supported protocols for playback from 
     *   whole downloaded content including memory buffer
     */
    javacall_int32                      wholeProtocols;
    /**  Bitmask of supported streaming protocols 
     *   including streaming from memory buffer
     */
    javacall_int32                      streamingProtocols;
} javacall_media_caps;

/**
 * struct javacall_media_configuration
 * @brief Multimedia capabilities of native platform
 */
typedef struct {
    /** Support mixing */
    javacall_bool                       supportMixing;
    /** Support recording */
    javacall_bool                       supportRecording;
    /** Supported capture audio formats and parameters;
     *  NULL if not supported
     */
    javacall_const_ascii_string         audioEncoding;
    /** Supported capture video formats and parameters;
     *  NULL if not supported 
     */
    javacall_const_ascii_string         videoEncoding;
    /** Supported video snapshot formats and parameters;
     *  NULL if not supported
     */
    javacall_const_ascii_string         videoSnapshotEncoding;

    /** Support Device Tone */
    javacall_bool                       supportDeviceTone;
    /** Support Device MIDI */
    javacall_bool                       supportDeviceMIDI;
    /** Support Capture Radio */
    javacall_bool                       supportCaptureRadio;

    /** Media capabilities.
     *  An array of javacall_media_caps for each supported media format.
     *  The last element in the array should have NULL mediaFormat field.
     */
    javacall_media_caps*                mediaCaps;
} javacall_media_configuration;

/* Java MMAPI JTS Values */
#define JAVACALL_SET_VOLUME  -8
#define JAVACALL_SILENCE     -1

/** @} */

/**********************************************************************************/

/**
 * @defgroup MediaMandatoryInitFunctions Mandatory Media initialization functions
 * @ingroup JSR135
 *
 * @brief Initialize multimedia javacall library
 * 
 * @{
 */

/**
 * Call this function when VM starts
 * Perform global initialization operation
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail
 */
javacall_result javacall_media_initialize(void);

/**
 * Call this function when VM ends 
 * Perform global free operation
 * 
 * @retval JAVACALL_OK      success
 * @retval JAVACALL_FAIL    fail 
 */
javacall_result javacall_media_finalize(void);
 
/** @} */

/**********************************************************************************/

/**
 * @defgroup MediaMandatoryConfiguration     Mandatory Configuration API
 * @ingroup JSR135
 *
 * @brief Configure MMAPI library
 * 
 * @{
 */

/**
 * Get multimedia configuration of the device.
 * This function should return pointer to static consfiguration structure with
 * static array of javacall_media_caps values.
 *
 * @retval JAVACALL_OK               success
 *         JAVACALL_INVALID_ARGUMENT if argument is NULL
 */
javacall_result javacall_media_get_configuration(
                    /*OUT*/ const javacall_media_configuration** configuration);

/** @} */ 

/**********************************************************************************/

/**
 * @defgroup MediaMandatoryBasic         Mandatory Basic media API
 * @ingroup JSR135
 *
 * @brief Basic multimedia functionality
 * 
 * @{
 */

/**
 * This function is called to get all the necessary return values from 
 * the JavaCall Media functions that can run in asynchronous mode.
 * This function is called every time the following situation occurs.
 * A JSR-135 JavaCall API function returned JAVACALL_WOULD_BLOCK and continued
 * its 
 * execution in asynchronous mode. Then it finished the execution and send the
 * corresponding event to inform Java layer about it. Such events are described
 * in the description of the enum javacall_media_notification_type after the
 * event 
 * JAVACALL_EVENT_MEDIA_JAVA_EVENTS_MARKER. After the event Java
 * layer calls javacall_media_get_event_data() to get the return values.
 *
 * @param handle        handle to the native player that the function having
 *                      returned JAVACALL_WOULD_BLOCK was called for.
 * @param eventType     the type of the event, one of 
 *                      javacall_media_notification_type (but greater than 
 *                      JAVACALL_EVENT_MEDIA_JAVA_EVENTS_MARKER)
 * @param pResult       The event data passed as the param \a data to the
 *                      function javanotify_on_media_notification() while
 *                      sending the event
 * @param numArgs       the number of return values to get
 * @param args          the pointer to the array to copy the return values to
 *
 * @retval JAVACALL_INVALID_ARGUMENT    bad arguments or the function should
 *                                      not be called now for this native
 *                                      player and eventType (no event has been
 *                                      sent, see the function description)
 * @retval JAVACALL_OK                  Success
 * @retval JAVACALL_FAIL                General failure
 * @see JAVACALL_WOULD_BLOCK
 * @see javacall_media_notification_type
 * @see JAVACALL_EVENT_MEDIA_JAVA_EVENTS_MARKER
 */
javacall_result javacall_media_get_event_data(javacall_handle handle, 
                    int eventType, void *pResult, int numArgs, void *args[]);

/**
 * Java MMAPI call this function to create native media handler.
 * This function is called at the first time to initialize native library.
 * You can do your own initialization job from this function.
 * 
 * @param appID         Unique application ID for this playing
 * @param playerId      Unique player object ID for this playing
 * @param uri           URI unicode string to media data.
 * @param uriLength     String length of URI
 * @param handle        Handle of native library.
 *
 * @retval JAVACALL_OK                      Success
 * @retval JAVACALL_CONNECTION_NOT_FOUND    Could not connect to the URL
 * @retval JAVACALL_IO_ERROR                IO error occurred while connecting
 *                                          the URL or getting data 
 * @retval JAVACALL_INVALID_ARGUMENT        Invalid URL or other parameter
 * @retval JAVACALL_NO_AUDIO_DEVICE     No audio device found and therefore
 *                                      playback is impossible. JVM will throw
 *                                      a MediaException. Please return this
 *                                      code only in case you want to
 *                                      reject playback, i.e. when the content
 *                                      is audio only. If some kind of playback
 *                                      is still possible (e.g. mute video),
 *                                      please return JAVACALL_OK instead
 * @retval JAVACALL_FAIL                    General failure or the following
 *                                          situation. Porting Layer may
 *                                          decide to reject the creation for
 *                                          some reason. For example, if you
 *                                          do not want media from some
 *                                          pre-defined "bad" Internet site.
 *                                          
 */
javacall_result javacall_media_create(int appID,
                                      int playerID,
                                      javacall_const_utf16_string uri, 
                                      long uriLength,
                                      /*OUT*/ javacall_handle* handle);

/**
 * Get the format type of media content
 *
 * @param handle    Handle to the library 
 * @param format    Format type
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_format(javacall_handle handle, 
                              /*OUT*/ javacall_media_format_type* format);

/**
 * Return bitmask of Media Controls supported by native player
 * 
 * Only Media Controls supported by native layer should be indicated
 *
 * @param handle    Handle to the library 
 * @param controls  bitmasks for Media Control implemented in native layer
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_player_controls(javacall_handle handle,
                              /*OUT*/ int* controls);

/**
 * Close native media player that created by creat or creat2 API call
 * After this call, you can't use any other function in this library
 * except for javacall_media_destroy
 * 
 * @param handle  Handle to the library.
 * 
 * @retval JAVACALL_OK      Java VM will proceed as if there is no problem
 * @retval JAVACALL_FAIL    Java VM will raise the media exception
 */
javacall_result javacall_media_close(javacall_handle handle);

/**
 * finally destroy native media player previously closed by
 * javacall_media_close. intended to be used by finalizer
 * 
 * @param handle  Handle to the library.
 * 
 * @retval JAVACALL_OK      Java VM will proceed as if there is no problem
 * @retval JAVACALL_FAIL    Java VM will raise the media exception
 */
javacall_result javacall_media_destroy(javacall_handle handle);

/**
 * Request to acquire device resources used to play media data.
 * You could implement this function to control device resource usage.
 * If there is no valid device resource to play media data, return JAVACALL_FAIL.
 * 
 * @param handle    Handle to the library
 * 
 * @retval JAVACALL_OK      Java VM will proceed as if there is no problem
 * @retval JAVACALL_FAIL    Java VM will raise the media exception
 */
javacall_result javacall_media_acquire_device(javacall_handle handle);

/**
 * Release device resource. 
 * Java MMAPI call this function to release limited device resources.
 * 
 * @param handle    Handle to the library
 * 
 * @retval JAVACALL_OK      Java VM will proceed as if there is no problem
 * @retval JAVACALL_FAIL    Nothing happened now. Same as JAVACALL_OK.
 */
javacall_result javacall_media_release_device(javacall_handle handle);

/**
 * Ask to the native layer if it will handle media download from specific URL.
 * Is media download for specific URL (provided in javacall_media_create)
 * will be handled by native layer or Java layer?
 * If isHandled is JAVACALL_TRUE, Java do not call 
 * javacall_media_do_buffering function
 * In this case, native layer should handle all of data gathering by itself
 * 
 * @param handle    Handle to the library
 * @param isHandled JAVACALL_TRUE if native player will handle media download
 * 
 * @retval JAVACALL_OK      
 * @retval JAVACALL_FAIL    
 */
javacall_result javacall_media_download_handled_by_device(javacall_handle handle,
                                                  /*OUT*/ javacall_bool* isHandled);

/**
 * This function returns desired size of Java Layer buffer for downloaded media content
 * It is possible if function returns different values for the same player in case of:
 *    - format of media data is unknown
 *    - format of media data is successfully discovered
 * Java Layer will call this function two times to create/update java Layer buffers:
 *    1) before downloading media content
 *    2) after 
 *
 * @param handle    Handle to the library
 * @param java_buffer_size  Desired size of java buffer
 * @param first_data_size  Size of the first chunk of media data, 
 *                          provided from Java to native
 * 
 * @retval JAVACALL_OK
 * @retval JAVACALL_FAIL
 * @retval JAVACALL_NOT_IMPLEMENTED
 */
javacall_result javacall_media_get_java_buffer_size(javacall_handle handle,
                                 /*OUT*/ long* java_buffer_size, 
                                 /*OUT*/ long* first_chunk_size);

/**
 * This function is called by Java Layer to notify javacall implementation about 
 * whole size of media content. This function is called in prefetch stage if 
 * whole size of media content is known only.
 *
 * @param handle    Handle to the library
 * @param whole_content_size  size of whole media content
 * 
 * @retval JAVACALL_OK
 * @retval JAVACALL_FAIL
 */
javacall_result javacall_media_set_whole_content_size(javacall_handle handle,
                                 long whole_content_size);

/**
 * Get native buffer address to store media content
 * 
 * @param handle    Handle to the library
 * @param buffer    Native layer provides address of data buffer for media content. 
 *                  Java layer will store downloaded media data to the provided buffer.
 *                  The size of data stored in the buffer should be equal or divisible 
 *                  by minimum media data chunk size and less or equal to max_size
 * @param max_size  The maximum size of data can be stored in the buffer
 * 
 * @retval JAVACALL_OK
 * @retval JAVACALL_FAIL   
 */
javacall_result javacall_media_get_buffer_address(javacall_handle handle, 
                                 /*OUT*/ const void** buffer, 
                                 /*OUT*/ long* max_size);

/**
 * Java MMAPI call this function to send media data to this library.
 * This function can be called multiple times to send large media data.
 * Native library can implement buffering by using any method (file, heap, etc...)
 * Buffering always occurs sequentially, not randomly.
 * 
 * When there is no more data, the buffer is set to NULL and the length to -1.
 * OEM should care about this case.
 * 
 * @param handle    Handle to the library.
 * @param buffer    Media data buffer pointer. Can be NULL at end of buffering.
 * @param length    Length of media data. Can be -1 at end of buffering.
 *                  If success returns length of data processed.
 * @param need_more_data    returns JAVACALL_FALSE if no more data is required
 *                          at the moment, otherwise returns JAVACALL_TRUE
 * @param next_chunk_size   next expected buffering data size
 *                          must be divisible by this value
 * 
 * @retval JAVACALL_OK
 * @retval JAVACALL_FAIL   
 * @retval JAVACALL_INVALID_ARGUMENT
 */
javacall_result javacall_media_do_buffering(javacall_handle handle, 
                                 const void* buffer,
                                 /*INOUT*/ long* length,
                                 /*OUT*/ javacall_bool* need_more_data,
                                 /*OUT*/ long* next_chunk_size);

/**
 * MMAPI call this function to clear(delete) buffered media data
 * You have to clear any resources created from previous buffering
 * 
 * @param handle    Handle to the library
 * 
 * @retval JAVACALL_OK      Can clear buffer
 * @retval JAVACALL_FAIL    Can't clear buffer. JVM can't erase resources.
 */
javacall_result javacall_media_clear_buffer(javacall_handle handle);

/**
 * Realize native player.
 * This function will be called by Java Layer to start Realize native player.
 * 
 * @param handle        Handle to the library
 * @param mime          Mime type unicode string. 
 *                      NULL if unknown
 * @param mimeLength    String length of media MIME type.
 * 
 * @retval JAVACALL_OK                  Success
 * @retval JAVACALL_NO_AUDIO_DEVICE     No audio device found and therefore
 *                                      playback is impossible. JVM will throw
 *                                      a MediaException. Please return this
 *                                      code only in case you want to
 *                                      reject playback, i.e. when the content
 *                                      is audio only. If some kind of playback
 *                                      is still possible (e.g. mute video),
 *                                      please return JAVACALL_OK instead
 * @retval JAVACALL_FAIL                General failure
 */
javacall_result javacall_media_realize(javacall_handle handle,
                                      javacall_const_utf16_string mime,
                                      long mimeLength);

/**
 * Prefetch native player. 
 * This function will be called by Java Layer to Prefetch native player.
 * 
 * @param handle    Handle to the library
 * 
 * @retval JAVACALL_OK
 * @retval JAVACALL_FAIL   
 */
javacall_result javacall_media_prefetch(javacall_handle handle);

/**
 * Try to start media playing.<br>
 * If this API return JAVACALL_FAIL, MMAPI will raise the media exception.<br>
 * If this API return JAVACALL_OK, MMAPI will return from start method.
 * 
 * @param handle    Handle to the library
 * 
 * @retval JAVACALL_OK      JVM will proceed as if there is no problem
 * @retval JAVACALL_FAIL    JVM will raise the media exception
 */
javacall_result javacall_media_start(javacall_handle handle);

/**
 * Stop media playing.
 * If this API return JAVACALL_FAIL, MMAPI will raise the media exception.<br>
 * If this API return JAVACALL_OK, MMAPI will return from stop method.
 * 
 * @param handle      Handle to the library
 * 
 * @retval JAVACALL_OK      JVM will proceed as if there is no problem
 * @retval JAVACALL_FAIL    JVM will raise the media exception
 */
javacall_result javacall_media_stop(javacall_handle handle);

/**
 * Pause media playing
 * 
 * @param handle      Handle to the library
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_pause(javacall_handle handle);

/**
 * Resume media playing
 * 
 * @param handle      Handle to the library
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_resume(javacall_handle handle);

/**
 * Get current media time (position) in ms unit
 * 
 * @param handle    Handle to the library
 * @param ms        current media time in ms
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_time(javacall_handle handle, /*OUT*/ long* ms );

/**
 * Seek to specified time.
 * This function can be called during play status or stop status
 * 
 * @param handle    Handle to the library
 * @param ms        Seek position as ms time, return actual time in ms
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_time(javacall_handle handle, /*INOUT*/ long* ms);
 
/**
 * Get whole media time in ms.
 * This function can be called during play status or stop status.
 * 
 * @param handle    Handle to the library
 * @param ms        return time in ms
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_NO_DATA_AVAILABLE
 */
javacall_result javacall_media_get_duration(javacall_handle handle, /*OUT*/ long* ms);

/** @} */

/**********************************************************************************/

/**
 * @defgroup MediaMandatoryVolumeControl      Mandatory VolumeControl API
 *
 * @brief Volume control functions - Implement VolumeControl
 * 
 * @ingroup JSR135
 * @{
 */

/**
 * Get current audio volume
 * Audio volume range have to be in 0 to 100 inclusive
 * 
 * @param handle    Handle to the library 
 * @param volume    Volume value
 *
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_NO_DATA_AVAILABLE
 */
javacall_result javacall_media_get_volume(javacall_handle handle, /*OUT*/ long* volume); 

/**
 * Set audio volume
 * Audio volume range have to be in 0 to 100 inclusive
 * 
 * @param handle    Handle to the library 
 * @param level     Volume value, return actual volume level
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_NO_DATA_AVAILABLE
 */
javacall_result javacall_media_set_volume(javacall_handle handle, /*INOUT*/ long* level);

/**
 * Is audio muted now?
 * 
 * @param handle    Handle to the library 
 * @param mute      JAVACALL_TRUE in mute state, 
 *                  JAVACALL_FALSE in unmute state
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_NO_DATA_AVAILABLE
 */
javacall_result javacall_media_is_mute(javacall_handle handle, /*OUT*/ javacall_bool* mute);

/**
 * Mute, Unmute audio
 * 
 * @param handle    Handle to the library 
 * @param mute      JAVACALL_TRUE to mute, JAVACALL_FALSE to unmute
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail. JVM will ignore this return value now.
 */ 
javacall_result javacall_media_set_mute(javacall_handle handle, javacall_bool mute);

/** @} */

/**********************************************************************************/

/**
 * @defgroup MediaMandatorySimpleTone         Mandatory Simple tone play API
 * @ingroup JSR135
 *
 * @brief Basic tone playing functions
 * 
 * @{
 */

/**
 * Play simple tone
 *
 * @param note     the note to be played. From 0 to 127 inclusive.
 *                 The frequency of the note can be calculated from the following formula:
 *                    SEMITONE_CONST = 17.31234049066755 = 1/(ln(2^(1/12)))
 *                    note = ln(freq/8.176)*SEMITONE_CONST
 *                    The musical note A = MIDI note 69 (0x45) = 440 Hz.
 * @param appID    ID of the application playing the tone
 * @param duration the duration of the note in ms 
 * @param volume   volume of this play. From 0 to 100 inclusive.
 * 
 * @retval JAVACALL_OK                  Success
 * @retval JAVACALL_NO_AUDIO_DEVICE     No audio device found. JVM will throw a
 *                                      MediaException
 * @retval JAVACALL_FAIL                General fail. JVM will raise the media
 *                                      exception.
 */
javacall_result javacall_media_play_tone(int appID, long note, long duration, long volume);

/**
 * Stop tone
 * 
 * @param appID             ID of the application playing the tone
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail. JVM will ignore this return value now.
 */
javacall_result javacall_media_stop_tone(int appID);

/**
 * @defgroup MediaOptionalDualTone         Optional Dual tone play API
 * @ingroup JSR135
 *
 * @brief Extended (optional) tone playing functions
 * 
 * @{
 */

/**
 * play dual tone (2 tones at the same time)
 *
 * @param noteA    the note to be played together with noteB. From 0 to 127 inclusive.
 * @param noteB    the note to be played together with noteA. From 0 to 127 inclusive.
 *                 The frequency of the note can be calculated from the following formula:
 *                    SEMITONE_CONST = 17.31234049066755 = 1/(ln(2^(1/12)))
 *                    note = ln(freq/8.176)*SEMITONE_CONST
 *                    The musical note A = MIDI note 69 (0x45) = 440 Hz.
 * @param
 * @param appID    ID of the application playing the tone
 * @param duration the duration of the note in ms 
 * @param volume   volume of this play. From 0 to 100 inclusive.
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail. JVM will raise the media exception.
 */
javacall_result javacall_media_play_dualtone(int appID, long noteA, long noteB, long duration, long volume);

/** @} */
/** @} */

/**********************************************************************************/

/**
 * @defgroup MediaOptionalVideoControl       Optional VideoControl API
 * @ingroup JSR135
 * 
 * @brief VideoControl controls the display of video. 
 * A Player which supports the playback of video must provide a VideoControl 
 * via its getControl and getControls  method.
 *
 * @{
 */

/**
 * Turn on or off video rendering destination color keying.
 * If this is on OEM native layer video renderer SHOULD use this color key
 * and draw on only the region that is filled with this color value.
 * 
 * @image html setalpha.png
 * 
 * @param handle Handle to the native player
 * @param on     Is color keying on?
 * @param color  Color key
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_video_color_key(javacall_handle handle,
    javacall_bool on, javacall_pixel color);

/**
 * Get original video width
 * 
 * @param handle    Handle to the library 
 * @param width     Pointer to long variable to get width of video
 * @param height    Pointer to long variable to get height of video
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_video_size(javacall_handle handle, 
                                              /*OUT*/ long* width, /*OUT*/ long* height);

/**
 * Set video rendering position in physical screen
 * 
 * @param handle    Handle to the library 
 * @param x         X position of rendering in pixels
 * @param y         Y position of rendering in pixels
 * @param w         Width of rendering
 * @param h         Height of rendering
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_video_location(javacall_handle handle, 
                                                  long x, long y, long w, long h);

/**
 * Set video preview visible state to show or hide
 * 
 * @param handle    Handle to the library
 * @param visible   JAVACALL_TRUE to show or JAVACALL_FALSE to hide
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_video_visible(javacall_handle handle, javacall_bool visible);
    
/**
 * Start get current snapshot of video data
 * When snapshot operation done, call callback function to provide snapshot image data to Java.
 *
 * @param handle            Handle to the library
 * @param imageType         Snapshot image type format as unicode string. 
 *                          For example, "encoding=png&width=128&height=128".
 *                          See Manager class section from MMAPI specification for detail.
 * @param length            imageType unicode string length
 * 
 * @retval JAVACALL_OK          Success.
 * @retval JAVACALL_WOULD_BLOCK This operation could takes long time. 
 *                              After this operation finished, MUST send 
 *                              JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED by using 
 *                              "javanotify_on_media_notification" function call
 * @retval JAVACALL_FAIL        Fail. Invalid encodingFormat or some errors.
 */
javacall_result javacall_media_start_video_snapshot(javacall_handle handle, 
                                                    javacall_const_utf16_string imageType, long length);

/**
 * Get snapshot data size
 * 
 * @param handle    Handle to the library
 * @param size      Size of snapshot data
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_video_snapshot_data_size(javacall_handle handle, 
                                                            /*OUT*/ long* size);

/**
 * Get snapshot data
 * 
 * @param handle    Handle to the library
 * @param buffer    Buffer will contains the snapshot data
 * @param size      Size of snapshot data
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_video_snapshot_data(javacall_handle handle, 
                                                       /*OUT*/ char* buffer, long size);


/**
 * Set video fullscreen mode
 * 
 * @param handle    Handle to the library 
 * @param fullScreenMode whether to set video playback in fullscreen mode
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 * @retval JAVACALL_NOT_IMPLEMENTED    Native FullScreen mode not implemented
 */
javacall_result javacall_media_set_video_full_screen_mode(javacall_handle handle, javacall_bool fullScreenMode);

/** @} */

 /**
 * @defgroup MediaOptionalImageEncoding  Optional image encoding
 *           API
 * @ingroup JSR135
 * 
 * @brief Image encoding is used by snapshot API if native player
 *        doesn't provide such functionality.
 * 
 * @note  Currently it is used for GIF player snapshot only.
 *
 * @{
 */
 
/**
 * @enum javacall_encoder_type
 * @brief Supported native encoders
 */
typedef enum {
    JAVACALL_JPEG_ENCODER,
    JAVACALL_PNG_ENCODER
}javacall_encoder_type;

/**
 * Encodes given raw RGB888 image to specified format.
 * 
 * @param rgb888        [IN] soure raw image to be encoded
 * @param width         [IN] source image width
 * @param height        [IN] source image height
 * @param encode        [IN]destination format
 * @param quality       [IN]quality of encoded image (for format
 *                      with losses)
 * @param result_buffer [OUT]a pointer where result buffer will
 *                      be stored. Must be released by
 *                      <tt>javacall_media_release_data</tt>
 *                      when it is no more need.
 * @param result_buffer_len [OUT] a pointer for result buffer
 *                          size
 * @param context       [OUT] a context saved during
 *                      asynchronous operation. An javacall
 *                      implementation is responsible for
 *                      allocation/releasing of the handle.
 * 
 * @retval  JAVACALL_OK  in case of success,
 * @retval  JAVACALL_OUT_OF_MEMORY if there is no memory for
 *          destination buffer
 * @retval  JAVACALL_FAIL if encoder failed
 *          JAVACALL_WOULD_BLOCK if operation requires time to
 *          complete, an application should call
 *          <tt>javacall_media_encode_finish</tt> later in
 *          respose to JAVACALL_EVENT_MEDIA_ENCODE_COMPLETE
 *          event.
 */
javacall_result javacall_media_encode_start(javacall_uint8* rgb888, javacall_uint8 width, javacall_uint8 height,
                                      javacall_encoder_type encode, javacall_uint8 quality,
                                            javacall_uint8** result_buffer, javacall_uint32* result_buffer_len,
                                            javacall_handle* context);

/**
 * Finish encode procedure for given raw RGB888 image.
 * 
 * @param result_buffer [OUT]a pointer where result buffer will
 *                      be stored. Must be released by
 *                      <tt>javacall_media_release_data</tt>
 *                      when it is no more need.
 * @param result_buffer_len [OUT] a pointer for result buffer
 *                          size
 * @param context       [IN] a context saved during
 *                      asynchronous operation. . An javacall
 *                      implementation is responsible for
 *                      allocation/releasing of the handle.
 * 
 * @retval  JAVACALL_OK  in case of success
 * @retval  JAVACALL_OUT_OF_MEMORY if there is no memory
 *          for destination buffer
 * @retval  JAVACALL_FAIL if encoder failed
 * @retval  JAVACALL_WOULD_BLOCK if operation is not complete
 *          yet, an application should recall this function
 *          later again. Generally this error code means that
 *          blocked java thread was unblocked at wrong time,
 *          probably due to implemenetation bug.
 * 
 */
javacall_result javacall_media_encode_finish(javacall_handle context,
                                      javacall_uint8** result_buffer, javacall_uint32* result_buffer_len);

/**
 * Release a data was acuired by
 * <tt>javacall_media_encode_start</tt> or
 * <tt>javacall_media_encode_finish</tt>
 * 
 * @param result_buffer     a pointer to a buffer need to be
 *                          released
 * @param result_buffer_len the buffer length
 */
void javacall_media_release_data(javacall_uint8* result_buffer, javacall_uint32 result_buffer_len);

/** @} */

 /**
 * @defgroup MediaOptionalSystemVolume  Optional system volume API
 * 
 * @ingroup JSR135
 * 
 * @brief System (Master) volume can be adjusted e.g. by dedicated volume keys.
 *        This API provides a way to get the volume level value from 
 *        the platform.
 * 
 * @{
 */
 
/**
 * Get current system audio volume level.
 * Audio volume range have to be in 0 to 100 inclusive. 0 means that audio is
 * muted.
 *
 * @note Player's volume level will be multiplied by the system volume 
 *       (divided by 100) before the javacall_media_set_volume() method will be 
 *       called by the Java layer. To block this calculation 
 *       the javacall_media_get_system_volume() method must return 
 *       JAVACALL_NO_DATA_AVAILABLE.
 *
 * @note If the device have a system mute/unmute capability 
 *       the Javacall layer is responsible for saving/restoring the current 
 *       system volume level when the audio is muted/unmuted.
 * 
 * @note This method must return JAVACALL_NO_DATA_AVAILABLE when the 
 *       System volume feature is not implemented.
 *
 * @param volume        Volume value
 *
 * @retval JAVACALL_OK                Success
 * @retval JAVACALL_NO_DATA_AVAILABLE System volume is not available
 */
javacall_result javacall_media_get_system_volume(/*OUT*/ javacall_int32 *volume);
/** @} */

/**********************************************************************************/

/**
 * @defgroup MediaOptionalFramePositioningControl    Optional FramePositioningControl API
 * @ingroup JSR135
 * 
 * @brief The FramePositioningControl is the interface to control precise 
 * positioning to a video frame for Players.
 *  
 * @{
 */

/**
 * Converts the given frame number to the corresponding media time in milli second unit.
 * 
 * @param handle    Handle to the library 
 * @param frameNum  The input frame number for the conversion
 * @param ms        The converted media time in milli seconds for the given frame
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_map_frame_to_time(javacall_handle handle, 
                                                 long frameNum, /*OUT*/ long* ms);

/**
 * Converts the given media time to the corresponding frame number.
 * 
 * @param handle    Handle to the library 
 * @param ms        The input media time for the conversion in milli seconds
 * @param frameNum  The converted frame number for the given media time. 
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail 
 */
javacall_result javacall_media_map_time_to_frame(javacall_handle handle, 
                                                 long ms, /*OUT*/ long* frameNum);

/**
 * Seek to a given video frame.
 * If the given frame number is less than the first or larger than the last frame number in the media, 
 * seek  will jump to either the first or the last frame respectively.
 * 
 * @param handle            Handle to the library 
 * @param frameNum          The frame to seek to
 * @param actualFrameNum    The actual frame that the Player has seeked to
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_seek_to_frame(javacall_handle handle, 
                                             long frameNum, /*OUT*/ long* actualFrameNum);

/**
 * Skip a given number of frames from the current position.
 * 
 * @param handle        Handle to the library 
 * @param nFrames       The number of frames to skip from the current position. 
 *                      If framesToSkip is negative, it will seek backward 
 *                      by framesToSkip number of frames.
 *                      Return number of actual skipped frames
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_skip_frames(javacall_handle handle, 
                                           /*INOUT*/ long* nFrames);

/** @} */ 

/**********************************************************************************/

/**
 * @defgroup MediaOptionalMetaDataControl    Optional MetaDataControl API
 * @ingroup JSR135
 * 
 * @brief MetaDataControl is used to retrieve metadata information included within the media streams.
 *  
 * @{
 */

/**
 * Get supported meta data key counts
 * 
 * @param handle    Handle to the library 
 * @param keyCounts Return meta data key string counts
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_metadata_key_counts(javacall_handle handle, 
                                                       /*OUT*/ long* keyCounts);

/**
 * Get meta data key strings by using index value
 * 
 * @param handle    Handle to the library 
 * @param index     Meta data key string's index value. from 0 to 'key counts - 1'.
 * @param bufLength keyBuf buffer's size in bytes. 
 * @param keyBuf    Buffer that used to return key strings. 
 *                  NULL value should be appended to the end of string.
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_OUT_OF_MEMORY   keyBuf size is too small
 * @retval JAVACALL_FAIL            Fail
 */
javacall_result javacall_media_get_metadata_key(javacall_handle handle, 
                                                long index, 
                                                long bufLength, 
                                                /*OUT*/ javacall_utf16* keyBuf);

/**
 * Get meta data value strings by using meta data key string
 * 
 * @param handle    Handle to the library 
 * @param key       Meta data key string
 * @param bufLength dataBuf buffer's size in bytes. 
 * @param dataBuf   Buffer that used to return meta data value as 
 *                  null-terminated string.
 *
 * @note  If the meta data value is null, the output buffer will contain
 *        following two 16-bit units: 0xffff, 0x0000.
 *
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_OUT_OF_MEMORY   dataBuf size is too small
 * @retval JAVACALL_FAIL            Fail
 */
javacall_result javacall_media_get_metadata(javacall_handle handle, 
                                            javacall_const_utf16_string key, 
                                            long bufLength, 
                                            /*OUT*/ javacall_utf16* dataBuf);

/** @}*/

/**********************************************************************************/

/**
 * @defgroup MediaOptionalMIDIControl        Optional MIDIControl API
 * @ingroup JSR135
 * 
 * @brief MIDIControl provides access to MIDI rendering and transmitting devices.
 *   
 * @{
 */

/**
 * Get volume for the given channel. 
 * The return value is independent of the master volume, which is set and retrieved with VolumeControl.
 * 
 * @param handle    Handle to the library 
 * @param channel   0-15
 * @param volume    channel volume, 0-127, or -1 if not known
 * 
 * @retval JAVACALL_OK                  Success
 * @retval JAVACALL_INVALID_ARGUMENT    channel value is out of range
 * @retval JAVACALL_FAIL                Fail
 */
javacall_result javacall_media_get_channel_volume(javacall_handle handle, 
                                                  long channel, /*OUT*/ long* volume);

/**
 * Set volume for the given channel. To mute, set to 0. 
 * This sets the current volume for the channel and may be overwritten during playback by events in a MIDI sequence.
 * 
 * @param handle    Handle to the library 
 * @param channel   0-15
 * @param volume    channel volume, 0-127
 * 
 * @retval JAVACALL_OK                  Success
 * @retval JAVACALL_INVALID_ARGUMENT    channel or volume value is out of range
 * @retval JAVACALL_FAIL                Fail
 */
javacall_result javacall_media_set_channel_volume(javacall_handle handle, 
                                                  long channel, long volume);

/**
 * Set program of a channel. 
 * This sets the current program for the channel and may be overwritten during playback by events in a MIDI sequence.
 * 
 * @param handle    Handle to the library 
 * @param channel   0-15
 * @param bank      0-16383, or -1 for default bank
 * @param program   0-127
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_program(javacall_handle handle, 
                                           long channel, long bank, long program);

/**
 * Sends a short MIDI event to the device.
 * 
 * @param handle    Handle to the library 
 * @param type      0x80..0xFF, excluding 0xF0 and 0xF7, which are reserved for system exclusive
 * @param data1     for 2 and 3-byte events: first data byte, 0..127
 * @param data2     for 3-byte events: second data byte, 0..127
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_short_midi_event(javacall_handle handle,
                                                long type, long data1, long data2);

/**
 * Sends a long MIDI event to the device, typically a system exclusive message.
 * 
 * @param handle    Handle to the library 
 * @param data      array of the bytes to send. 
 *                  This memory buffer will be freed after this function returned.
 *                  So, you should copy this data to the other internal memory buffer
 *                  if this function needs data after return.
 * @param offset    start offset in data array
 * @param length    number of bytes to be sent
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_long_midi_event(javacall_handle handle,
                                               const char* data, long offset, /*INOUT*/ long* length);


/**
 * This function is used to ascertain the availability of MIDI bank support
 *
 * @param handle     Handle to the native player
 * @param supported  return of support availability
 * 
 * @retval JAVACALL_OK      MIDI Bank Query support is available
 * @retval JAVACALL_FAIL    NO MIDI Bank Query support is available
 */
javacall_result javacall_media_is_midibank_query_supported(javacall_handle handle,
                                                           long* supported);

/**
 * This function is used to get a list of installed banks. If the custom
 * parameter is true, a list of custom banks is returned. Otherwise, a list of
 * all banks (custom and internal) is returned. This function can be left empty.
 *
 * @param handle    Handle to the native player
 * @param custom    a flag indicating whether to return just custom banks, or 
 *                  all banks.
 * @param banklist  an array which will be filled out with the banklist
 * @param numlist   the length of the array to be filled out, and on return
 *                  contains the number of values written to the array.
 * 
 * @retval JAVACALL_OK      Bank List is available
 * @retval JAVACALL_FAIL    Bank List is NOT available
 */
javacall_result javacall_media_get_midibank_list(javacall_handle handle,
                                                 long custom,
                                                 /*OUT*/ short* banklist,
                                                 /*INOUT*/ long* numlist);


/**
 * Given a bank, program and key, get name of key. This function applies to
 * key-mapped banks (i.e. percussive banks or effect banks) only. If the returned
 * keyname length is 0, then the key is not mapped to a sound. For melodic banks,
 * where each key (=note) produces the same sound at different pitch, this
 * function always returns a 0 length string. For space saving reasons an
 * implementation may return a 0 length string instead of the keyname. This
 * can be left empty.
 *
 * @param handle    Handle to the native player
 * @param bank      The bank to query
 * @param program   The program to query
 * @param key       The key to query
 * @param keyname   The name of the key returned.
 * @param keynameLen    The length of the keyname array, and on return the
 *                      length of the keyname.
 *
 * @retval JAVACALL_OK      Keyname available
 * @retval JAVACALL_FAIL    Keyname not supported
 */
javacall_result javacall_media_get_midibank_key_name(javacall_handle handle,
                                            long bank,
                                            long program,
                                            long key,
                                            /*OUT*/ javacall_ascii_string keyname,
                                            /*INOUT*/ long* keynameLen);

/**
 * Given the bank and program, get name of program. For space-saving reasons
 * a 0 length string may be returned.
 *
 * @param handle    Handle to the native player
 * @param bank      The bank being queried
 * @param program   The program being queried
 * @param progname  The name of the program returned
 * @param prognameLen    The length of the progname array, and on return the 
 *                       length of the progname
 *
 * @retval JAVACALL_OK      Program name available
 * @retval JAVACALL_FAIL    Program name not supported
 */
javacall_result javacall_media_get_midibank_program_name(javacall_handle handle,
                                                long bank,
                                                long program,
                                                /*OUT*/ javacall_ascii_string progname,
                                                /*INOUT*/ long* prognameLen);

/**
 * Given bank, get list of program numbers. If and only if this bank is not
 * installed, an empty array is returned.
 *
 * @param handle    Handle to the native player
 * @param bank      The bank being queried
 * @param proglist  The Program List being returned
 * @param proglistLen     The length of the proglist, and on return the number
 *                        of program numbers in the list
 *
 * @retval JAVACALL_OK     Program list available
 * @retval JAVACALL_FAIL   Program list unsupported
 */
javacall_result javacall_media_get_midibank_program_list(javacall_handle handle,
                                                long bank,
                                                /*OUT*/ javacall_ascii_string proglist,
                                                /*INOUT*/ long* proglistLen);

/**
 * Returns the program assigned to the channel. It represents the current state
 * of the channel. During playbank of the MIDI file, the program may change due
 * to program change events in the MIDI file. The returned array is represented
 * by an array {bank, program}. The support of this function is optional.
 *
 * @param handle    Handle to the native player
 * @param channel   The channel being queried
 * @param prog      The return array (size 2) in the form {bank, program}
 *
 * @retval JAVACALL_OK    Program available
 * @retval JAVACALL_FAIL  Get Program unsupported
 */
javacall_result javacall_media_get_midibank_program(javacall_handle handle,
                                                long channel,
                                                /*OUT*/ long* prog);


/** @} */ 

/**********************************************************************************/

/**
 * @defgroup MediaOptionalTempoControl       Optional TempoControl API
 * @ingroup JSR135
 * 
 * @brief TempoControl controls the tempo, in musical terms, of a song.
 * TempoControl is typically implemented in Players for MIDI media, 
 * i.e. playback of a Standard MIDI File (SMF).
 *  
 * @{
 */

/** 
 * Get media's current playing tempo.
 * 
 * @param handle    Handle to the library
 * @param tempo     Current playing tempo
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_tempo(javacall_handle handle, /*OUT*/ long* tempo);

/**
 * Set media's current playing tempo
 * 
 * @param handle    Handle to the library
 * @param tempo     Tempo to set
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_set_tempo(javacall_handle handle, long tempo);

/** @}*/

/**********************************************************************************/

/**
 * @defgroup MediaOptionalPitchControl       Optional PitchControl API
 * @ingroup JSR135
 * 
 * @brief PitchControl raises or lowers the playback pitch of audio without changing the playback speed.
 * PitchControl can be implemented in Players for MIDI media or sampled audio. 
 * It is not possible to set audible output to an absolute pitch value. 
 * This control raises or lowers pitch relative to the original.
 * 
 * @{
 */

/**
 * Gets the maximum playback pitch raise supported by the Player
 * 
 * @param handle    Handle to the library 
 * @param maxPitch  The maximum pitch raise in "milli-semitones".
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_max_pitch(javacall_handle handle, /*OUT*/ long* maxPitch);

/**
 * Gets the minimum playback pitch raise supported by the Player
 * 
 * @param handle    Handle to the library 
 * @param minPitch  The minimum pitch raise in "milli-semitones"
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_min_pitch(javacall_handle handle, /*OUT*/ long* minPitch);

/**
 * Set media's current playing rate
 * 
 * @param handle    Handle to the library 
 * @param pitch     The number of semi tones to raise the playback pitch. It is specified in "milli-semitones"
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_pitch(javacall_handle handle, long pitch);

/**
 * Get media's current playing rate
 * 
 * @param handle    Handle to the library 
 * @param pitch     The current playback pitch raise in "milli-semitones"
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_pitch(javacall_handle handle, /*OUT*/ long* pitch);

/** @} */ 

/**********************************************************************************/

/**
 * @defgroup MediaOptionalRateControl        Optional RateControl API
 * @ingroup JSR135
 * 
 * @brief RateControl controls the playback rate of a Player.
 * The rate defines the relationship between the Player's media time and its TimeBase. 
 * Rates are specified in "milli- percentage".
 *
 * @{
 */

/**
 * Get maximum rate of media type
 * 
 * @param handle    Handle to the library 
 * @param maxRate   Maximum rate value for this media type
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_FAIL            Fail
 */
javacall_result javacall_media_get_max_rate(javacall_handle handle, /*OUT*/ long* maxRate);

/**
 * Get minimum rate of media type
 * 
 * @param handle    Handle to the library 
 * @param minRate   Minimum rate value for this media type
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_get_min_rate(javacall_handle handle, /*OUT*/ long* minRate);

/**
 * Set media's current playing rate
 * 
 * @param handle    Handle to the library 
 * @param rate      Rate to set
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_rate(javacall_handle handle, long rate);

/**
 * Get media's current playing rate
 * 
 * @param handle    Handle to the library 
 * @param rate      Current playing rate
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_rate(javacall_handle handle, /*OUT*/ long* rate);

/** @}*/

/**********************************************************************************/

/**
 * @defgroup MediaOptionalRecordControl      Optional RecordControl API
 * @ingroup JSR135
 * 
 * @brief RecordControl controls the recording of media from a Player. 
 * RecordControl records what's currently being played by the Player.
 * 
 * @{
 */

/**
 * Specify the maximum size of the recording including any headers.<br>
 * If a size of -1 is passed then the record size limit should be removed.<br>
 * If device don't want to support this feature, just return JAVACALL_FAIL always.
 * 
 * @param handle    Handle to the library 
 * @param size      The maximum size bytes of the recording requested as input parameter.
 *                  The supported maximum size bytes of the recording which is less than or 
 *                  equal to the requested size as output parameter.
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_FAIL            Fail
 * @retval JAVACALL_NOT_IMPLEMENTED Unsupported
 */
javacall_result javacall_media_set_recordsize_limit(javacall_handle handle, /*INOUT*/ long* size);

/**
 * Is this recording transaction is handled by native layer or Java layer?
 * If this API return JAVACALL_OK, Java layer don't try to get a recording data by using 
 * 'javacall_media_get_recorded_data' API. It is totally depends on OEM's implementation.
 * 
 * @param handle    Handle to the library 
 * @param locator   URL locator string for recording data (ex: file:///root/test.wav)
 * @param locatorLength locator string length
 * 
 * @retval JAVACALL_OK      This recording transaction will be handled by native layer
 * @retval JAVACALL_FAIL    This recording transaction should be handled by Java layer
 * @retval JAVACALL_INVALID_ARGUMENT
 *                          The locator string is invalid format. Java will throw the exception.
 */
javacall_result javacall_media_recording_handled_by_native(javacall_handle handle, 
                                                           javacall_const_utf16_string locator,
                                                           long locatorLength);

/**
 * Starts the recording. records all the data of the player ( video / audio )
 * Before this function call, 'javacall_media_recording_handled_by_native' API MUST be called
 * to check about the OEM's way of record handling.
 * Paused recording by 'javacall_media_pause_recording' function can be resumed by 
 * this function.
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_start_recording(javacall_handle handle);

/**
 * Pause the recording. this should enable a future call to javacall_media_start_recording. 
 * Another call to javacall_media_start_recording after pause has been called will result 
 * in recording the new data and concatenating it to the previously recorded data.
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_pause_recording(javacall_handle handle);

/**
 * Stop the recording. Stopped recording can't not be resumed by 
 * 'javacall_media_start_recording' call. This recording session should be
 * restarted from star-up.
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_stop_recording(javacall_handle handle);

/**
 * The recording that has been done so far should be discarded. (deleted)
 * Recording will be stopped by calling 'javacall_media_stop_recording' from JVM
 * before this method is called. 
 * If 'javacall_media_start_recording' is called after this method is called, recording should resume.
 * If the Player that is associated with this RecordControl is closed, 
 * 'javacall_media_reset_recording' will be called implicitly. 
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_reset_recording(javacall_handle handle);

/**
 * The recording should be completed; 
 * this may involve updating the header,flushing buffers and closing the temporary file if it is used
 * by the implementation.
 * 'javacall_media_stop_recording' will be called before this method is called.
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_commit_recording(javacall_handle handle);

/**
 * Get how much data was returned. 
 * This function can be called after a successful call to 'javacall_media_commit_recording'.
 * 
 * @param handle    Handle to the library 
 * @param size      How much data was recorded
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_recorded_data_size(javacall_handle handle, /*OUT*/ long* size);

/**
 * Gets the recorded data. 
 * This function can be called after a successful call to 'javacall_media_commit_recording'.
 * It receives the data recorded from offset till the size.
 * This function can be called multiple times until get all of the recorded data.
 * 
 * @param handle    Handle to the library 
 * @param buffer    Buffer will contains the recorded data
 * @param offset    An offset to the start of the required recorded data
 * @param size      How much data will be copied to buffer
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_recorded_data(javacall_handle handle, 
                                                 /*OUT*/ char* buffer, long offset, long size);

/**
 * Get the current recording data content type mime string length
 *
 * @param handle    Handle to the library 
 * @param length    Length of string
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_record_content_type_length(javacall_handle handle,
                                                              /*OUT*/ int* length);

/**
 * Get the current recording data content type mime string length
 * For example : 'audio/x-wav' for audio recording
 *
 * @param handle                Handle of native player
 * @param contentTypeBuf        Buffer to return content type unicode string
 * @param length                Length of contentTypeBuf as input parameter and 
 *                              length of content type string stored in contentTypeBuf
 *
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_get_record_content_type(javacall_handle handle, 
                                           /*OUT*/ javacall_utf16* contentTypeBuf,
                                           /*INOUT*/ int* length);

/**
 * Close the recording. OEM can delete all resources related with this recording.
 * This function can be called after a successful call to 'javacall_media_commit_recording'.
 * If the Player that is associated with this RecordControl is closed, 
 * 'javacall_media_close_recording' will be called implicitly after 
 * 'javacall_media_reset_recording' called.
 * 
 * @param handle    Handle to the library 
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_close_recording(javacall_handle handle);

/** @} */ 

/**********************************************************************************/

/**
 * @defgroup MediaMVMSupport      Optional MVM Support API
 * @ingroup JSR135
 * 
 * @brief MVM Support API used to control audio and video resources from MVM.
 * 
 * @{
 */

/**
 * This function called by JVM when this player goes to foreground.
 * There is only one foreground midlets but, multiple player can be exists at this midlet.
 * So, there could be multiple players from JVM.
 * Device resource handling policy is not part of Java implementation. It is totally depends on
 * native layer's implementation.
 *
 * Also, this function can be called by JVM after finishing media buffering.
 * Native poring layer can check about the player's foreground / background status from this invocation.
 *
 * @param handle    Handle to the native player
 * @param appID     ID of the application to be foreground
 *
 * @retval JAVACALL_OK      Something happened
 * @retval JAVACALL_FAIL    Nothing happened. JVM ignores this return value now.
 */
javacall_result javacall_media_to_foreground(const javacall_handle handle,
                                             const int appID);

/**
 * This function called by JVM when this player goes to background.
 * There is only one foreground midlets but, multiple player can be exits at this midlets.
 * So, there could be multiple players from JVM.
 * Device resource handling policy is not part of Java implementation. It is totally depends on
 * native layer's implementation.
 *
 * Also, this function can be called by JVM after finishing media buffering.
 * Native poring layer can check about the player's foreground / background status from this invocation.
 *
 * @param handle    Handle to the native player
 * @param appID     ID of the application to be background
 *
 * @retval JAVACALL_OK      Somthing happened
 * @retval JAVACALL_FAIL    Nothing happened. JVM ignores this return value now.
 */
javacall_result javacall_media_to_background(const javacall_handle handle,
                                             const int appID);

/** @} */ 

/**********************************************************************************/

/** @} */ 

#ifdef __cplusplus
}
#endif

#endif 
