/*
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
#include <kni.h>
#include "mni.h"

/*=========================
* KNI functions
*=========================
*/


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundRenderThread_nRender()
{
KNI_ReturnInt(0);
}


/*
* native int nOpen(int sRate, int bits, int channels, int nchunks, int nbuflen);
*/


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPCMOut_nOpen() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundPCMOut_nClose() {
KNI_ReturnVoid();
}

/*
* native int nWrite(int peer, int offset, int len, byte[] data);
*/
KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPCMOut_nWrite() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundPCMOut_nAddPlayer()
{
KNI_ReturnVoid();
}
KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundPCMOut_nRemovePlayer()
{
KNI_ReturnVoid();
}
KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPCMOut_nInitEffectModule()
{
KNI_ReturnInt(0);
}
KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPCMOut_nInitVolumeCtrl()
{
KNI_ReturnInt(0);
}
KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPCMOut_nInitRateCtrl()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundGlobalEffectModule_finalize()
{
KNI_ReturnVoid();
}



/*
* native int nInitQSound();
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundHiddenManager_nInitQSound()
{
KNI_ReturnInt(0);
}


/*
* native int nInitReverbCtrl(int gmPeer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalManager_nInitReverbCtrl()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalManager_nInitCommitCtrl()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalManager_nInitAudioVirtualizerCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalManager_nInitChorusCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalManager_nInitEqualizerCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalManager_nInitVolumeCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalManager_nInitPanCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalManager_nInitEffectOrderCtrl()
{
KNI_ReturnInt(0);
}




KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundGlobalEffectModule_nInitGlobalEffectModule()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundGlobalEffectModule_nAddPlayer()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundGlobalEffectModule_nRemovePlayer()
{
KNI_ReturnVoid();
}




/*
* native int nInitSoundSouce3D(int peer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSoundSource3D_nInitSoundSource3D()
{
KNI_ReturnInt(0);
}


/*
* native int nInitLocationCtrl(int peer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSoundSource3D_nInitLocationCtrl()
{
KNI_ReturnInt(0);
}

/*
* native int nInitDopplerCtrl(int peer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSoundSource3D_nInitDopplerCtrl()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSoundSource3D_nInitDirectivityCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSoundSource3D_nInitDistanceAttenuationCtrl()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSoundSource3D_nInitMacroscopicCtrl()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSoundSource3D_nInitObstructionCtrl()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSoundSource3D_nInitReverbSourceCtrl()
{
KNI_ReturnInt(0);
}



/*
* native int nAddMIDIPlayer(int ss_peer, int pl_peer);
*/

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundSoundSource3D_nAddMIDIPlayer()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundSoundSource3D_nRemoveMIDIPlayer()
{
KNI_ReturnVoid();
}


/*
* native int nAddPlayer(int ss_peer, int pl_peer);
*/

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundSoundSource3D_nAddPlayer()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundSoundSource3D_nRemovePlayer()
{
KNI_ReturnVoid();
}

/*
* native int nInitEffectModule(int peer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectModule_nInitEffectModule()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectModule_nInitReverbSourceCtrl()
{
KNI_ReturnInt(0);
}



KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectModule_nInitAudioVirtualizerCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectModule_nInitChorusCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectModule_nInitEqualizerCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectModule_nInitVolumeCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectModule_nInitPanCtrl()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectModule_nInitEffectOrderCtrl()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundEffectModule_nAddMIDIPlayer()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundEffectModule_nRemoveMIDIPlayer()
{
KNI_ReturnVoid();
}



KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundEffectModule_nAddPlayer()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundEffectModule_nRemovePlayer()
{
KNI_ReturnVoid();
}



/*
* native int nInitSpectator(int peer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSpectator_nInitSpectator()
{
KNI_ReturnInt(0);
}


/*
* native int nInitSpecLocCtrl(int peer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSpectator_nInitSpecLocCtrl()
{
KNI_ReturnInt(0);
}


/*
* native int nInitSpecOriCtrl(int peer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSpectator_nInitSpecOriCtrl()
{
KNI_ReturnInt(0);
}


/*
* native int nInitSpecDopCtrl(int peer);
*/

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSpectator_nInitSpecDopCtrl()
{
KNI_ReturnInt(0);
}








KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundLocationCtrl_nGetPosition()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundLocationCtrl_nSetPosition()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundLocationCtrl_nSetPolarPosition()
{
KNI_ReturnBoolean(KNI_FALSE);
}




KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundOrientationCtrl_nGetOrientationVec()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundOrientationCtrl_nSetOrientation()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundOrientationCtrl_nSetOrientationVectors()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundDopplerCtrl_nIsEnabled()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundDopplerCtrl_nSetEnabled()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundDopplerCtrl_nGetVelocityCart()
{
KNI_ReturnVoid();
}



KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundDopplerCtrl_nSetVelocity()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundDopplerCtrl_nSetVelocitySphere()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundReverbCtrl_nIsEnabled()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundReverbCtrl_nSetEnabled()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundReverbCtrl_nSetPreset()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundReverbCtrl_nGetScope()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundReverbCtrl_nSetScope()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundReverbCtrl_nIsEnforced()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundReverbCtrl_nSetEnforced()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundReverbCtrl_nGetPreset()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundReverbCtrl_nGetPresetName()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundReverbCtrl_nGetReverbTime()
{
KNI_ReturnBoolean(KNI_FALSE);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundReverbCtrl_nSetReverbTime()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundReverbCtrl_nGetReverbLevel()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundReverbCtrl_nSetReverbLevel()
{
KNI_ReturnBoolean(KNI_FALSE);
}





KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundDirectivityCtrl_nGetParameters()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundDirectivityCtrl_nSetParameters()
{
KNI_ReturnBoolean(KNI_FALSE);
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundDirectivityCtrl_nSetOrientation()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundDirectivityCtrl_nGetOrientationVectors()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundDirectivityCtrl_nSetOrientationVectors()
{
KNI_ReturnBoolean(KNI_FALSE);
}



KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundDistanceAttenuationCtrl_nGetMaxDistance()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundDistanceAttenuationCtrl_nGetMinDistance()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundDistanceAttenuationCtrl_nGetRolloffFactor()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundDistanceAttenuationCtrl_nGetMuteAfterMax()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundDistanceAttenuationCtrl_nSetParameters()
{
KNI_ReturnBoolean(KNI_FALSE);
}



KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundMacroscopicCtrl_nGetSize()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMacroscopicCtrl_nSetSize()
{
KNI_ReturnBoolean(KNI_FALSE);
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundMacroscopicCtrl_nGetOrientationVec()
{
KNI_ReturnVoid();
}



KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundMacroscopicCtrl_nSetOrientation()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMacroscopicCtrl_nSetOrientationVectors()
{
KNI_ReturnBoolean(KNI_FALSE);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundObstructionCtrl_nGetLevel()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundObstructionCtrl_nSetLevel()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundObstructionCtrl_nGetHFLevel()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundObstructionCtrl_nSetHFLevel()
{
KNI_ReturnBoolean(KNI_FALSE);
}



KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundReverbSourceCtrl_nGetRoomLevel()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundReverbSourceCtrl_nSetRoomLevel()
{
KNI_ReturnBoolean(KNI_FALSE);
}




KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundCommitCtrl_nIsDeferred()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundCommitCtrl_nSetDeferred()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundCommitCtrl_nCommit()
{
KNI_ReturnVoid();
}





KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundAudioVirtualizerCtrl_nSetEnabled()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundAudioVirtualizerCtrl_nSetPreset()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundAudioVirtualizerCtrl_nGetScope()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundAudioVirtualizerCtrl_nSetScope()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundAudioVirtualizerCtrl_nSetEnforced()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundAudioVirtualizerCtrl_nGetPreset()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundAudioVirtualizerCtrl_nGetPresetName()
{
KNI_ReturnInt(0);
}





KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundChorusCtrl_nSetEnabled()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundChorusCtrl_nSetPreset()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetScope()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundChorusCtrl_nSetScope()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundChorusCtrl_nSetEnforced()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetPreset()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetPresetName()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetMaxAverageDelay()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetMaxModulationDepth()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetMaxModulationRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetMinModulationRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetAverageDelay()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetModulationDepth()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetModulationRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundChorusCtrl_nGetWetLevel()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundChorusCtrl_nSetAverageDelay()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundChorusCtrl_nSetModulationDepth()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundChorusCtrl_nSetModulationRate()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundChorusCtrl_nSetWetLevel()
{
KNI_ReturnBoolean(KNI_FALSE);
}










KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundEqualizerCtrl_nSetEnabled()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEqualizerCtrl_nSetPreset()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetScope()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEqualizerCtrl_nSetScope()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundEqualizerCtrl_nSetEnforced()
{
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetPreset()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetPresetName()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetBand()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetCenterFreq()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetMaxBandLevel()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetMinBandLevel()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetNumberOfBands()
{
KNI_ReturnInt(0);
}



KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetBandLevel()
{
KNI_ReturnBoolean(KNI_FALSE);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetBass()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEqualizerCtrl_nGetTreble()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEqualizerCtrl_nSetBandLevel()
{
KNI_ReturnBoolean(KNI_FALSE);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEqualizerCtrl_nSetBass()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEqualizerCtrl_nSetTreble()
{
KNI_ReturnBoolean(KNI_FALSE);
}





KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundVolumeCtrl_nSetMute()
{
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundVolumeCtrl_nSetLevel()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundVolumeCtrl_nIsMuted()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundVolumeCtrl_nGetLevel()
{
KNI_ReturnInt(0);
}



KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPanCtrl_nGetPan()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPanCtrl_nSetPan()
{
KNI_ReturnInt(0);
}



KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEffectOrderCtrl_nGetEffectOrder()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundEffectOrderCtrl_nGetEffectOrders()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundEffectOrderCtrl_nSetEffectOrder()
{
KNI_ReturnBoolean(KNI_FALSE);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundHiddenManager_nInitQSoundMIDI() 
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayControl_nInitVolumeCtrl() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayControl_nInitMIDICtrl() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayControl_nInitPitchCtrl() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayControl_nInitRateCtrl() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayControl_nInitTempoCtrl() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayControl_nInitMetaDataCtrl() {
KNI_ReturnInt(0);
}






KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIToneSequencePlayControl_nInitVolumeCtrl() {
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIToneSequencePlayControl_nInitPitchCtrl() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIToneSequencePlayControl_nInitRateCtrl() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIToneSequencePlayControl_nInitTempoCtrl() {
KNI_ReturnInt(0);
}




KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayBase_nOpen() {
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundMIDIPlayBase_nClose() {
KNI_ReturnVoid();
}



KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMIDIPlayBase_nFillBuffer() {
KNI_ReturnBoolean(KNI_FALSE);
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundMIDIPlayBase_nDeallocateBuffer() {
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundMIDIPlayBase_nStart() {
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundMIDIPlayBase_nStop() {
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMIDIPlayBase_nIsDone() {
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayBase_nLoopsDone() {
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayBase_nGetDuration() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayBase_nGetPosition() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDIPlayBase_nSetPosition() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundMIDIPlayBase_nSetLoopCount() {
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSynthPerformance_nInitSynth() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundSynthPerformance_nInitSynthPerformance() {
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundSynthPerformance_nEnableChannel() {
KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundSynthPerformance_nWriteEventInt() {
KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundSynthPerformance_nPlayTone() {
KNI_ReturnVoid();
}




KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundRateCtrl_nGetMaxRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundRateCtrl_nGetMinRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundRateCtrl_nGetRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundRateCtrl_nSetRate()
{
KNI_ReturnInt(0);
}



KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPitchCtrl_nGetMaxPitch()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPitchCtrl_nGetMinPitch()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPitchCtrl_nGetPitch()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundPitchCtrl_nSetPitch()
{
KNI_ReturnInt(0);
}





KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundTempoCtrl_nGetMaxRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundTempoCtrl_nGetMinRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundTempoCtrl_nGetRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundTempoCtrl_nSetRate()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundTempoCtrl_nGetTempo()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundTempoCtrl_nSetTempo()
{
KNI_ReturnInt(0);
}






KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDICtrl_nGetBankList()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDICtrl_nGetChannelVolume()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDICtrl_nGetKeyName()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMIDICtrl_nGetProgram()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDICtrl_nGetProgramList()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDICtrl_nGetProgramName()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMIDICtrl_nIsBankQuerySupported()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDICtrl_nLongMidiEvent()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMIDICtrl_nSetChannelVolume()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMIDICtrl_nSetProgram()
{
KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundMIDICtrl_nShortMidiEvent()
{
KNI_ReturnBoolean(KNI_FALSE);
}



KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMetaDataCtrl_nGetKeyValue()
{
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMetaDataCtrl_nGetKeys()
{
KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDINullPlayControl_nInitVolumeCtrl() {
    KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundMIDINullPlayControl_nInitMIDICtrl() {
    KNI_ReturnInt(0);
}

//-----------------ABB stuff--------------

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundABBMIDIPlayControl_nInitVolumeCtrl() {
KNI_ReturnInt(0);
}


KNIEXPORT KNI_RETURNTYPE_VOID Java_com_sun_mmedia_QSoundABBVolumeCtrl_nSetMute()
{
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundABBVolumeCtrl_nSetLevel()
{
    KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN Java_com_sun_mmedia_QSoundABBVolumeCtrl_nIsMuted()
{
    KNI_ReturnBoolean(KNI_FALSE);
}

KNIEXPORT KNI_RETURNTYPE_INT Java_com_sun_mmedia_QSoundABBVolumeCtrl_nGetLevel()
{
    KNI_ReturnInt(0);
}
