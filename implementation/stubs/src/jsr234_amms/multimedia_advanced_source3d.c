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

#include <stdlib.h>
#include <malloc.h>
#include <limits.h>
#include <memory.h>
#include <stdio.h>

#include "javacall_multimedia_advanced.h"

#include "multimedia_advanced.h"

struct tag_javacall_audio3d_distance_attenuation_control
{
    int minDistance;
    int maxDistance;
    int rolloffFactor;
    javacall_bool muteAfterMax;
};

struct tag_javacall_audio3d_soundsource3d
{
    javacall_audio3d_location_control_t location;
    javacall_audio3d_distance_attenuation_control_t dist_att;
    javacall_amms_control_t controls[2];
    controllable_t controllable;
};

javacall_result javacall_audio3d_location_control_set_cartesian (
    javacall_audio3d_location_control_t *location_control,
                     int x,
                     int y,
                     int z)
{
    if( location_control == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }

    location_control->coord[1] = x;
    location_control->coord[2] = y;
    location_control->coord[3] = z;
    printf( "Location was changed to (%d, %d, %d)\n", x,y,z );

    return JAVACALL_OK;
}

javacall_result javacall_audio3d_location_control_set_spherical (
    javacall_audio3d_location_control_t *location_control,
                     int azimuth,
                     int elevation,
                     int radius)
{
    if( location_control == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }

    location_control->coord[1] = azimuth;
    location_control->coord[2] = elevation;
    location_control->coord[3] = radius;
    printf( 
"Location was changed to azimuth: %d, elevation: %d, radius: %d\n",
        azimuth, elevation, radius );

    return JAVACALL_OK;
}

const int* javacall_audio3d_location_control_get_cartesian (
    javacall_audio3d_location_control_t *location_control )
{
    if( location_control == NULL )
    {
        return NULL;
    }
    return location_control->coord;
}

javacall_result javacall_audio3d_distance_attenuation_control_set_parameters (
    javacall_audio3d_distance_attenuation_control_t *dac,
                      int minDistance,
                      int maxDistance,
                      javacall_bool isMuteAfterMax,
                      int rolloffFactor)
{
    if( dac == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    dac->minDistance = minDistance;
    dac->maxDistance = maxDistance;
    dac->muteAfterMax = isMuteAfterMax;
    dac->rolloffFactor = rolloffFactor;
    printf( 
"3D Sound Source Distance Attenuation parameters were changed.\
Min. Distance: %d, Max. Distance: %d, Mute After Max: %d, Rolloff Factor: %d\n",
        minDistance, maxDistance, isMuteAfterMax, rolloffFactor );

    return JAVACALL_OK;
}


int javacall_audio3d_distance_attenuation_control_get_min_distance (
        javacall_audio3d_distance_attenuation_control_t *dac )
{
    if( dac == NULL )
    {
        return -1;
    }
    return dac->minDistance;
}

int javacall_audio3d_distance_attenuation_control_get_max_distance (
        javacall_audio3d_distance_attenuation_control_t *dac )
{
    if( dac == NULL )
    {
        return -1;
    }
    return dac->maxDistance;
}

javacall_bool javacall_audio3d_distance_attenuation_control_is_mute_after_max (
        javacall_audio3d_distance_attenuation_control_t *dac )
{
    if( dac == NULL )
    {
        return -2;
    }
    return dac->muteAfterMax;
}

int javacall_audio3d_distance_attenuation_control_get_rolloff_factor (
        javacall_audio3d_distance_attenuation_control_t *dac )
{
    if( dac == NULL )
    {
        return -1;
    }
    return dac->rolloffFactor;
}

javacall_result javacall_audio3d_soundsource3d_add_player ( 
    javacall_audio3d_soundsource3d_t *module, 
    javacall_handle handle )
{
    printf( 
        "The player %x was added to the 3D sound source %x\n",
        handle, module );
    return JAVACALL_OK;
}

javacall_result javacall_audio3d_soundsource3d_remove_player ( 
    javacall_audio3d_soundsource3d_t *module, 
    javacall_handle handle )
{
    printf( 
        "The player %x was removed from the 3D sound source %x\n", 
        handle, module );
    return JAVACALL_OK;
}

javacall_result javacall_audio3d_soundsource3d_add_midi_channel ( 
    javacall_audio3d_soundsource3d_t *module, 
    javacall_handle handle, int channel )
{
    printf( 
"The channel %d of the player %x was added to the 3D sound source %x\n",
        channel, handle, module );
    return JAVACALL_OK;
}

javacall_result javacall_audio3d_soundsource3d_remove_midi_channel ( 
    javacall_audio3d_soundsource3d_t *module, 
    javacall_handle handle, int channel )
{
    printf( 
"The channel %d of the player %x was removed from the 3D sound source %x\n", 
        channel, handle, module );
    return JAVACALL_OK;
}

javacall_amms_control_t* javacall_audio3d_soundsource3d_get_control (
    javacall_audio3d_soundsource3d_t *source,
    javacall_amms_control_type_enum_t type )
{
    return controllable_get_control( &source->controllable, type );
}

javacall_amms_control_t* javacall_audio3d_soundsource3d_get_controls (
    javacall_audio3d_soundsource3d_t *source,
    /*OUT*/ int *controls_number )
{
    return controllable_get_controls( &source->controllable, controls_number );
}

javacall_result createSoundSource3D( 
                    /*OUT*/ javacall_audio3d_soundsource3d_t** source )
{

    javacall_audio3d_soundsource3d_t *ptr = 
        malloc( sizeof( javacall_audio3d_soundsource3d_t ) );

    if( ptr == NULL )
    {
        return JAVACALL_OUT_OF_MEMORY;
    }

    memset( ptr, 0, sizeof( *ptr ) );
    ptr->location.coord[0] = ptr->location.coord[1] = ptr->location.coord[2] = 0;
    ptr->dist_att.minDistance = 1000;
    ptr->dist_att.maxDistance = INT_MAX;
    ptr->dist_att.muteAfterMax = JAVACALL_TRUE;
    ptr->dist_att.rolloffFactor = 1000;
    ptr->controls[0].type = javacall_audio3d_eLocationControl;
    ptr->controls[0].ptr = &ptr->location; 
    ptr->controls[1].type = javacall_audio3d_eDistanceAttenuationControl;
    ptr->controls[1].ptr = &ptr->dist_att;
    ptr->controllable.controls = ptr->controls;
    ptr->controllable.number_of_controls = 
        sizeof( ptr->controls ) / sizeof( ptr->controls[0] );

    *source = ptr;
    return JAVACALL_OK;
}

javacall_result destroySoundSource3D( 
                    javacall_audio3d_soundsource3d_t* source )
{
    if( source == NULL )
    {
        return JAVACALL_FAIL;
    }
    free( source );
    return JAVACALL_OK;
}

