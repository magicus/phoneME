/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * - MIDP 2.0 Specification (JSR-118)
 * - MMAPI 1.1 Specification (JSR-135)
 * - JTWI 1.0 Specification
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
#include "javacall_lcd.h"

/** @defgroup Constant Constants
 *  Constant values
 *  @{
 */

/* */
#define JAVACALL_VIDEO_MPEG4_MIME    "video/mp4v-es"
#define JAVACALL_VIDEO_MPEG4_MIME_2  "video/mp4"
#define JAVACALL_VIDEO_3GPP_MIME     "video/3gpp"
#define JAVACALL_VIDEO_3GPP_MIME_2   "video/3gpp2"

#define JAVACALL_AUDIO_MIDI_MIME     "audio/midi"
#define JAVACALL_AUDIO_MIDI_MIME_2   "audio/mid"
#define JAVACALL_AUDIO_SP_MIDI_MIME  "audio/sp-midi"
#define JAVACALL_AUDIO_WAV_MIME      "audio/x-wav"
#define JAVACALL_AUDIO_MP3_MIME      "audio/mpeg"
#define JAVACALL_AUDIO_MP3_MIME_2    "audio/mp3"
#define JAVACALL_AUDIO_AMR_MIME      "audio/amr"
#define JAVACALL_AUDIO_MPEG4_MIME    "audio/mp4a-latm"
#define JAVACALL_AUDIO_MPEG4_MIME_2  "audio/mp4"
#define JAVACALL_AUDIO_AAC_MIME      "audio/aac"
#define JAVACALL_AUDIO_TONE_MIME     "audio/x-tone-seq"
#define JAVACALL_AUDIO_QCELP_MIME    "audio/qcelp"
#define JAVACALL_AUDIO_QCELP_MIME_2  "audio/vnd.qcelp"

#define JAVACALL_IMAGE_JPEG_MIME     "image/jpeg"
#define JAVACALL_IMAGE_PNG_MIME      "image/png"

#define JAVACALL_DEVICE_TONE_MIME    "device://tone"
#define JAVACALL_DEVICE_MIDI_MIME    "device://midi"
#define JAVACALL_CAPTURE_VIDEO_MIME  "capture://video"
#define JAVACALL_CAPTURE_AUDIO_MIME  "capture://audio"

/* Java MMAPI JTS Values */
#define JAVACALL_SET_VOLUME  -8
#define JAVACALL_SILENCE     -1

/**
 * @enum javacall_media_notification_type
 * 
 * @brief Multimedia notification type.
 */
typedef enum {
    /** Posted when a Player has reached the end of the media. */
    JAVACALL_EVENT_MEDIA_END_OF_MEDIA = 1,      
    /** Posted when the duration of a Player is updated. */    
    JAVACALL_EVENT_MEDIA_DURATION_UPDATED, 
    /** Record size limit is reached or no more space is available */
    JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT,
    /** Posted when an error occurs during the recording. */
    JAVACALL_EVENT_MEDIA_RECORD_ERROR,          
    /** Posted when the system or another higher priority application has released 
        an exclusive device which is now available to the Player. */
    JAVACALL_EVENT_MEDIA_DEVICE_AVAILABLE,      
    /** Posted when the system or another higher priority application has temporarily 
        taken control of an exclusive device which was previously available to the Player. */
    JAVACALL_EVENT_MEDIA_DEVICE_UNAVAILABLE,    
    /** Posted when the Player enters into a buffering mode. */
    JAVACALL_EVENT_MEDIA_BUFFERING_STARTED,     
    /** Posted when the Player leaves the buffering mode. */
    JAVACALL_EVENT_MEDIA_BUFFERING_STOPPED,
    /** Posted when the volume changed from external action. */
    JAVACALL_EVENT_MEDIA_VOLUME_CHANGED,
    /** Posted when the blocked snapshot finished */
    JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED,
    /** Posted when an error had occurred. */
    JAVACALL_EVENT_MEDIA_ERROR
} javacall_media_notification_type;

/**
 * @enum javacall_media_type
 * 
 * @brief Multimedia contents type. If you want to add new media types, you have to consult with Sun Microsystems.
 */
typedef enum {
    /** MPEG4 video      */
    JAVACALL_VIDEO_MPEG4 = 1,   
    /** 3GPP video       */
    JAVACALL_VIDEO_3GPP = 2,        
    /** MIDI audio       */
    JAVACALL_AUDIO_MIDI = 3,        
    /** WAV audio        */
    JAVACALL_AUDIO_WAV = 4,         
    /** MP3 audio        */
    JAVACALL_AUDIO_MP3 = 5,         
    /** AMR audio        */
    JAVACALL_AUDIO_AMR = 6,         
    /** MPEG4 audio      */
    JAVACALL_AUDIO_MPEG4 = 7,       
    /** JTS tone         */
    JAVACALL_AUDIO_TONE = 8,        
    /** QCELP audio      */
    JAVACALL_AUDIO_QCELP = 9,       
    /** AAC audio        */
    JAVACALL_AUDIO_AAC = 10,
    /** Audio capture    */
    JAVACALL_CAPTURE_AUDIO = 11,    
    /** Video capture    */
    JAVACALL_CAPTURE_VIDEO = 12,
    /** Interactive MIDI */
    JAVACALL_INTERACTIVE_MIDI = 13, 
    JAVACALL_END_OF_TYPE
} javacall_media_type;

/**
 * @enum javacall_media_mvm_option
 * 
 * @brief MVM related options.
 */
typedef enum {
    /** This player can be played by background. This option specified by JAD property */
    JAVACALL_MEDIA_PLAYABLE_FROM_BACKGROUND = 0x01  
} javacall_media_mvm_option;

/** @} */

/**
 * @defgroup MediaType Types
 * @ingroup JSR135
 * @{
 */

#define JAVACALL_MEDIA_MAX_PROTOCOL_COUNT   5

/**
 * struct javacall_media_caps
 * @brief Multimedia capability of device 
 */
typedef struct {
    /** Mime type string */
    javacall_const_utf8_string mimeType;
    /** Supported protocol count */
    int         protocolCount;  
    /** Supported protocol strings for this Mime type. Can't exceed JAVACALL_MEDIA_MAX_PROTOCOL_COUNT. */
    javacall_const_utf8_string protocols[JAVACALL_MEDIA_MAX_PROTOCOL_COUNT];
} javacall_media_caps;

/** @} */

/**
 * @defgroup MediaNotification Notification API for Multimedia
 * @ingroup JSR135 
 *
 * @brief Multimedia related external events notification
 *
 * @{
 */        

/**
 * Post native media event to Java event handler
 * 
 * @param type          Event type
 * @param playerId      Player ID that came from javacall_media_create function
 * @param data          Data that will be carried with this notification
 *                      - JAVACALL_EVENT_MEDIA_END_OF_MEDIA
 *                          data = Media time when the Player reached end of media and stopped.
 *                      - JAVACALL_EVENT_MEDIA_DURATION_UPDATED
 *                          data = The duration of the media.
 *                      - JAVACALL_EVENT_MEDIA_RECORD_SIZE_LIMIT
 *                          data = The media time when the recording stopped.
 *                      - JAVACALL_EVENT_MEDIA_DEVICE_AVAILABLE
 *                          data = String specifying the name of the device.
 *                      - JAVACALL_EVENT_MEDIA_DEVICE_UNAVAILABLE   
 *                          data = String specifying the name of the device.
 *                      - JAVACALL_EVENT_MEDIA_BUFFERING_STARTED
 *                          data = Designating the media time when the buffering is started.
 *                      - JAVACALL_EVENT_MEDIA_BUFFERING_STOPPED
 *                          data = Designating the media time when the buffering stopped.
 *                      - JAVACALL_EVENT_MEDIA_VOLUME_CHANGED
 *                          data = volume value.
 *                      - JAVACALL_EVENT_MEDIA_SNAPSHOT_FINISHED
 *                          data = None.
 */
void javanotify_on_media_notification(javacall_media_notification_type type, 
                                      javacall_int64 playerId, 
                                      void* data);

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
 * Get multimedia capabilities of the device.
 * This function should return pointer to static array of javacall_media_caps value
 * The last item of javacall_media_caps array should hold NULL mimeType value
 * Java layer will use this NULL value as a end of item mark
 */
const javacall_media_caps* javacall_media_get_caps(void);

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
 * Java MMAPI call this function to create native media handler.
 * This function is called at the first time to initialize native library.
 * You can do your own initialization job from this function.
 * 
 * @param playerId      Unique player object ID for this playing
 *                      This unique ID is generated by Java MMAPI library.
 * @param mime          Mime unicode string
 * @param mimeLength    String length of mimeType
 * @param uri           URI unicode string to media data
 * @param uriLength     String length of URI
 * @param contentLength Content length in bytes
 *                      If Java MMAPI couldn't determine content length, this value should be -1
 * 
 * @return              Handle of native library. if fail return NULL.
 */
javacall_handle javacall_media_create(javacall_int64 playerId, 
                                      const javacall_utf16* mime, 
                                      long mimeLength,
                                      const javacall_utf16* uri, 
                                      long uriLength,
                                      long contentLength);

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
 * Ask to the native layer.
 * Is this protocol handled by native layer or Java layer?
 * If this function return JAVACALL_OK, Java do not call 
 * javacall_media_do_buffering function
 * In this case, native layer should handle all of data gathering by itself
 * 
 * @retval JAVACALL_OK      Yes, this protocol handled by device.
 * @retval JAVACALL_FAIL    No, please handle this protocol from Java.
 */
javacall_result javacall_media_protocol_handled_by_device(javacall_handle handle);

/**
 * Java MMAPI call this function to send media data to this library
 * This function can be called multiple time to send large media data
 * Native library could implement buffering by using any method (for example: file, heap and etc...)
 * And, buffering occurred in sequentially. not randomly.
 * 
 * When there is no more data, Java indicates end of buffering by setting buffer to NULL and length to -1.
 * OEM should care about this case.
 * 
 * @param handle    Handle to the library
 * @param buffer    Media data buffer pointer. Can be NULL at end of buffering
 * @param length    Length of media data. Can be -1 at end of buffering
 * @param offset    Offset. If offset value is 0, it means start of buffering
 *                  It'll be incremented as buffering progress
 *                  You can determine your internal buffer's writting position by using this value
 *                  Can be -1 at end of buffering
 * 
 * @return          If success return 'length of buffered data' else return -1
 */
long javacall_media_do_buffering(javacall_handle handle, 
                                 const void* buffer, long length, long offset);

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
 * @return          If success return time in ms else return -1
 */
long javacall_media_get_time(javacall_handle handle);

/**
 * Seek to specified time.
 * This function can be called during play status or stop status
 * 
 * @param handle    Handle to the library
 * @param ms        Seek position as ms time
 * 
 * @return          If success return time in ms else return -1
 */
long javacall_media_set_time(javacall_handle handle, long ms);
 
/**
 * Get whole media time in ms.
 * This function can be called during play status or stop status.
 * 
 * @param handle    Handle to the library
 * 
 * @return          If success return time in ms else return -1
 */
long javacall_media_get_duration(javacall_handle handle);

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
 *
 * @return          Volume value
 */
long javacall_media_get_volume(javacall_handle handle); 

/**
 * Set audio volume
 * Audio volume range have to be in 0 to 100 inclusive
 * 
 * @param handle    Handle to the library 
 * @param level     Volume value
 * 
 * @return          if success return volume level else return -1
 */
long javacall_media_set_volume(javacall_handle handle, long level);

/**
 * Is audio muted now?
 * 
 * @param handle    Handle to the library 
 * 
 * @retval JAVACALL_TRUE    Now in mute state
 * @retval JAVACALL_FALSE   Now in un-mute state
 */
javacall_bool javacall_media_is_mute(javacall_handle handle);

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
 * play simple tone
 *
 * @param note     the note to be played. From 0 to 127 inclusive.
 *                 The frequency of the note can be calculated from the following formula:
 *                    SEMITONE_CONST = 17.31234049066755 = 1/(ln(2^(1/12)))
 *                    note = ln(freq/8.176)*SEMITONE_CONST
 *                    The musical note A = MIDI note 69 (0x45) = 440 Hz.
 * @param duration the duration of the note in ms 
 * @param volume   volume of this play. From 0 to 100 inclusive.
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail. JVM will raise the media exception.
 */
javacall_result javacall_media_play_tone(long note, long duration, long volume);

/**
 * stop simple tone
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail. JVM will ignore this return value now.
 */
javacall_result javacall_media_stop_tone(void);

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
 * Turn on or off video rendering alpha channel.
 * If this is on OEM native layer video renderer SHOULD use this mask color
 * and draw on only the region that is filled with this color value.
 * 
 * @image html setalpha.png
 * 
 * @param on    Alpha channel is on?
 * @param color Color of alpha channel
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_set_video_alpha(javacall_bool on, javacall_pixel color);

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
                                                    const javacall_utf16* imageType, long length);

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
 * @param framesToSkip  The number of frames to skip from the current position. 
 *                      If framesToSkip is negative, it will seek backward by framesToSkip number of frames.
 * 
 * @retval JAVACALL_OK      Success
 * @retval JAVACALL_FAIL    Fail
 */
javacall_result javacall_media_skip_frames(javacall_handle handle, long framesToSkip);

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
                                                long index, long bufLength, /*OUT*/ char* keyBuf);

/**
 * Get meta data value strings by using meta data key string
 * 
 * @param handle    Handle to the library 
 * @param key       Meta data key string
 * @param bufLength dataBuf buffer's size in bytes. 
 * @param dataBuf   Buffer that used to return meta data strings. 
 *                  NULL value should be appended to the end of string.
 * 
 * @retval JAVACALL_OK              Success
 * @retval JAVACALL_OUT_OF_MEMORY   dataBuf size is too small
 * @retval JAVACALL_FAIL            Fail
 */
javacall_result javacall_media_get_metadata(javacall_handle handle, 
                                            const char* key, long bufLength, /*OUT*/ char* dataBuf);

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
                                             /*OUT*/short* banklist,
                                             /*INOUT*/long* numlist);


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
                                            /*OUT*/char* keyname,
                                            /*INOUT*/long* keynameLen);

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
                                                /*OUT*/char* progname,
                                                /*INOUT*/long* prognameLen);

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
                                                /*OUT*/char* proglist,
                                                /*INOUT*/long* proglistLen);

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
                                                /*OUT*/long* prog);


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
 * Query if recording is supported based on the player's content-type
 * 
 * @param handle  Handle to the library 
 * 
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
 */
javacall_result javacall_media_supports_recording(javacall_handle handle);

/**
 * Is javacall_media_set_recordsize_limit function is working for this player?
 * In other words - set recording size limit function is working for this player?
 * 
 * @retval JAVACALL_TRUE    Yes. Supported.
 * @retval JAVACALL_FALSE   No. Not supported.
 */
javacall_bool javacall_media_set_recordsize_limit_supported(javacall_handle handle);

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
 * @retval JAVACALL_OK          Success
 * @retval JAVACALL_FAIL        Fail
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
                                                           const javacall_utf16* locator,
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
 * in recording the new data and concatanating it to the previously recorded data.
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
 * @return  If success return length of string else return 0
 */
int javacall_media_get_record_content_type_length(javacall_handle handle);

/**
 * Get the current recording data content type mime string length
 * For example : 'audio/x-wav' for audio recording
 *
 * @param handle                Handle of native player
 * @param contentTypeBuf        Buffer to return content type unicode string
 * @param contentTypeBufLength  Length of contentTypeBuf buffer (in unicode metrics)
 *
 * @return  Length of content type string stored in contentTypeBuf
 */
int javacall_media_get_record_content_type(javacall_handle handle, 
                                           /*OUT*/ javacall_utf16* contentTypeBuf,
                                           int contentTypeBufLength);

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
 * \image html mvmsupport.png
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
 * @param option    MVM options. Check about javacall_media_mvm_option type definition.
 * 
 * @retval JAVACALL_OK      Something happened
 * @retval JAVACALL_FAIL    Nothing happened. JVM ignore this return value now.
 */
javacall_result javacall_media_to_foreground(javacall_handle handle,
                                             javacall_media_mvm_option option);

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
 * @param option    MVM options. Check about javacall_media_mvm_option type definition.
 * 
 * @retval JAVACALL_OK      Something happened
 * @retval JAVACALL_FAIL    Nothing happened. JVM ignore this return value now.
 */
javacall_result javacall_media_to_background(javacall_handle handle,
                                             javacall_media_mvm_option option);

/** @} */ 

/**********************************************************************************/

/** @} */ 







#ifdef __cplusplus
}
#endif

#endif 
