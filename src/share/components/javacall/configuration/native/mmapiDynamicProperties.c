/*
 *    
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

#include <string.h>
#include <javacall_defs.h>
#include <javacall_multimedia.h>

static javacall_media_configuration *configuration;
static javacall_bool configuration_filled = JAVACALL_FALSE;

void mmapi_string_delete_duplicates(char *p);

javacall_media_configuration *get_configuration() {
    if (JAVACALL_FALSE == configuration_filled) {
        if (JAVACALL_OK == javacall_media_get_configuration(&configuration)) {
            configuration_filled = JAVACALL_TRUE;
            return configuration;
        } else {
            return NULL;
        }
    }
    return configuration;
}


const char* get_system_property_supports_mixing()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        if (JAVACALL_TRUE == cfg->supportMixing) {
            return "true";
        } else {
            return "false";
        }
    }
    return "";
}

const char* get_system_property_supports_audio_capture()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        if (NULL != cfg->audioEncoding) {
            return "true";
        } else {
            return "false";
        }
    }
    return "";
}

const char* get_system_property_supports_video_capture()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        if (NULL != cfg->videoEncoding) {
            return "true";
        } else {
            return "false";
        }
    }
    return "";
}

const char* get_system_property_supports_recording()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        if (JAVACALL_TRUE == cfg->supportRecording) {
            return "true";
        } else {
            return "false";
        }
    }
    return "";
}

const char* get_system_property_audio_encodings()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        return cfg->audioEncoding;
    }
    return NULL;
}

const char* get_system_property_video_encodings()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        return cfg->videoEncoding;
    }
    return NULL;
}

const char* get_system_property_video_snapshot_encodings()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        return cfg->videoSnapshotEncoding;
    }
    return NULL;
}

#define MAX_STREAMABLE_CONTENT_PROPERTY_LEN 1024
static char streamable_content_property[MAX_STREAMABLE_CONTENT_PROPERTY_LEN] = {0};
static javacall_bool streamable_content_property_filled = JAVACALL_FALSE;

const char* get_system_property_streamable_contents()
{
    javacall_media_configuration *cfg = get_configuration();
    javacall_media_caps* mediaCaps = NULL;
    char *p=streamable_content_property;
    int types_len;
    
    if (cfg != NULL && streamable_content_property_filled == JAVACALL_FALSE) {
        mediaCaps = cfg->mediaCaps;
        while (mediaCaps != NULL && mediaCaps->mediaFormat != NULL) {
            /* verify if support streaming from memory */
            if (mediaCaps->streamingProtocols & JAVACALL_MEDIA_MEMORY_PROTOCOL) {
                /* add content Types to property string */
                types_len = strlen(mediaCaps->contentTypes);
                if (((int)(p-streamable_content_property)+types_len+1) >=
                    MAX_STREAMABLE_CONTENT_PROPERTY_LEN) {
                    /* delete duplicates and try again */
                    
                    mmapi_string_delete_duplicates(streamable_content_property);
                    p = streamable_content_property + strlen(streamable_content_property);

                    if (((int)(p-streamable_content_property)+types_len+1) >=
                        MAX_STREAMABLE_CONTENT_PROPERTY_LEN) {
                        
                        mediaCaps++;
                        continue;
                    }
                }
                memcpy(p, mediaCaps->contentTypes, types_len);
                p += types_len;
                *p++ = ' ';
            }
            mediaCaps++;
        }
        if (p != streamable_content_property) {
            p--; *p = '\0'; /* replace last space with zero */
            /* delete duplicates */
            mmapi_string_delete_duplicates(streamable_content_property);
        }
        streamable_content_property_filled = JAVACALL_TRUE;
    }
    return streamable_content_property;
}
