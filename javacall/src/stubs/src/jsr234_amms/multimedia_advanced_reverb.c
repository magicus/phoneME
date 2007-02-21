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

#include <stdio.h>
#include <string.h>

#include "javacall_multimedia_advanced.h"
#include "multimedia_advanced.h"

static const javacall_utf16 *presets[] = 
{
    /* 0  */ L"alley",
    /* 1  */ L"arena",
    /* 2  */ L"auditorium",
    /* 3  */ L"bathroom",
    /* 4  */ L"cave",
    /* 5  */ L"hallway",
    /* 6  */ L"hangar",
    /* 7  */ L"livingroom",
    /* 8  */ L"mountains",
    /* 9  */ L"room",
    /* 10 */ L"underwater",
    /* 11 */ L"smallroom",
    /* 12 */ L"mediumroom",
    /* 13 */ L"largeroom",
    /* 14 */ L"mediumhall",
    /* 15 */ L"largehall",
    /* 16 */ L"plate"
};

int javacall_music_reverb_control_get_reverb_level (
    javacall_music_reverb_control_t *reverb_control )
{
    return ( reverb_control == NULL ) ? 1 : /* normally reverb level is negative */ 
        reverb_control->level;
}

javacall_result javacall_music_reverb_control_set_reverb_level (
    javacall_music_reverb_control_t *reverb_control,
    int level )
{
    if( reverb_control == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    reverb_control->level = level;
    printf( "Reverb level was set to: %d\n", level );
    return JAVACALL_OK;
}

int javacall_music_reverb_control_get_reverb_time (
    javacall_music_reverb_control_t *reverb_control )
{
    return ( reverb_control == NULL ) ? -1 : reverb_control->time;
}

javacall_result javacall_music_reverb_control_set_reverb_time (
    javacall_music_reverb_control_t *reverb_control,
    int time )
{
    if( reverb_control == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    reverb_control->time = time;
    printf( "Reverb time was set to: %d\n", time );
    return JAVACALL_OK;
}

const javacall_utf16* javacall_music_reverb_control_get_preset (
    javacall_music_reverb_control_t *reverb )
{

    if ( reverb == NULL )
    {
        return NULL;
    }

    return reverb->preset;

}

const javacall_utf16 **javacall_music_reverb_control_get_preset_names (
    javacall_music_reverb_control_t *reverb,
    /*OUT*/ int *number_of_presets )
{
    if( number_of_presets == NULL )
    {
        return NULL;
    }

    *number_of_presets = sizeof( presets ) / sizeof( presets[0] );
    return presets;
}


javacall_result javacall_music_reverb_control_set_preset (
    javacall_music_reverb_control_t *reverb,
    const javacall_utf16* preset_name )
{
    int i;
    if( preset_name == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }

    if( reverb == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }

    for( i = 0; i < sizeof( presets ) / sizeof( presets[0] ); i++ )
    {
        if( wcscmp( preset_name, presets[i] ) == 0 )
        {
            reverb->preset = presets[i];
            printf( "The Reverb preset %S was set\n", preset_name );
            return JAVACALL_OK;
        }
    }

    return JAVACALL_INVALID_ARGUMENT;
}

javacall_amms_effect_control_scope_enum_t javacall_music_reverb_control_get_scope (
    javacall_music_reverb_control_t *reverb )
{
    return ( reverb == NULL ) ? -1 : 
      reverb->scope;
}


javacall_result javacall_music_reverb_control_set_scope (
    javacall_music_reverb_control_t *reverb,
    javacall_amms_effect_control_scope_enum_t scope )
{
    if( reverb == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    if( scope != javacall_amms_eSCOPE_LIVE_ONLY &&
        scope != javacall_amms_eSCOPE_RECORD_ONLY &&
        scope != javacall_amms_eSCOPE_LIVE_AND_RECORD )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    reverb->scope = scope;
    printf( "Reverb scope was set to %d\n", scope );
    return JAVACALL_OK;
}

javacall_bool javacall_music_reverb_control_is_enabled (
    javacall_music_reverb_control_t *reverb )
{
    if( reverb == NULL )
    {
        return -2;
    }
    
    return reverb->enabled;

}

javacall_result javacall_music_reverb_control_set_enabled (
    javacall_music_reverb_control_t *reverb,
    javacall_bool enabled )
{
    if( reverb == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    if( enabled != JAVACALL_FALSE && enabled != JAVACALL_TRUE )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    reverb->enabled = enabled;
    if( enabled )
    {
        printf( "Reverb was enabled\n" );
    }
    else
    {
        printf( "Reverb was disabled\n" );
    }
    return JAVACALL_OK;

}

javacall_bool javacall_music_reverb_control_is_enforced (
    javacall_music_reverb_control_t *reverb )
{
    if( reverb == NULL )
    {
        return -2;
    }
    
    return reverb->enforced;

}

javacall_result javacall_music_reverb_control_set_enforced (
    javacall_music_reverb_control_t *reverb,
    javacall_bool enforced )
{
    if( reverb == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    if( enforced != JAVACALL_FALSE && enforced != JAVACALL_TRUE )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    reverb->enforced = enforced;
    if( enforced )
    {
        printf( "Reverb was enforced\n" );
    }
    else
    {
        printf( "Reverb was de-enforced\n" );
    }
    return JAVACALL_OK;

}

static javacall_music_reverb_control_t gReverb;

javacall_music_reverb_control_t gReverb = 
{ 0, 1, JAVACALL_FALSE, JAVACALL_FALSE, javacall_amms_eSCOPE_LIVE_ONLY, NULL,
};


void initReverbContol( javacall_music_reverb_control_t *reverb )
{
    memcpy( reverb, &gReverb, sizeof( gReverb ) );
}
