/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

#define FRAME_BUFFER_INITIAL_SIZE 32000

#define BUFFER_PROCESSED_OK 1
#define OUTPUT_BUFFER_NOT_FILLED 2

typedef struct JPEG_DEPACKETIZER_ {
    RTP_BOOL  gotFirstPacket;
    RTP_SHORT firstSeq;
    char *frameBuffer;
    RTP_WORD  frameBufferLength;
    RTP_WORD  dataLength;
    RTP_SHORT numPkts;
    RTP_BYTE  quality;
    RTP_BYTE  type;
    void *jpgdecoder;
    RTP_BYTE *rgbData;
    RTP_WORD  rgbDataLength;

    int lastQuality;
    int lastType;
    int lastWidth;
    int lastHeight;

    RTP_BYTE *lastJFIFHeader;
    RTP_WORD lastJFIFHeaderLength;
 
    int width;
    int height;
} JPEG_DEPACKETIZER;

JPEG_DEPACKETIZER * jpeq_depacketizer_init();

RTP_BOOL jpeg_gotAllPackets(JPEG_DEPACKETIZER *depacketizer, long lastSeq);

void jpeq_depacketizer_reset(JPEG_DEPACKETIZER *depacketizer, RTP_BYTE *data, RTP_WORD size, RTP_SHORT seqnum);

void jpeg_depacketizer_add(JPEG_DEPACKETIZER *depacketizer,
                            RTP_BYTE *data, RTP_WORD size, int extraskip);

void jpeg_completeTransfer(RTP_MEMORY *rtp_memory,
			   JPEG_DEPACKETIZER *depacketizer,
			   RTP_BYTE *data,
			   RTP_WORD size);

void jpeg_depacketizer_close(JPEG_DEPACKETIZER *depacketizer);
