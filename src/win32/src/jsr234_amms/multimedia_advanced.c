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
#include <memory.h>

#include "javacall_multimedia_advanced.h"

#include "multimedia_advanced.h"

extern void initReverbContol( javacall_music_reverb_control_t *reverb );
extern void initSpectatorImpl( javacall_audio3d_spectator_t *spectator );
extern javacall_result createSoundSource3D( 
                    /*OUT*/ javacall_audio3d_soundsource3d_t** source );
extern javacall_result destroySoundSource3D( 
                    javacall_audio3d_soundsource3d_t* source );

static const javacall_media_type gSupported3DMedia[] = { JAVACALL_AUDIO_WAV };


struct tag_javacall_amms_local_manager
{
    controllable_t controllable;
    javacall_music_reverb_control_t reverb;
    javacall_amms_control_t controls[1];
    javacall_audio3d_spectator_t spectator;
};


javacall_amms_control_t* controllable_get_control (
    controllable_t *controllable,
    javacall_amms_control_type_enum_t type )
{
    int i = 0;
    if( controllable == NULL )
    {
        return NULL;
    }
    if( controllable->controls == NULL || controllable->number_of_controls < 1 )
    {
        return NULL;
    }

    for( i = 0; i < controllable->number_of_controls; i++ )
    {
        if( type == controllable->controls[i].type )
        {
            return &controllable->controls[i];
        }
    }

    return NULL;
}

javacall_amms_control_t* controllable_get_controls (
    controllable_t *controllable,
    /*OUT*/ int *controls_number )
{
    if( controllable == NULL )
    {
        return NULL;
    }

    *controls_number = controllable->number_of_controls;
    return controllable->controls;
}

/* 
 * function javacall_audio3d_get_spectator() 
 * for details see declaration in javacall_multimedia_advanced.h  
 */
const javacall_media_type*
    javacall_audio3d_get_supported_soundsource3d_player_types( 
        /*OUT*/ int *number_of_types )
{
    if( number_of_types == NULL )
    {
        return NULL;
    }
    *number_of_types = 1;
    return gSupported3DMedia;
}

/* 
 * function javacall_amms_create_local_manager() 
 * for details see declaration in javacall_multimedia_advanced.h  
 */
javacall_result javacall_amms_create_local_manager( 
                   /*OUT*/ javacall_amms_local_manager_t** manager )
{
    javacall_amms_local_manager_t *ptr; 

    if( manager == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }

    ptr = malloc( sizeof( *ptr ) );
    if( ptr == NULL )
    {
        return JAVACALL_OUT_OF_MEMORY;
    }

    memset( ptr, 0, sizeof( *ptr ) );

    initReverbContol( &ptr->reverb );
    initSpectatorImpl( &ptr->spectator );
    ptr->controls[0].type = javacall_music_eReverbControl;
    ptr->controls[0].ptr = &ptr->reverb;
    ptr->controllable.controls = ptr->controls;
    ptr->controllable.number_of_controls = sizeof( ptr->controls ) / 
        sizeof( ptr->controls[0] );
    
    *manager = ptr;

    return JAVACALL_OK;
}

/* 
 * function javacall_amms_destroy_local_manager() 
 * for details see declaration in javacall_multimedia_advanced.h  
 */
javacall_result javacall_amms_destroy_local_manager(
                        javacall_amms_local_manager_t* manager )
{
    if( manager == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    free( manager );
    return JAVACALL_OK;
}


javacall_amms_control_t* javacall_amms_local_manager_get_control (
    javacall_amms_local_manager_t *manager,
    javacall_amms_control_type_enum_t type )
{
    return controllable_get_control( &manager->controllable, type );
}

javacall_amms_control_t* javacall_amms_local_manager_get_controls (
    javacall_amms_local_manager_t *manager,
    /*OUT*/ int *controls_number )
{
    return controllable_get_controls( &manager->controllable, controls_number );
}


/* 
 * function javacall_amms_local_manager_create_sound_source3d() 
 * for details see declaration in javacall_multimedia_advanced.h  
 */
javacall_result javacall_amms_local_manager_create_sound_source3d(
                    javacall_amms_local_manager_t* manager,
                    /*OUT*/ javacall_audio3d_soundsource3d_t** source )
{
    return createSoundSource3D( source );
}

/* 
 * function javacall_amms_local_manager_destroy_sound_source3d() 
 * for details see declaration in javacall_multimedia_advanced.h  
 */
javacall_result javacall_amms_local_manager_destroy_sound_source3d( 
                    javacall_amms_local_manager_t* manager,
                    javacall_audio3d_soundsource3d_t* source )
{
    return destroySoundSource3D( source );
}

/* 
 * function javacall_amms_local_manager_get_spectator() 
 * for details see declaration in javacall_multimedia_advanced.h  
 */
javacall_audio3d_spectator_t* javacall_amms_local_manager_get_spectator(
                    javacall_amms_local_manager_t* manager )
{
    if( manager == NULL )
    {
        return NULL;
    }
    return &manager->spectator;
}


