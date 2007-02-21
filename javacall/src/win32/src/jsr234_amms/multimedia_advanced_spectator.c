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
#include <memory.h>

#include "javacall_multimedia_advanced.h"
#include "multimedia_advanced.h"

static javacall_audio3d_spectator_t impl;

javacall_audio3d_spectator_t impl = 
{
    { 0,0,0 }, { 0,0,0, {0,0,-1000,0,1000,0}},
    {
        { javacall_audio3d_eLocationControl, &impl.location}, 
        { javacall_audio3d_eOrientationControl, &impl.orient}
    },
    { impl.controls, 
      sizeof( impl.controls ) / sizeof( impl.controls[0] ) }
};


void initSpectatorImpl( javacall_audio3d_spectator_t *spectator )
{
    memcpy( spectator, &impl, sizeof( impl ) );
}

javacall_amms_control_t* javacall_audio3d_spectator_get_control (
    javacall_audio3d_spectator_t *manager,
    javacall_amms_control_type_enum_t type )
{
    return controllable_get_control( &manager->controllable, type );
}

javacall_amms_control_t* javacall_audio3d_spectator_get_controls (
    javacall_audio3d_spectator_t *manager,
    /*OUT*/ int *controls_number )
{
    return controllable_get_controls( &manager->controllable, controls_number );
}


javacall_result javacall_audio3d_orientation_control_set_orientation (
        javacall_audio3d_orientation_control_t *orientation_control,
                       int heading,
                       int pitch,
                       int roll )
{
    if( orientation_control == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    orientation_control->heading = heading;
    orientation_control->pitch = pitch;
    orientation_control->roll = roll;
    printf( 
"Spectator orientation was changed. Heading: %d, pitch: %d, roll: %d\n",
        heading, pitch, roll );

    return JAVACALL_OK;
}

javacall_result javacall_audio3d_orientation_control_set_orientation_vec (
        javacall_audio3d_orientation_control_t *orientation_control,
                       const int frontVector[3],
                       const int aboveVector[3])
{
    if( orientation_control == NULL )
    {
        return JAVACALL_INVALID_ARGUMENT;
    }
    orientation_control->orientation_vectors[0] = frontVector[0];
    orientation_control->orientation_vectors[1] = frontVector[1];
    orientation_control->orientation_vectors[2] = frontVector[2];
    orientation_control->orientation_vectors[3] = aboveVector[0];
    orientation_control->orientation_vectors[4] = aboveVector[1];
    orientation_control->orientation_vectors[5] = aboveVector[2];
    printf( 
"Spectator orientation vectors were changed. front vector: (%d,%d,%d), above \
vector: (%d,%d,%d)\n", 
        frontVector[0], frontVector[1], frontVector[2], 
        aboveVector[0], aboveVector[1], aboveVector[2] ); 
    return JAVACALL_OK;
}

const int* javacall_audio3d_orientation_control_get_orientation_vectors (
    javacall_audio3d_orientation_control_t *orientation_control )
{
    if( orientation_control == NULL )
    {
        return NULL;
    }
    return orientation_control->orientation_vectors;
}
