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

int wav_setStreamPlayerData(ah_wav *handle)
{
    struct wavechnk  *wc;
    struct fmtchnk   *fc;
    struct datachnk  *dc;
    struct listchnk  *lc;

    unsigned char *data = handle->originalData;

    struct riffchnk *rc = (struct riffchnk *)data; 

    int chnkSize = 0;

    char *end;

    if(data == NULL) return 0;

    end = data + handle->originalDataLen;

    if (rc->chnk_id != 0x46464952)  // RIFF
        return 0;

    if (rc->type != 0x45564157)  // WAVE
        return 0;

    data += sizeof(struct riffchnk);

    while(data<end)
    {
        wc = (struct wavechnk *)data;

        switch(wc->chnk_id)
        {
            // fmt.
            case 0x20746D66:
                fc = (struct fmtchnk *)data;
            
                // Only support PCM
                if(fc->compression_code != 1)
                    return -1;

                handle->channels = fc->num_channels;
                handle->rate = fc->sample_rate;
                handle->bits = fc->bits;

                chnkSize = fc->chnk_ds;

                data += chnkSize + 8; 

            break;
       
            // DATA 
            case 0x61746164:
                dc = (struct datachnk *)data;
                chnkSize = dc->chnk_ds;

                handle->streamBuffer = 
                    REALLOC(handle->streamBuffer, 
                        handle->streamBufferLen+chnkSize);

                memcpy(handle->streamBuffer+handle->streamBufferLen, 
                    data+8, chnkSize);

                handle->streamBufferLen += chnkSize;

                data += chnkSize + 8;

            break;

            // LIST
            case 0x5453494C:
            {
                lc = (struct listchnk *)data;
                chnkSize = lc->chnk_ds;

                // Only support INFO currently
                if(lc->type == 0x4F464E49)
                {
                    int chnkSizeCnt = 0;

                    char * tmpData = data + sizeof(struct listchnk);
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

                    data += chnkSizeCnt + 8;
                }

                data += chnkSize + 8;
                break;
            }   

            break;

            // UNKNOWN
            default:
                chnkSize = wc->chnk_ds;

                {
                    char str[5];
                    char outstr[256];
                    memcpy(str, &(wc->chnk_id), 4);
                    str[4] = '\0';

                    sprintf(outstr, 
                        "Unexpected chunk desc: %s size:%d\n", str, chnkSize);

                    OutputDebugString(outstr);
                }

                if(chnkSize < 0)  // Huh? something has gone wrong...
                    chnkSize = 0;

                data += chnkSize + 8;

            break;
        }
    }

    return 1;
}
