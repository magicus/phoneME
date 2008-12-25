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

#define CHUNKID_RIFF 0x46464952
#define TYPE_WAVE    0x45564157
#define CHUNKID_data 0x61746164
#define CHUNKID_fmt  0x20746D66
#define CHUNKID_LIST 0x5453494C
#define TYPE_INFO    0x4F464E49
#define INFOID_IART  0x54524149
#define INFOID_ICOP  0x504F4349
#define INFOID_ICRD  0x44524349
#define INFOID_INAM  0x4D414E49
#define INFOID_ICMT  0x544D4349
#define INFOID_ISFT  0x54465349
