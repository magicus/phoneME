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
#ifndef __JAVACALL_MULTIMEDIA_ADVANCED_H
#define __JAVACALL_MULTIMEDIA_ADVANCED_H
/**
 * @file javacall_multimedia_advanced.h
 * @ingroup JSR234
 * @brief Javacall interfaces for JSR-234, Advanced Multimedia Supplements
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_multimedia.h" 

/** 
 * @defgroup JSR234 JSR234 Advanced Multimedia Supplements (AMMS) API
 *
 * <H2>Introduction</H2>
 * Advanced Multimedia Supplements (AMMS, JSR-234) is based on the
 * Mobile Media API (MMAPI, JSR-135). AMMS introduces a lot of new Controls
 * and other features.
 *
 * @{
 */

/**
 * @defgroup jsrMandatoryJSR234Common The part common for all JSR-234 features
 * @ingroup JSR234
 * @{
 */

/**
 * @enum javacall_amms_control_type_enum_t
 * @brief AMMS Control types
 */
typedef enum 
{
    /** This type value means that the Control is LocationControl,
      * see JSR-234 Spec 
      * @see javacall_audio3d_location_control_t
      */
    javacall_audio3d_eLocationControl,
    /** This type value means that the Control is OrientationControl,
      * see JSR-234 Spec
      * @see javacall_audio3d_orientation_control_t
      */
    javacall_audio3d_eOrientationControl,
    /** This type value means that the Control is DistanceAttenuationControl
      * see JSR-234 Spec
      * @see javacall_audio3d_distance_attenuation_control_t
      */
    javacall_audio3d_eDistanceAttenuationControl,
    /** This type value means that the Control is ReverbControl,
      * see JSR-234 Spec
      * @see javacall_music_reverb_control_t
      */
    javacall_music_eReverbControl
} javacall_amms_control_type_enum_t;

    
/**
 * struct javacall_amms_control_t
 * @brief Structure corresponding to the Control interface
 * of MMAPI Java API, see JSR-135 Spec
 */
typedef struct 
{
    /** the type of the control 
      * @see  javacall_amms_control_type_enum_t 
      */
    javacall_amms_control_type_enum_t type;
    /** pointer to the specific control structure according to its type 
      * @see  javacall_amms_control_type_enum_t 
      */
    void *ptr;
} javacall_amms_control_t;

/**
 * @enum javacall_amms_effect_control_scope_enum_t
 * @brief   possible Scope values for EffectControl,
 *          for example ReverbControl (javacall_music_reverb_control_t),
 *          see JSR-234 Spec
 */
typedef enum
{
    /**
      * Corresponds to EffectControl.SCOPE_LIVE_ONLY constant
      * in AMMS Java API, see JSR-234 Spec
      */
    javacall_amms_eSCOPE_LIVE_ONLY,
    /**
      * Corresponds to EffectControl.SCOPE_RECORD_ONLY constant
      * in AMMS Java API, see JSR-234 Spec
      */
    javacall_amms_eSCOPE_RECORD_ONLY,
    /**
      * Corresponds to EffectControl.SCOPE_LIVE_AND_RECORD constant
      * in AMMS Java API, see JSR-234 Spec
      */
    javacall_amms_eSCOPE_LIVE_AND_RECORD
} javacall_amms_effect_control_scope_enum_t;


/**
  * @typedef javacall_amms_local_manager_t
  * @brief Type corresponding to the class GlobalManager in the Java API
  * to a certain extent (see JSR-234 Spec). The difference is that there can
  * be multiple Local Managers if several isolated Java applications are
  * run concurrently.
  *
  */
typedef struct tag_javacall_amms_local_manager
    javacall_amms_local_manager_t;  

/**
  * To create a new Local Manager
  * 
  * @param manager                   pointer to return the address of the new
  *                                  Local Manager
  *
  * @retval JAVACALL_OK              Success
  * @retval JAVACALL_FAIL            Fail
  * @retval JAVACALL_OUT_OF_MEMORY   if there was not enough memory to 
  *                                  create new Local Manager
  * @see javacall_result
  * @see javacall_amms_local_manager_t
  */
javacall_result javacall_amms_create_local_manager( 
                   /*OUT*/ javacall_amms_local_manager_t **manager );

/**
  * To destroy a Local Manager
  * 
  * @param manager                   pointer to the Local Manager
  *
  * @retval JAVACALL_OK              Success
  * @retval JAVACALL_FAIL            Fail
  *
  * @see javacall_result
  * @see javacall_amms_local_manager_t
  */
javacall_result javacall_amms_destroy_local_manager(
                        javacall_amms_local_manager_t *manager );
    
/**
  * The function corresponding to GlobalManager.getControl()
  * method of MMAPI, see JSR-135 Spec
  *
  * Return a "local" Control which scope is the current MIDP application.
  * (but not any other MIDP applications run concurrently)
  *
  * @param manager     pointer to the local manager.
  * @param type     the type of the requested Control (from the enum)
  * 
  * @return pointer to the requested Control structure or NULL if this
  *         type of control is not supported
  * @see javacall_amms_controllable_t
  */
javacall_amms_control_t* javacall_amms_local_manager_get_control (
    javacall_amms_local_manager_t *manager,
    javacall_amms_control_type_enum_t type );

/**
  * The function corresponding to GlobalManager.getControls()
  * method of MMAPI, see JSR-135 Spec
  *
  * Obtain the array of the supported "local" Controls common for the 
  *  current MIDP application
  * (but not any other MIDP applications run concurrently)
  * 
  * @param manager     pointer to the local manager.
  * @param controls_number  pointer to return the returned array length
  * 
  * @return array of the supported Controls, which length is return through
  *         the controls_number parameter. The array of Control's 
  *         returned will not contain any duplicates. And the list will
  *         not change over time. If no Control is supported, NULL is 
  *         returned.
  * @see javacall_amms_local_manager_t
  */
javacall_amms_control_t* javacall_amms_local_manager_get_controls (
    javacall_amms_local_manager_t *manager,
    /*OUT*/ int *controls_number );

/** @} */

/**
 * @defgroup jsrMandatoryJSR234Audio3D Mandatory part of 3D Audio Capability
 * @ingroup JSR234
 * @{
 */


/**
  * @typedef javacall_audio3d_spectator_t
  * @brief type corresponding to Spectator class of AMMS Java API,
  * see JSR-234 Spec
  */
typedef struct tag_javacall_audio3d_spectator javacall_audio3d_spectator_t;

/**
  * The function corresponding to Spectator.getControl()
  * method of MMAPI, see JSR-135 Spec
  *
  * Return a Control to control the specified Spectator
  *
  * @param spectator     pointer to the spectator.
  * @param type     the type of the requested Control (from the enum)
  * 
  * @return pointer to the requested Control structure or NULL if this
  *         type of control is not supported
  * @see javacall_amms_controllable_t
  */
javacall_amms_control_t* javacall_audio3d_spectator_get_control (
    javacall_audio3d_spectator_t *spectator,
    javacall_amms_control_type_enum_t type );

/**
  * The function corresponding to Spectator.getControls()
  * method of MMAPI, see JSR-135 Spec
  *
  * Obtain the array of the supported Controls which can be used to 
  *  control the specified Spectator
  * 
  * @param spectator     pointer to the spectator.
  * @param controls_number  pointer to return the returned array length
  * 
  * @return array of the supported Controls, which length is return through
  *         the controls_number parameter. The array of Control's 
  *         returned will not contain any duplicates. And the list will
  *         not change over time. If no Control is supported, NULL is 
  *         returned.
  * @see javacall_audio3d_spectator_t
  */
javacall_amms_control_t* javacall_audio3d_spectator_get_controls (
    javacall_audio3d_spectator_t *spectator,
    /*OUT*/ int *controls_number );

/**
  * @typedef javacall_audio3d_soundsource3d_t
  * @brief type corresponding to SoundSource3D interface of AMMS Java API,
  * see JSR-234 Spec
  */
typedef struct tag_javacall_audio3d_soundsource3d 
    javacall_audio3d_soundsource3d_t;

/**
  * The function corresponding to SoundSource3D.addPlayer() method
  * of AMMS Java API, see JSR-234 Spec
  *
  * Add a Player to the SoundSource3D
  * 
  * @param soundsource3d    pointer to the SoundSource3D.
  * @param handle   Handle of native player. Java layer is responsible
  *                 for the following: 
  *                 - neither the player nor any of its channels is already 
  *                 belonging to the SoundSource3D,
  *                 - there is no player currently belonging to the
  *                  SoundSource3D in PREFETCHED or STARTED state,
  *                 - the player to be added is in neither PREFETCHED nor
  *                 STARTED state,
  *                 - the player handle is not NULL
  *                 See JSR-135 Spec for Player states explanation
  *                 @see javacall_media_acquire_device
  *                 @see javacall_media_release_device
  *                 @see javacall_media_start
  *                 @see javacall_media_stop
  * 
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail, 
  * @retval JAVACALL_NOT_IMPLEMENTED    The addition is not supported 
  *                                     by the implementation. (For 
  *                                     example, if the implementation 
  *                                     does not support adding the same 
  *                                     Player to multiple modules or if 
  *                                     the implementation does not 
  *                                     support the type of the Player.) 
  * @retval JAVACALL_INVALID_ARGUMENT   if the player handle is NULL
  * @see javacall_amms_module_t
  */
javacall_result javacall_audio3d_soundsource3d_add_player ( 
    javacall_audio3d_soundsource3d_t *soundsource3d, 
    javacall_handle handle );

/**
  * The function corresponding to SoundSource3D.removePlayer() method
  * of AMMS Java API, see JSR-234 Spec
  *
  * Remove a Player from the SoundSource3D
  * 
  * @param soundsource3d    pointer to the SoundSource3D.
  * @param handle   Handle of native player. Java layer is responsible
  *                 for the following: 
  *                 - the player is currently being a part the SoundSource3D,
  *                 - the SoundSource3D is currently containing no Players in
  *                 PREFETCHED or STARTED state,
  *                 - the Player to be removed is not in PREFETCHED or STARTED
  *                 state,
  *                 - the player handle is not NULL
  *                 See JSR-135 Spec for Player states explanation
  *                 @see javacall_media_acquire_device
  *                 @see javacall_media_release_device
  *                 @see javacall_media_start
  *                 @see javacall_media_stop
  *                 
  * 
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail, 
  * @retval JAVACALL_INVALID_ARGUMENT   if the player handle is NULL
  * @see javacall_audio3d_soundsource3d_t
  */
javacall_result javacall_audio3d_soundsource3d_remove_player ( 
    javacall_audio3d_soundsource3d_t *soundsource3d, 
    javacall_handle handle );

/**
  * The function corresponding to SoundSource3D.addMIDIChannel()
  * method of AMMS Java API, see JSR-234 Spec
  * 
  * Add MIDI channel to the SoundSource3D
  * 
  * @param soundsource3d    pointer to the SoundSource3D.
  * @param handle       Handle of native player. Java layer is responsible
  *                     for the following: 
  *                     - the player is a MIDI player,
  *                     - neither the channel nor the whole player is already a
  *                     part of the SoundSource3D, 
  *                     - the SoundSource3D currently is containing no Player in
  *                     PREFETCHED or STARTED state,
  *                     - the Player to be added is not in PREFETCHED or
  *                     STARTED state,
  *                     - the player is not NULL
  *                     See JSR-135 Spec for Player states explanation
  *                     @see javacall_media_acquire_device
  *                     @see javacall_media_release_device
  *                     @see javacall_media_start
  *                     @see javacall_media_stop
  * @param channel      the channel of the player to be added. The range 
  *                     is 0-15. Java layer is responsible for the 
  *                     following: 
  *                     - neither the channel nor the whole player is already a
  *                     part of the SoundSource3D
  * 
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail, 
  * @retval JAVACALL_INVALID_ARGUMENT   if the player handle is NULL or 
  *                                     the channel is not in 
  *                                     the range 0-15
  * @see javacall_audio3d_soundsource3d_t
  */
javacall_result javacall_audio3d_soundsource3d_add_midi_channel ( 
    javacall_audio3d_soundsource3d_t *soundsource3d, 
    javacall_handle handle, int channel );

/**
  * The function corresponding to SoundSource3D.removeMIDIChannel()
  * method of AMMS Java API, see JSR-234 Spec
  * 
  * Remove MIDI channel from the SoundSource3D
  * 
  * @param soundsource3d    pointer to the SoundSource3D.
  * @param handle       Handle of native player. Java layer is responsible
  *                     for the following: 
  *                     - the player is a MIDI player,
  *                     - the channel is a part of the SoundSource3D, 
  *                     - the SoundSource3D currently is containing no Player
  *                     in PREFETCHED or STARTED state,
  *                     - the Player to be removed is not in PREFETCHED or
  *                     STARTED state,
  *                     - the player handle is not NULL
  *                     See JSR-135 Spec for Player states explanation
  *                     @see javacall_media_acquire_device
  *                     @see javacall_media_release_device
  *                     @see javacall_media_start
  *                     @see javacall_media_stop
  * @param channel      the channel of the player to be added. The range 
  *                     is 0-15. Java layer is responsible for the 
  *                     following: 
  *                     - the channel is a part of the SoundSource3D
  * 
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail, 
  * @retval JAVACALL_INVALID_ARGUMENT   if the player handle is NULL
  * @see javacall_audio3d_soundsource3d_t
  */
javacall_result javacall_audio3d_soundsource3d_remove_midi_channel ( 
    javacall_audio3d_soundsource3d_t *soundsource3d, 
    javacall_handle handle, int channel );

/**
  * The function corresponding to SoundSource3D.getControl()
  * method of MMAPI, see JSR-135 Spec
  *
  * Return a Control to control the specified SoundSource3D
  *
  * @param soundsource3d     pointer to the soundsource3d.
  * @param type     the type of the requested Control (from the enum)
  * 
  * @return pointer to the requested Control structure or NULL if this
  *         type of control is not supported
  * @see javacall_audio3d_soundsource3d_t
  */
javacall_amms_control_t* javacall_audio3d_soundsource3d_get_control (
    javacall_audio3d_soundsource3d_t *soundsource3d,
    javacall_amms_control_type_enum_t type );

/**
  * The function corresponding to SoundSource3D.getControls()
  * method of MMAPI, see JSR-135 Spec
  *
  * Obtain the array of the supported Controls which can be used to 
  *  control the specified SoundSource3D
  * 
  * @param soundsource3d     pointer to the soundsource3d. 
  * @param controls_number  pointer to return the returned array length
  * 
  * @return array of the supported Controls, which length is return through
  *         the controls_number parameter. The array of Control's 
  *         returned will not contain any duplicates. And the list will
  *         not change over time. If no Control is supported, NULL is 
  *         returned.
  * @see javacall_audio3d_soundsource3d_t
  */
javacall_amms_control_t* javacall_audio3d_soundsource3d_get_controls (
    javacall_audio3d_soundsource3d_t *soundsource3d,
    /*OUT*/ int *controls_number );

/** 
  * @typedef javacall_music_reverb_control_t 
  * @brief type corresponding to ReverbControl in AMMS Java API,
  * see JSR-234 Spec
  */
typedef struct tag_javacall_music_reverb_control
                javacall_music_reverb_control_t;

/** 
  * The function corresponding to 
  * ReverbControl.getReverbLevel()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param reverb_control       pointer to the ReverbControl 
  * @return     current level of the reverberation. For detailed
  *             explanation see JSR-234 Spec, ReverbControl explanation
  */
int javacall_music_reverb_control_get_reverb_level (
    javacall_music_reverb_control_t *reverb_control );

/** 
  * The function corresponding to 
  * ReverbControl.setReverbLevel()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param reverb_control       pointer to the ReverbControl 
  * @param level            this parameter has the same meaning as 
  *                         "level" parameter of 
  *                         ReverbControl.setReverbLevel() 
  *                         method of AMMS
  *                         Java API. See JSR-234 Spec
  *                         N.B.: this value should not be positive,
  *                         because it denotes the level in millibels!
  *                         Java layer is responsible for this value
  *                         being less than or equal to zero.
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_INVALID_ARGUMENT   if any parameter value limitation
  *                                     mentioned above is violated by
  *                                     Java layer
  */
javacall_result javacall_music_reverb_control_set_reverb_level (
    javacall_music_reverb_control_t *reverb_control,
    int level );

/** 
  * The function corresponding to 
  * ReverbControl.getReverbTime()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param reverb_control       pointer to the ReverbControl 
  * @return     current reverberation time. For detailed
  *             explanation see JSR-234 Spec, ReverbControl explanation
  */
int javacall_music_reverb_control_get_reverb_time (
    javacall_music_reverb_control_t *reverb_control );

/** 
  * The function corresponding to 
  * ReverbControl.setReverbTime()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param reverb_control       pointer to the ReverbControl 
  * @param time             this parameter has the same meaning as 
  *                         "time" parameter of 
  *                         ReverbControl.setReverbTime() 
  *                         method of AMMS
  *                         Java API. See JSR-234 Spec
  *                         Java layer is responsible for this value
  *                         being greater than or equal to zero.
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_INVALID_ARGUMENT   if any parameter value limitation
  *                                     mentioned above is violated by
  *                                     Java layer
  * @retval JAVACALL_NOT_IMPLEMENTED    if changing the Reverberation Time
  *                                     is not supported by the device
  */
javacall_result javacall_music_reverb_control_set_reverb_time (
    javacall_music_reverb_control_t *reverb_control,
    int time );

/**
  * The function corresponding to ReverbControl.getPreset() 
  * method of the AMMS Java API. See JSR-234 Spec
  * 
  * Get the current preset of the ReverbControl.
  * 
  * @param reverb   pointer to the ReverbControl.
  * @return null-terminated JavaCall-unicode string naming the currently
  *         set preset or NULL if no preset
  *         is set at the moment. For the detailed explanation of presets
  *         see JSR-234 Spec, ReverbControl interface description.
  */
const javacall_utf16* javacall_music_reverb_control_get_preset (
    javacall_music_reverb_control_t *reverb );
/**
  * The function corresponding to 
  * ReverbControl.getPresetNames() method of the AMMS Java API. 
  * See JSR-234 Spec
  * 
  * Get the available preset names.
  * 
  * @param reverb   pointer to the ReverbControl.
  * @param number_of_presets    pointer to return the returned array
  *                             length
  * @return the names of all the available preset modes as an array of
  *         null-terminated JavaCall-unicode strings. The length of the
  *         array is returned by the
  *         "number_of_preset" parameter. For detailed explanation of
  *         presets see JSR-234 Spec, ReverbControl interface description.
  *         
  */
const javacall_utf16 **javacall_music_reverb_control_get_preset_names (
    javacall_music_reverb_control_t *reverb,
    /*OUT*/ int *number_of_presets );

/**
  * The function corresponding to 
  * ReverbControl.setPreset() method of the AMMS Java API. 
  * See JSR-234 Spec
  * 
  * Set the effect according to the given preset
  * 
  * @param reverb   pointer to the ReverbControl.
  * @param preset_name  the preset to be set. A null-terminated
  *                     JavaCall-unicode string.
  *                     For detailed explanation of presets see JSR-234
  *                     Spec, ReverbControl interface description.
  *
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_INVALID_ARGUMENT   if the preset is not available or it is
  *                                     NULL
  */
javacall_result javacall_music_reverb_control_set_preset (
    javacall_music_reverb_control_t *reverb,
    const javacall_utf16* preset_name );

/**
  * The function corresponding to ReverbControl.getScope()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * Returns the current scope of the effect.
  * For detailed explanation of effect scope see JSR-234 Spec,
  * ReverbControl interface description.
  * @see javacall_music_reverb_control_scope_enum_t
  * 
  * @param reverb   pointer to the ReverbControl.
  * @return the current scope of the effect
  * 
  */
javacall_amms_effect_control_scope_enum_t 
    javacall_music_reverb_control_get_scope (
        javacall_music_reverb_control_t *reverb );

/**
  * The function corresponding to ReverbControl.setScope()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * Set the scope of the effect. 
  * For detailed explanation of effect scope see JSR-234 Spec,
  * ReverbControl interface description.
  * @see javacall_amms_effect_control_scope_enum_t
  * 
  * @param reverb   pointer to the ReverbControl.
  * @param scope                the scope to be set for this effect
  *
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_NOT_IMPLEMENTED    if the scope passed is not supported
  * 
  */
javacall_result javacall_music_reverb_control_set_scope (
    javacall_music_reverb_control_t *reverb,
    javacall_amms_effect_control_scope_enum_t scope );

/**
  * The function corresponding to ReverbControl.isEnabled()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * Determine whether the effect is enabled or not.
  * An effect should be enabled in order to take any effect
  * @see javacall_bool
  * 
  * @param reverb   pointer to the ReverbControl.
  * @retval JAVACALL_TRUE       if the effect is enabled
  * @retval JAVACALL_FALSE      if the effect is not enabled
  * 
  */
javacall_bool javacall_music_reverb_control_is_enabled (
    javacall_music_reverb_control_t *reverb );

/**
  * The function corresponding to ReverbControl.setEnabled()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * Enable or disable the effect 
  * (according to the boolean parameter passed)
  * An effect should be enabled in order to take any effect
  * @see javacall_bool
  * 
  * @param reverb   pointer to the ReverbControl.
  * @param enabled              pass JAVACALL_TRUE to enable the effect or
  *                             JAVACALL_FALSE to disable it
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_INVALID_ARGUMENT   if "enabled" parameter is neither
  *                                     JAVACALL_TRUE nor JAVACALL_FALSE
  */
javacall_result javacall_music_reverb_control_set_enabled (
    javacall_music_reverb_control_t *reverb,
    javacall_bool enabled );

/**
  * The function corresponding to ReverbControl.isEnforced()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * Determine whether the effect is enabled or not.
  * For explanation of an effect being enforced
  * see JSR-234 Spec, ReverbControl interface description
  * @see javacall_bool
  * 
  * @param reverb   pointer to the ReverbControl.
  * @retval JAVACALL_TRUE       if the effect is enforced
  * @retval JAVACALL_FALSE      if the effect is not enforced
  * 
  */
javacall_bool javacall_music_reverb_control_is_enforced (
    javacall_music_reverb_control_t *reverb );

/**
  * The function corresponding to ReverbControl.setEnforced()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * Set the effect enforced or not enforced 
  * (according to the boolean parameter passed)
  * For explanation of an effect being enforced
  * see JSR-234 Spec, ReverbControl interface description
  * @see javacall_bool
  * 
  * @param reverb   pointer to the ReverbControl.
  * @param enabled              pass JAVACALL_TRUE to set the effect
  *                             enforced or JAVACALL_FALSE to set it not
  *                             enforced
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_INVALID_ARGUMENT   if "enforced" parameter is neither
  *                                     JAVACALL_TRUE nor JAVACALL_FALSE
  */
javacall_result javacall_music_reverb_control_set_enforced (
    javacall_music_reverb_control_t *reverb,
    javacall_bool enforced );

/** 
  * @typedef javacall_audio3d_distance_attenuation_control_t 
  * @brief type corresponding to DistanceAttenuationControl in AMMS Java API,
  * see JSR-234 Spec
  */
typedef struct tag_javacall_audio3d_distance_attenuation_control
    javacall_audio3d_distance_attenuation_control_t;

/** 
  * The function corresponding to 
  * DistanceAttenuationControl.setParameters()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param dac             pointer to the DistanceAttenuationControl
  * @param minDistance      this parameter has the same meaning as 
  *                         minDistance parameter of 
  *                         DistanceAttenuationControl.setParameters().
  *                         See JSR-234 Spec.
  *                         Java layer is responsible for this parameter
  *                         being greater than zero
  * @param maxDistance      this parameter has the same meaning as 
  *                         maxDistance parameter of 
  *                         DistanceAttenuationControl.setParameters()
  *                         See JSR-234 Spec
  *                         Java layer is responsible for this parameter
  *                         being greater than zero and greater than
  *                         minDistance parameter
  * @param isMuteAfterMax   this parameter has the same meaning as 
  *                         muteAfterMax parameter of 
  *                         DistanceAttenuationControl.setParameters()
  *                         See JSR-234 Spec
  * @param rolloffFactor    this parameter has the same meaning as 
  *                         rolloffFactor parameter of 
  *                         DistanceAttenuationControl.setParameters()
  *                         See JSR-234 Spec
  *                         Java layer is responsible for this parameter
  *                         being greater than or equal to zero.
  * 
  *
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_INVALID_ARGUMENT   if any parameter value limitation
  *                                     mentioned above is violated by
  *                                     Java layer
  */
javacall_result javacall_audio3d_distance_attenuation_control_set_parameters (
    javacall_audio3d_distance_attenuation_control_t *dac,
                      int minDistance,
                      int maxDistance,
                      javacall_bool isMuteAfterMax,
                      int rolloffFactor);

/** 
  * The function corresponding to 
  * DistanceAttenuationControl.getMinDistance()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param dac             pointer to the DistanceAttenuationControl
  * @return     the Minimum Distance currently set for this Control.
  *             For explanation what is Minimum Distance see JSR-234 Spec,
  *             DistanceAttenuationControl description
  */
int javacall_audio3d_distance_attenuation_control_get_min_distance (
        javacall_audio3d_distance_attenuation_control_t *dac );

/** 
  * The function corresponding to 
  * DistanceAttenuationControl.getMaxDistance()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param dac             pointer to the DistanceAttenuationControl
  * @return     the Maximum Distance currently set for this Control.
  *             For explanation what is Maximum Distance see JSR-234 Spec,
  *             DistanceAttenuationControl description
  */
int javacall_audio3d_distance_attenuation_control_get_max_distance (
        javacall_audio3d_distance_attenuation_control_t *dac );

/** 
  * The function corresponding to 
  * DistanceAttenuationControl.getMuteAfterMax()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param dac             pointer to the DistanceAttenuationControl
  * @retval JAVACALL_TRUE   if the Control setting muteAfterMax is on
  *                         For explanation what is muteAfterMax setting
  *                         see JSR-234 Spec, DistanceAttenuationControl
  *                         description.
  * @retval JAVACALL_FALSE  if the Control setting muteAfterMax is off
  *                         For explanation what is muteAfterMax setting
  *                         see JSR-234 Spec, DistanceAttenuationControl
  *                         description.
  */
javacall_bool javacall_audio3d_distance_attenuation_control_is_mute_after_max (
                javacall_audio3d_distance_attenuation_control_t *dac );

/** 
  * The function corresponding to 
  * DistanceAttenuationControl.getRolloffFactor()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param dac             pointer to the DistanceAttenuationControl
  * @return     the Rolloff Factor currently set for this Control.
  *             For explanation what is Rolloff Factor see JSR-234 Spec,
  *             DistanceAttenuationControl description
  */
int javacall_audio3d_distance_attenuation_control_get_rolloff_factor (
        javacall_audio3d_distance_attenuation_control_t *dac );

/** 
  * @typedef javacall_audio3d_location_control_t 
  * @brief type corresponding to LocationControl in AMMS Java API,
  * see JSR-234 Spec
  */
typedef struct tag_javacall_audio3d_location_control
                    javacall_audio3d_location_control_t;

/** 
  * The function corresponding to 
  * LocationControl.setCartesian()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * @param location_control     pointer to the LocationControl 
  * @param x                this parameter has the same meaning as 
  *                         "x" parameter of 
  *                         LocationControl.setCartesian() method of AMMS 
  *                         Java API. See JSR-234 Spec
  * @param y                this parameter has the same meaning as 
  *                         "y" parameter of 
  *                         LocationControl.setCartesian() method of AMMS 
  *                         Java API. See JSR-234 Spec
  * @param z                this parameter has the same meaning as 
  *                         "z" parameter of 
  *                         LocationControl.setCartesian() method of AMMS 
  *                         Java API. See JSR-234 Spec
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  */
javacall_result javacall_audio3d_location_control_set_cartesian (
    javacall_audio3d_location_control_t *location_control,
                     int x,
                     int y,
                     int z);

/** 
  * The function corresponding to 
  * LocationControl.setSpherical()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param location_control     pointer to the LocationControl 
  * @param azimuth          this parameter has the same meaning as 
  *                         "azimuth" parameter of 
  *                         LocationControl.setSpherical() method of AMMS 
  *                         Java API. See JSR-234 Spec
  * @param elevation        this parameter has the same meaning as 
  *                         "elevation" parameter of 
  *                         LocationControl.setSpherical() method of AMMS 
  *                         Java API. See JSR-234 Spec
  * @param radius           this parameter has the same meaning as 
  *                         "radius" parameter of 
  *                         LocationControl.setSpherical() method of AMMS 
  *                         Java API. See JSR-234 Spec
  *                         Java layer is responsible for this parameter
  *                         being greater than or equal to zero.
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_INVALID_ARGUMENT   if any parameter value limitation
  *                                     mentioned above is violated by
  *                                     Java layer
  */
javacall_result javacall_audio3d_location_control_set_spherical (
    javacall_audio3d_location_control_t *location_control,
                     int azimuth,
                     int elevation,
                     int radius);

/** 
  * The function corresponding to 
  * LocationControl.getCartesian()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param location_control     pointer to the LocationControl 
  * @return     an array of 3 consecutive values "x", "y" and "z". For 
  *             explanation of what the values mean see JSR-234,
  *             LocationControl description.
  *             @see setCartesian
  */
const int* javacall_audio3d_location_control_get_cartesian (
    javacall_audio3d_location_control_t *location_control );

/** 
  * @typedef javacall_audio3d_orientation_control_t 
  * @brief type corresponding to OrientationControl in AMMS Java API,
  * see JSR-234 Spec
  */
typedef struct tag_javacall_audio3d_orientation_control
            javacall_audio3d_orientation_control_t;

/** 
  * The function corresponding to 
  * OrientationControl.setOrientation(int, int, int)
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param orientation_control    pointer to the OrientationControl 
  * @param heading          this parameter has the same meaning as 
  *                         "heading" parameter of 
  *                         OrientationControl.setOrientation(int, int, 
  *                         int) method of AMMS 
  *                         Java API. See JSR-234 Spec
  * @param pitch            this parameter has the same meaning as 
  *                         "pitch" parameter of 
  *                         OrientationControl.setOrientation(int, int,
  *                         int) method of AMMS 
  *                         Java API. See JSR-234 Spec
  * @param roll             this parameter has the same meaning as 
  *                         "roll" parameter of 
  *                         OrientationControl.setOrientation(int, int,
  *                         int) method of AMMS 
  *                         Java API. See JSR-234 Spec
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  */
javacall_result javacall_audio3d_orientation_control_set_orientation (
        javacall_audio3d_orientation_control_t *orientation_control,
                       int heading,
                       int pitch,
                       int roll );

/**
  * The function corresponding to 
  * OrientationControl.setOrientation(int[], int[])
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param orientation_control    pointer to the OrientationControl 
  * @param frontVector      this parameter has the same meaning as 
  *                         "frontVector" parameter of 
  *                         OrientationControl.setOrientation(int[],
  *                         int[]) method of AMMS 
  *                         Java API. See JSR-234 Spec.
  *                         frontVector is a vector in 3D space which
  *                         is frontward-directed in regard to the 
  *                         spectator. Java layer is responsible for
  *                         the following:
  *                         - this vector is not (0,0,0)
  *                         - this vector is not parallel to the aboveVector in
  *                         3D space
  * @param aboveVector      this parameter has the same meaning as 
  *                         "aboveVector" parameter of 
  *                         OrientationControl.setOrientation(int[],
  *                         int[]) method of AMMS 
  *                         Java API. See JSR-234 Spec
  *                         aboveVector is a vector in 3D space which
  *                         is directed somewhere above but w/o left or
  *                         right deviation in regard to the 
  *                         spectator. Java layer is responsible for
  *                         the following:
  *                         - this vector is not (0,0,0)
  *                         - this vector is not parallel to the frontVector in
  *                         3D space
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_INVALID_ARGUMENT   if any parameter value limitation
  *                                     mentioned above is violated by
  *                                     Java layer
  */
javacall_result javacall_audio3d_orientation_control_set_orientation_vec (
        javacall_audio3d_orientation_control_t *orientation_control,
                       const int frontVector[3],
                       const int aboveVector[3]);

/** 
  * The function corresponding to 
  * OrientationControl.getOrientationVectors()
  * method of AMMS Java API. See JSR-234 Spec
  * 
  * 
  * @param orientation_control    pointer to the OrientationControl 
  * @return     an array of 6 elements which are two consecutive vectors
  *             in 3D space:
  *             - frontVector
  *             - aboveVector
  *             For explanation what are these vectors see the following: 
  *             - JSR-234 Spec, OrientationControl description.
  *             - SeeAlso link
  *             @see setOrientationVec
  */
const int* javacall_audio3d_orientation_control_get_orientation_vectors (
    javacall_audio3d_orientation_control_t *orientation_control );

/**
  * The function corresponding to GlobalManager.createSoundSource3D()
  *  method of AMMS Java API, see JSR-234 Spec
  * Create new source of sound in 3D space.
  *
  * @param  manager     pointer to the Local Manager responsible for creation
  *                     3D Sound Sources within the current Java application.
  *                     @see javacall_amms_local_manager_t
  * @param  source  pointer to return the address of the new 
  *                 javacall_audio3d_soundsource3d_t instance
  *                 or NULL if creation of 3D sound sources is not supported
  *                 @see javacall_audio3d_soundsource3d_t
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_FAIL               Fail
  * @retval JAVACALL_NOT_IMPLEMENTED    if creation of 3D sound sources is not
  *                                     supported
  * @retval JAVACALL_OUT_OF_MEMORY      if there is not enough memory to 
  *                                     create new SoundSource3D
  *                                     @see javacall_result
  */
javacall_result javacall_amms_local_manager_create_sound_source3d(
                    javacall_amms_local_manager_t* manager,
                    /*OUT*/ javacall_audio3d_soundsource3d_t** source );

/**
  * Destroys source of sound in 3D space when it is no longer needed.
  *
  * @param  manager     pointer to the Local Manager responsible for creation
  *                     3D Sound Sources within the current Java application.
  *                     @see javacall_amms_local_manager_t
  * @param  source  pointer to the 3D sound source to be destroyed
  *                 @see javacall_audio3d_soundsource3d_t
  * @retval JAVACALL_OK                 Success
  * @retval JAVACALL_INVALID_ARGUMENT   if source is NULL or it is not a 
  *                                     pointer to 
  *                                     javacall_audio3d_soundsource3d_t
  */
javacall_result javacall_amms_local_manager_destroy_sound_source3d( 
                    javacall_amms_local_manager_t* manager,
                    javacall_audio3d_soundsource3d_t* source );

/**
  * The function corresponding to GlobalManager.getSpectator()
  *  method of AMMS Java API, see JSR-234 Spec
  * Return pointer to the Spectator in 3D Audio space
  *
  * @param  manager     pointer to the Local Manager responsible for 
  *                     the current Java application.
  *                     @see javacall_amms_local_manager_t
  * @return pointer to the javacall_audio3d_spectator_t instance which is
  *         unique for the current Java application or NULL if Spectator
  *         is not supported
  *         @see javacall_audio3d_spectator_t
  */
javacall_audio3d_spectator_t* javacall_amms_local_manager_get_spectator(
                    javacall_amms_local_manager_t* manager );

/**
  * The function corresponding to 
  * GlobalManager.getSupportedSoundSource3DPlayerTypes()
  *  method of AMMS Java API, see JSR-234 Spec
  * Return array of content types, device and capture locators
  * (from the enum) that can be
  * used to create players connectable to 3D Sound Sources.
  * 
  * @param number_of_types  pointer to return the returned array size
  *
  * @return array of the player types (from enum) supported by 3D stuff. The
  *         size of the array is returned using "javacall_media_type"
  *         parameter
  *         @see javacall_media_type
  */
const javacall_media_type*
    javacall_audio3d_get_supported_soundsource3d_player_types( 
        /*OUT*/ int *number_of_types );

/** @} */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /*__JAVACALL_MULTIMEDIA_ADVANCED_H*/
