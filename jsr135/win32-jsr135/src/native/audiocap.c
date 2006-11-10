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

/**
 * @file audiocap.c
 * Windows-specific implementation for audio capture.
 * This file implements native functionality required by WavCapture.java
 */

/* IMPL_NOTE:
 * - consolidate the native Java functions in one shared file,
 *   have only platform-specific implementations of
 *   open/start/stop/close/read
 * - remove global buffer!
 */

#include "mni.h"
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#ifdef CDC
#include "com_sun_mmedia_protocol_WavCapture.h"
#endif

/* Constants for command types */

#define START 1
#define STOP  2
#define CLOSE 3


/* Structure to hold run-time values and buffers for capture */
typedef struct _WIP {
    HANDLE handle;
    int nchunks;
    int chunkSize;
    int count;
    char **data;
    int *size;
    WAVEHDR **headers;
    char *pendingData;
    int pendingSize;
} WaveInPeer;


/* Allocate buffer 1/32 of a second at max sample rate 48000,
   16 bit, stereo
   48000*2*2/32 = 6000;
*/
#define AUDIOCAP_BUFFER_SIZE (48000*2*2/32)

/**
 * This function closes the audio device and
 * uses MNI_FREE to release all memory.
 *
 * Precondition:
 * waveInStop() and waveInReset() must
 * have been called prior to calling this
 * function.
 *
 * @param peer Free the WaveInPeer resource.
 */
void releaseResources(WaveInPeer *peer) {
    int i;
    int maxCloseTrials = 200;

    while ((waveInClose(peer->handle) == WAVERR_STILLPLAYING)
           && --maxCloseTrials) {
	Sleep(20); // in millis
    }
#ifdef DEBUG_CAPTURE
    if (!maxCloseTrials) {
	printf("Could not close audio device!\n");
    }
#endif
    for (i = 0; i < peer->nchunks; i++) {
	MNI_FREE(peer->headers[i]);
	MNI_FREE(peer->data[i]);
    }
    MNI_FREE(peer->headers);
    MNI_FREE(peer->data);
    MNI_FREE(peer->pendingData);
    MNI_FREE(peer);
}

/**
 * This function opens the audio device in the specified format.
 * @param jsampleRate The sample rate desired.
 * @param jsampleSize The sample size or bits/sample desired.
 * @param jchannels The number of channels (mono=1 or stereo).
 * @param jnchunks The number of chunks (unused parameter).
 * @return a handle to the capture device.
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_protocol_WavCapture_nOpen
(MNI_FUNCTION_PARAMS_4(jint jsampleRate,
		       jint jsampleSize,
		       jint jchannels,
		       jint jnchunks)) {

    jint sampleRate;
    jint sampleSize;
    jint channels;
    jint nchunks;

    WaveInPeer *peer;
    WAVEFORMATEX wfe;
    HWAVEIN wavein;
    int i;
    int chunkSize;
    MMRESULT result;


    MNI_GET_INT_PARAM(sampleRate, jsampleRate, 1);
    MNI_GET_INT_PARAM(sampleSize, jsampleSize, 2);
    MNI_GET_INT_PARAM(channels, jchannels, 3);
    MNI_GET_INT_PARAM(nchunks, jnchunks, 4);

    wfe.cbSize = sizeof(WAVEFORMATEX);
    wfe.wFormatTag = WAVE_FORMAT_PCM;
    wfe.nSamplesPerSec = sampleRate;
    wfe.wBitsPerSample = (USHORT) sampleSize;
    wfe.nChannels = (USHORT) channels;
    wfe.nBlockAlign = (channels * sampleSize) / 8;
    wfe.nAvgBytesPerSec = sampleRate * wfe.nBlockAlign;

    chunkSize = 8 * (wfe.nAvgBytesPerSec / 64); /* 125 milliseconds */

    result = waveInOpen((LPHWAVEIN) &wavein, WAVE_MAPPER, &wfe, 0, 0, CALLBACK_NULL);
    if (result != MMSYSERR_NOERROR) {
	MNI_RET_VALUE_INT(0);
    }

    peer = (WaveInPeer*) MNI_MALLOC(sizeof(WaveInPeer));
    peer->nchunks = nchunks;
    peer->chunkSize = chunkSize;
    peer->handle = wavein;
    peer->data = (char **) MNI_MALLOC(sizeof(char *) * nchunks);
    peer->headers = (WAVEHDR **) MNI_MALLOC(sizeof(WAVEHDR *) * nchunks);
    peer->pendingData = (char *) MNI_MALLOC(chunkSize);
    peer->pendingSize = 0;
    for (i = 0; i < nchunks; i++) {
	peer->data[i] = (char*) MNI_MALLOC(chunkSize);
	peer->headers[i] = (WAVEHDR*) MNI_MALLOC(sizeof(WAVEHDR));
	peer->headers[i]->dwBufferLength = 0;
    }
    peer->count = 0;

    /* Prepare all headers */
    for (i = 0; i < peer->nchunks; i++) {
	peer->headers[i]->lpData = peer->data[i];
	peer->headers[i]->dwFlags = 0;
	peer->headers[i]->dwBufferLength = peer->chunkSize;
	waveInPrepareHeader(peer->handle, peer->headers[i], sizeof(WAVEHDR));
	waveInAddBuffer(peer->handle, peer->headers[i], sizeof(WAVEHDR));
    }
    MNI_RET_VALUE_INT((jint) peer);
}

int
aRead(WaveInPeer *peer, char *bdata, int offset, int length)
{
    int next = peer->count;
    int toCopy = 0;
    int totalCopied = 0;

    /* See if we have pending data */
    if (peer->pendingSize > 0) {
	toCopy = peer->pendingSize;
	if (toCopy > length)
	    toCopy = length;
	memcpy(bdata + offset, peer->pendingData, toCopy);
	peer->pendingSize -= toCopy;
	/* Move pending data to the beginning of pending buffer */
	if (peer->pendingSize)
	    memmove(peer->pendingData, peer->pendingData + toCopy, peer->pendingSize);
	offset += toCopy;
	length -= toCopy;
	totalCopied = toCopy;
    }

    /* Check if the next buffer in line is done */
    if (length > 0 && (peer->headers[next]->dwFlags & WHDR_DONE)) {
	int bytesAvailable = peer->headers[next]->dwBytesRecorded;
	toCopy = bytesAvailable;
	if (toCopy > length)
	    toCopy = length;
	memcpy(bdata + offset, peer->data[next], toCopy);
	totalCopied += toCopy;
	/* Have any pending data to keep for next read? */
	if (toCopy < bytesAvailable) {
	    peer->pendingSize = bytesAvailable - toCopy;
	    memcpy(peer->pendingData, peer->data[next] + toCopy, peer->pendingSize);
	}
	/* Re-use the buffer just emptied */
	peer->count = (peer->count + 1) % peer->nchunks;
	waveInUnprepareHeader(peer->handle, peer->headers[next], sizeof(WAVEHDR));
	peer->headers[next]->lpData = peer->data[next];
	peer->headers[next]->dwFlags = 0;
	peer->headers[next]->dwBufferLength = peer->chunkSize;
	waveInPrepareHeader(peer->handle, peer->headers[next], sizeof(WAVEHDR));
	waveInAddBuffer(peer->handle, peer->headers[next], sizeof(WAVEHDR));
    }

    return totalCopied;
}

void
flush(WaveInPeer *peer)
{
    int nRead;
    while ((nRead = aRead(peer, peer->pendingData, 0, peer->chunkSize)) != 0);
}

/**
 * This function is used to START, STOP or CLOSE the capture audio device.
 * @param jpeer Handle to the capture audio device.
 * @param jcommand The command sent to the capture audio device.
 * 	jcommand = 1 ==> START command
 * 	jcommand = 2 ==> STOP  command
 *	jcommand = 3 ==> CLOSE command
 * @param jparam Parameters if any to the command. These three
 * commands don't have any parameters and so jparam is not used.
 * @return 0 on error and 1 on success.
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_protocol_WavCapture_nCommand
(MNI_FUNCTION_PARAMS_3(jint jpeer,
                       jint jcommand,
                       jint jparam)) {

    jint peerint;
    jint command;
    jint param;
    WaveInPeer *peer;

    MNI_GET_INT_PARAM(peerint, jpeer, 1);
    MNI_GET_INT_PARAM(command, jcommand, 2);
    MNI_GET_INT_PARAM(param, jparam, 3);

    peer = (WaveInPeer *) peerint;

    if (peer == NULL) { // no-op
	MNI_RET_VALUE_INT(0);
    }

    switch (command) {
    case START:
	/* $$fb don't think we need flush here
	 * flush(peer); */
	waveInStart(peer->handle);
	break;

    case STOP:
	/* fall through */
    case CLOSE:
	waveInStop(peer->handle);
	waveInReset(peer->handle);
	if (command == CLOSE) {
	    releaseResources(peer);
	}
	break;
    }

    MNI_RET_VALUE_INT(1);
}

/**
 * This function is used to read audio data from the capture device.
 * @param jpeer Handle to the capture audio device.
 * @param jdata The byte array to read the data into.
 * @param joffset Data read from the capture device will be stored in the
 * byte array with this array offset.
 * @param jlength Number of bytes to read.
 * @return Actual number of bytes read
 */
MNI_RET_TYPE_INT
Java_com_sun_mmedia_protocol_WavCapture_nRead
(MNI_FUNCTION_PARAMS_4(jint jpeer,
		      jbyteArray jdata,
		      jint joffset,
		      jint jlength)) {
    WaveInPeer *peer;
    jint peerint;
    jint offset;
    jint length;
    jint actual = 0;

    MNI_GET_INT_PARAM(peerint, jpeer, 1);
    MNI_GET_INT_PARAM(offset, joffset, 3);
    MNI_GET_INT_PARAM(length, jlength, 4);

    peer = (WaveInPeer*) peerint;
    if (peer != NULL) {
        char* data = (char*) MNI_MALLOC(AUDIOCAP_BUFFER_SIZE);
        if (data != NULL) {
            actual = aRead(peer, data, 0, length);
            MNI_SET_BYTE_ARRAY_REGION(data, jdata, offset, length, 2);
            MNI_FREE(data);
        } else {
            actual = 0;
            MNI_THROW_NEW("java/lang/OutOfMemoryError", "Can't allocate file read buffer");
        }
    }
    MNI_RET_VALUE_INT(actual);
}

