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
#include "mmrecord.h"

#pragma pack(1)
struct riffchnk
{
    long chnk_id;
    long chnk_ds;
    long type;
};
#pragma pack()

#pragma pack(1)
struct wavechnk
{
    long chnk_id;
    long chnk_ds;
};
#pragma pack()

#pragma pack(1)
struct fmtchnk
{
    long chnk_id;
    long chnk_ds;
    short compression_code;
    short num_channels;
    long sample_rate;
    long bytes_per_second;
    short block_align;
    short bits;
};
#pragma pack()

#pragma pack(1)
struct datachnk
{
    long chnk_id;
    long chnk_ds;
};
#pragma pack()

#pragma pack(1)
struct listchnk
{
    long chnk_id;
    long chnk_ds;
    long type;
};
#pragma pack()

#pragma pack(1)
struct sublistchnk
{
    long chnk_id;
    long chnk_ds;
};
#pragma pack()

#pragma pack(1)
struct std_head
{
    struct riffchnk rc;
    struct fmtchnk  fc;
    struct datachnk dc;
    struct listchnk lc;
};
#pragma pack()

int create_wavhead(recorder* h, char *buffer, int buflen)
{
    struct std_head wh;

    if(buffer == NULL) return sizeof(wh);
    JC_MM_ASSERT(buflen >= sizeof(wh));

    memset(&wh, '\0', sizeof(wh));

    wh.rc.chnk_id          = 0x46464952; // RIFF
    wh.rc.chnk_ds          = 4;
    wh.rc.type             = 0x45564157; // WAVE

    wh.fc.chnk_id          = 0x20746D66;  //fmt.
    wh.fc.chnk_ds          = 16;
    wh.fc.compression_code = 1;
    wh.fc.num_channels     = h->channels;
    wh.fc.sample_rate      = h->rate;
    wh.fc.bytes_per_second = 0;  // ??? NEED REVISIT
    wh.fc.block_align      = 0;   // ?? NEED REVISIT
    wh.fc.bits             = h->bits;

    wh.dc.chnk_id          = 0x61746164;
    wh.dc.chnk_ds          = h->recordLen;

    memcpy(buffer, &wh, sizeof(wh));
    return sizeof(wh);
}

int wav_setStreamPlayerData(ah_wav *handle, const void* buffer, long length)
{
    struct wavechnk  *wc;
    struct fmtchnk   *fc;
    struct datachnk  *dc;
    struct listchnk  *lc;

    unsigned char *data;
    unsigned char *end = (unsigned char *)buffer + length;

    int chnkSize = 0;

    javacall_bool parse_next = JAVACALL_TRUE;

    if (buffer == NULL) {
        if (handle->originalData)
            FREE(handle->originalData);
        handle->originalData = NULL;
        handle->streamBufferFull = JAVACALL_TRUE;
        return 1;
    }

    if (handle->originalData == NULL && handle->streamBufferLen == 0) {
        struct riffchnk *rc; 
        data = (unsigned char *)buffer;
        end = data + length;
        rc = (struct riffchnk *)data; 
        if (rc->chnk_id != 0x46464952)  // RIFF
            return 0;

        if (rc->type != 0x45564157)  // WAVE
            return 0;

        data += sizeof(struct riffchnk);
        parse_next = JAVACALL_TRUE;
        handle->originalData_dataChnk = JAVACALL_FALSE;
    } else {
        if (handle->originalData_dataChnk) {
            data = (unsigned char *)buffer;
            chnkSize = handle->originalData_dataChnkFull - handle->originalData_dataChnkOffset;
            if (chnkSize <= length) {
                memcpy(handle->streamBuffer+handle->streamBufferLen, data, chnkSize);
                handle->streamBufferLen += chnkSize;
                data += chnkSize;
                handle->originalData_dataChnk = JAVACALL_FALSE;
            } else {
                memcpy(handle->streamBuffer+handle->streamBufferLen, data, length);
                handle->streamBufferLen += length;
                data += length;
                handle->originalData_dataChnkOffset += length;
            }
printf("jsr135: download data len=%d whole_size=%d\n", length, handle->streamBufferLen);
        } else {
            /* just skip this chunk for now */
            if (handle->originalDataLen == 4) {
                data = (unsigned char *)buffer;
                chnkSize = *((long *)data);
                data += 4 + chnkSize;
            } else {
                data = (unsigned char *)(handle->originalData + 4);
                chnkSize = *((long *)data);
                data = (unsigned char *)buffer + (chnkSize - handle->originalDataLen - 8);
            }
        }
    }

    while(parse_next && (end - data)>8)
    {
        wc = (struct wavechnk *)data;
        switch(wc->chnk_id)
        {
            // fmt.
            case 0x20746D66:
                if ((data + wc->chnk_ds + 8) <= end) {
                    fc = (struct fmtchnk *)data;
                
                    // Only support PCM
                    if(fc->compression_code != 1)
                        return -1;

                    handle->channels = fc->num_channels;
                    handle->rate = fc->sample_rate;
                    handle->bits = fc->bits;

                    data += wc->chnk_ds + 8; 
                } else {
                    parse_next = 0;
                }
            break;
/* Temporary unsupported - just skip
            // LIST
            case 0x5453494C:
            {
                char *pData = data;
                lc = (struct listchnk *)pData;
                chnkSize = lc->chnk_ds;
                if ((data + chnkSize + 8) <= end )
                    // Only support INFO currently
                    if(lc->type == 0x4F464E49)
                    {
                        int chnkSizeCnt = 0;

                        char * tmpData = pData + sizeof(struct listchnk);
                        while(chnkSizeCnt < chnkSize)
                        {
                            struct sublistchnk *tmpChnk = 
                                (struct sublistchnk *)(tmpData + chnkSizeCnt);
                            char **str = NULL;

                            switch(tmpChnk->chnk_id)
                            {
                                // IART
                                case 0x54524149:
                                    str = &(handle->metaData.iartData);
                                break;

                                // ICOP
                                case 0x504F4349:
                                    str = &(handle->metaData.icopData);
                                break;

                                // ICRD
                                case 0x44524349:
                                    str = &(handle->metaData.icrdData);
                                break;

                                case 0x4D414E49:
                                    str = &(handle->metaData.inamData);
                                break;

                                default:
                                    OutputDebugString("Unexpected INFO SubList type\n");
                                break;
                            }

                            if(str != NULL)
                            {
                                *str = REALLOC(*str, tmpChnk->chnk_ds + 1);
                                memset(*str, '\0', tmpChnk->chnk_ds + 1);
                                memcpy(*str, tmpData+sizeof(struct wavechnk), 
                                    tmpChnk->chnk_ds);
                            }

                            chnkSizeCnt += tmpChnk->chnk_ds + 8;
                        }

                        pData += chnkSizeCnt + 8;
                    }
                    pData += chnkSize + 8;
                    data = pData;
                } else {
                    parse_next = 0;
                }
                break;
            }
            break;
*/

            // DATA 
            case 0x61746164:
                dc = (struct datachnk *)data;
                chnkSize = dc->chnk_ds;
                data += 8;
                handle->streamBuffer = 
                    REALLOC(handle->streamBuffer, 
                        handle->streamBufferLen+chnkSize);
                if ((end - data)>=chnkSize) {
                    memcpy(handle->streamBuffer+handle->streamBufferLen, 
                        data, chnkSize);
                    handle->streamBufferLen += chnkSize;
                    data += chnkSize;
                } else {
                    handle->originalData_dataChnk = JAVACALL_TRUE;
                    handle->originalData_dataChnkFull = chnkSize;
                    handle->originalData_dataChnkOffset = end - data;
                    memcpy(handle->streamBuffer+handle->streamBufferLen, 
                        data, handle->originalData_dataChnkOffset);
                    handle->streamBufferLen += handle->originalData_dataChnkOffset;
                    data += handle->originalData_dataChnkOffset;
                }
            break;

            // UNKNOWN
            default:
                if ((data + wc->chnk_ds + 8) <= end) {

                    {
                        char str[5];
                        char outstr[256];
                        memcpy(str, &(wc->chnk_id), 4);
                        str[4] = '\0';

                        sprintf(outstr, 
                            "Unexpected chunk desc: %s size:%d\n", str, wc->chnk_ds);

                        OutputDebugString(outstr);
                    }

                    if(chnkSize < 0)  // Huh? something has gone wrong...
                        chnkSize = 0;

                    data += wc->chnk_ds + 8;
                } else {
                    parse_next = 0;
                }
            break;
        }
        if (parse_next == 0) {
            handle->originalDataLen = end - data;
            if (handle->originalData == NULL) {
                handle->originalDataFull = handle->originalDataLen;
                handle->originalData = MALLOC(handle->originalDataFull);
                memcpy(handle->originalData, data, handle->originalDataLen);
            } else {
                if (handle->originalDataFull < handle->originalDataLen) {
                    handle->originalData = REALLOC(handle->originalData, handle->originalDataLen);
                    handle->originalDataFull = handle->originalDataLen;
                }
                memcpy(handle->originalData, data, handle->originalDataLen);
            }
        }
    }

    return 1;
}
