/*
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

#include "mm_qsound_audio.h"
#include "wav_struct.h"

int wav_setStreamPlayerData(ah_wav *wav) {
    struct wavechnk  *wc;
    struct fmtchnk   *fc;
    struct datachnk  *dc;
    struct listchnk  *lc;

    unsigned char *data = wav->hdr.dataBuffer;
    unsigned char *end = data + wav->hdr.dataPos;

    if (wav->lastChunkOffset == 0) {
        /* Start wav decode */
        struct riffchnk *rc = (struct riffchnk *)wav->hdr.dataBuffer;
        if (wav->hdr.dataPos < sizeof(struct riffchnk))
            return 1;
        if (rc->chnk_id != CHUNKID_RIFF)
            return 0;

        if (rc->type != TYPE_WAVE)
            return 0;

        wav->lastChunkOffset = sizeof(struct riffchnk);
    }
    
    data += wav->lastChunkOffset;

    while (data + sizeof(struct wavechnk) <= end) {
        wc = (struct wavechnk *)data;

        /* Only data chunk does not require full presence */
        if (wc->chnk_id == CHUNKID_data) {
            dc = (struct datachnk *)data;
            wav->dataChunkLen = dc->chnk_ds;
            if (wav->playBuffer == NULL) {
                /* data chunk contents is the playBuffer */
                wav->playBuffer = data + sizeof(struct datachnk);
                wav->playPos = 0;
            }
            if (wav->playBuffer + dc->chnk_ds > end) {
                /* data chunk is not finished. Breaking the while loop */
                wav->playBufferLen = end - wav->playBuffer;
                break;
            } else {
                /* data chunk finished. Going to the next chunk */
                wav->playBufferLen = dc->chnk_ds;
                data = wav->playBuffer + dc->chnk_ds;
                /* verify if data chunk alignment is required */
                if (dc->chnk_ds % 2) {
                    data++;
                }
            }
        }

        /* Decode other chunks, fully */
        else if (data + wc->chnk_ds <= end) {
            switch (wc->chnk_id) {
            case CHUNKID_fmt:
                fc = (struct fmtchnk *)data;                
                /* Only support PCM */
                if (fc->compression_code != 1)
                    return -1;
                wav->channels = fc->num_channels;
                wav->rate = fc->sample_rate;
                wav->bits = fc->bits;
                wav->bytesPerMilliSec = (wav->rate *
                    wav->channels * (wav->bits >> 3)) / 1000;
            break; /* CHUNKID_fmt */

            case CHUNKID_LIST:
                lc = (struct listchnk *)data;
                /* Currently supporting only INFO */
                if (lc->type == TYPE_INFO) {
                    char *tmpData = data + sizeof(struct listchnk);
                    char *endData = tmpData + lc->chnk_ds;

                    while (tmpData < endData) {
                        struct sublistchnk *tmpChnk =
                            (struct sublistchnk *)tmpData;
                        char **str = NULL;

                        tmpData += sizeof(struct sublistchnk);
                        
                        switch (tmpChnk->chnk_id) {
                            /* Support only 4 subtypes */
                            case INFOID_IART:
                                str = &(wav->metaData.iartData);
                            break;
                            case INFOID_ICOP:
                                str = &(wav->metaData.icopData);
                            break;
                            case INFOID_ICRD:
                                str = &(wav->metaData.icrdData);
                            break;
                            case INFOID_INAM:
                                str = &(wav->metaData.inamData);
                            break;
                            case INFOID_ICMT:
                                str = &(wav->metaData.icmtData);
                            break;
                            case INFOID_ISFT:
                                str = &(wav->metaData.isftData);
                            break;
                        }
                        if (str != NULL) {
                            *str = REALLOC(*str, tmpChnk->chnk_ds + 1);
                            memcpy(*str, tmpData, tmpChnk->chnk_ds);
                            (*str)[tmpChnk->chnk_ds] = '\0';
                        }
                        tmpData += tmpChnk->chnk_ds;
                        /* verify if data chunk alignment is required */
                        if (tmpChnk->chnk_ds % 2) {
                            tmpData++;
                        }
                    }
                }
            break; /* CHUNKID_LIST */
            
            default: /* UNKNOWN */
            {
                char str[5];
                char outstr[256];
                memcpy(str, &(wc->chnk_id), 4);
                str[4] = '\0';
                sprintf(outstr, "Unexpected chunk desc: %s size:%d\n", str, wc->chnk_ds);
                OutputDebugString(outstr);
                if (wc->chnk_ds < 0)
                    wc->chnk_ds = 0; /* Wrong thing! Trying to recover .*/
            } /* default */
            } /* switch (wc->chnk_id) */
            data += sizeof(struct wavechnk) + wc->chnk_ds;
            /* verify if data chunk alignment is required */
            if (wc->chnk_ds % 2) {
                data++;
            }
        } else {
            break;
        }
    }

    /* Remember last unfinished chunk */
    wav->lastChunkOffset = data - wav->hdr.dataBuffer;

    /* finally check if data ends */
    data = wav->hdr.dataBuffer;
    wav->hdr.dataEnded = 
        ( wav->hdr.dataPos >= ((struct riffchnk *)data)->chnk_ds 
                              + (int)sizeof(struct riffchnk) 
                              - (int)sizeof(long) )
        ? JAVACALL_TRUE
        : JAVACALL_FALSE;
    return 1;
}
