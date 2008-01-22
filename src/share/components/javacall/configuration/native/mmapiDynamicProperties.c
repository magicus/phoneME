/*
 *    
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

#include <javacall_defs.h>
#include <javacall_multimedia.h>

static const javacall_media_configuration *configuration;
static javacall_bool configuration_filled = JAVACALL_FALSE;

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
        if (NULL != cfg->audioEncoding) {
            return cfg->audioEncoding;
        }
    }
    return "";
}

const char* get_system_property_video_encodings()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        if (NULL != cfg->videoEncoding) {
            return cfg->videoEncoding;
        }
    }
    return "";
}

const char* get_system_property_video_snapshot_encodings()
{
    javacall_media_configuration *cfg = get_configuration();
    if (cfg != NULL) {
        if (NULL != cfg->videoSnapshotEncoding) {
            return cfg->videoSnapshotEncoding;
        }
    }
    return "";
}

const char* get_system_property_streamable_contents()
{
    return "";
}
