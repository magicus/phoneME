/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

/** 
 * @file rtp.h
 *
 * Interface definitions of the Mobile Media RTP stack.
 */

#ifdef CDC
#include "com_sun_mmedia_RTPPlayer.h"
#endif

#ifndef WIN32
/**
 * Type definition of RTP_BOOL.
 */
typedef unsigned char RTP_BOOL;

/**
 * Type definition of RTP_WORD.
 */
typedef unsigned int RTP_WORD;

/**
 * Type definition of RTP_SHORT.
 */
typedef unsigned short RTP_SHORT;
#else

/**
 * Type definition of RTP_BOOL.
 */
typedef int RTP_BOOL;

/**
 * Type definition of RTP_WORD.
 */
typedef unsigned short RTP_WORD;

/**
 * Type definition of RTP_BOOL.
 */
typedef short RTP_SHORT;
#endif

/**
 * Type definition of RTP_BYTE.
 */
typedef unsigned char RTP_BYTE;


#ifndef TRUE
/** 
 * @def TRUE
 *
 * Definition of TRUE.
 */
#define TRUE 1
#endif

#ifndef FALSE
/** 
 * @def FALSE
 *
 * Definition of FALSE.
 */
#define FALSE 0       
#endif

#ifndef NULL
/** 
 * @def NULL
 *
 * Definition of NULL.
 */
#define NULL ((void *)0)
#endif


/**
 * The RTP debug flag
 *
 * #define RTP_DEBUG
 */

/** 
 * @def RTP_SUCCESS
 *
 * Return value indicating that this function executed
 * successfully.
 */

#define RTP_SUCCESS 1

/** 
 * @def RTP_OUT_OF_MEMORY
 *
 * Return value indicating that an out-of-memory error
 * occurred.
 */

#define RTP_OUT_OF_MEMORY 2

/** 
 * @def RTP_WRITE_AUDIO_FAILURE
 *
 * Return value indicating that a fatal error occurred
 * when trying to write to the audio device.
 */

#define RTP_WRITE_AUDIO_FAILURE 3

/** 
 * @def RTP_OPEN_AUDIOSTREAM_FAILURE
 * Return value indicating that the audio stream
 * could not be opened.
 */

#define RTP_OPEN_AUDIOSTREAM_FAILURE 4

/** 
 * @def RTP_DECODER_FAILURE
 *
 * Return value indicating that the decoder
 * failed to decode the frame.
 */

#define RTP_DECODER_FAILURE 5

/** 
 * @def RTP_UNKNOWN_PAYLOAD_TYPE
 *
 * Return value indicating that the payload
 * type is unknown cannot be decoded.
 */

#define RTP_UNKNOWN_PAYLOAD_TYPE 6

/** 
 * @def RTP_INVALID_VERSION
 *
 * Return value indicating that this is
 * not a valid RTP version.
 *
 * The expected version is Version 2,
 * the second draft version of RTP as
 * defined in RFC 1889.
 */

#define RTP_INVALID_VERSION 7

/** 
 * @def RTP_CREATE_UDP_SOCKET_FAILURE
 *
 * Return value indicating that the UDP
 * socket for this RTP session could
 * not be created successfully.
 */

#define RTP_CREATE_UDP_SOCKET_FAILURE 8

/** 
 * @def RTP_BIND_UDP_SOCKET_FAILURE
 *
 * Return value indicating that the
 * RTP connector module could not bind
 * to the UDP socket it just created.
 */

#define RTP_BIND_UDP_SOCKET_FAILURE 9

/** 
 * @def RTP_JOIN_MULTICAST_GROUP_FAILURE
 *
 * Return value indicating that the
 * RTP connector module could join
 * the multicast group.
 */

#define RTP_JOIN_MULTICAST_GROUP_FAILURE 10

/** 
 * @def RTP_BAD_FORMAT
 *
 * Return value indicating that a malformed
 * RTP packet has been encountered.
 */

#define RTP_BAD_FORMAT 11

/** 
 * @def RTP_MALFORMED_RTCP_PACKET
 *
 * Return value indicating that a malformed
 * RTCP packet has been encountered.
 *
 * This error is returned if the RTCP packet
 * does not contain a CNAME element.
 */

#define RTP_MALFORMED_RTCP_PACKET 12

/** 
 * @def RTP_UNKNOWN_PACKET_TYPE
 *
 * Return value indicating that an unknown
 * RTCP packet has been encountered.
 */

#define RTP_UNKNOWN_PACKET_TYPE 13

/** 
 * @def RTP_MTU_SIZE
 *
 * Defines the Maximum Transmission Unit (packet size in bytes).
 * For 802.11 the MTU size is 1492 bytes, the Ethernet MTU
 * size is defined as 1500 bytes.
 */

#define RTP_MTU_SIZE 1492

/** 
 * @def RTP_PT_PCMU
 *
 * Payload type indicating RTP/PCMU.
 */
#define RTP_PT_PCMU   0


/** 
 * @def RTP_PT_GSM
 *
 * Payload type indicating RTP/GSM.
 */

#define RTP_PT_GSM    3

/** 
 * @def RTP_PT_MPA
 *
 * Payload type indicating RTP/MPEG-Audio.
 */

#define RTP_PT_MPA   14

/** 
 * @def RTP_PT_JPEG
 *
 * Payload type indicating RTP/JPEG.
 */

#define RTP_PT_JPEG  26

/** 
 * @def RTP_PT_H263
 *
 * Payload type indicating RTP/H.263.
 */

#define RTP_PT_H263  34


/**
 * @struct RTP_CONNECTOR_
 *
 * Definition of the RTP_CONNECTOR_ structure.
 */

/**
 * @var typedef struct RTP_CONNECTOR
 *
 * Type definition of RTP_CONNECTOR.
 */

typedef struct RTP_CONNECTOR_ {
    /** 
     * File descriptor for the RTP socket.
     */
    int rtp_socket;

    /** 
     * File descriptor for the RTCP socket. 
     */
    int rtcp_socket;

    /**
     * The receive buffer for RTP and RTCP packets. 
     */
    RTP_BYTE  recv_buf[RTP_MTU_SIZE]; 
} RTP_CONNECTOR;


/* Forward declaration of RTP_MEMORY_ structure */
struct RTP_MEMORY_;

/* Forward declaration of RTP_DECODER_ structure */
struct RTP_DECODER_;

/** 
 * Writes the decoder's audio output to the audio device.
 *
 * This is a callback function implemented by audio codecs.
 * The RTP stack calls it in case that not all audio data
 * has been written in a previous decoding cycle.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_decoder  Pointer to the RTP_DECODER structure.
 * @return             Returns RTP_SUCCESS if the write call
 *                     to the audio device succeeded, otherwise
 *                     RTP_WRITE_AUDIO_FAILURE. 
 */

int writeAudioFn(struct RTP_MEMORY_ *rtp_memory, 
		 struct RTP_DECODER_ *rtp_decoder); 


/**
 * Type definition of RTP_DECODER.
 */
typedef struct RTP_DECODER_ {
    /**
     * Pointer to decoder state. 
     */
    void *state;

    /**
     * Pointer to the decoder's write-audio function.
     */
    int (*writeAudioFn) (struct RTP_MEMORY_ *, struct RTP_DECODER_ *);  

    /**
     * Pointer to the decoder's close function.
     */
    void  (*closeFn) (void *);
} RTP_DECODER;

/**
 * Type definition of RTP_SSRC.
 */
typedef struct RTP_SSRC_ {
    /**
     * The synchronization source.
     */
    RTP_WORD  ssrc;

    /** 
     * canonical end-point identifier.
     */
    RTP_BYTE *cname;    

    /**
     * Identifies the format of the RTP payload and
     * determines its interpretation by the application.
     */
    int payload_type;

    /** 
     * A pointer to a decoder for this source
     * stream. 
     */
    RTP_DECODER *decoder;

    /**
     * Pointer to next synchronization source in the list.
     */
    struct RTP_SSRC_ *next;
} RTP_SSRC;

/** 
 * Minimum RTP buffer size.
 *
 * Initial buffer size allocated by rtp_memory and
 * used for parsing of RTP and RTCP packets.
 *
 * This data segment will increase in size
 * dynamically, if needed.
 */
#define RTP_MIN_BUFFER_SIZE 2000

/** 
 * Minimum RTP data buffer size.
 *
 * Initial size allocated by rtp_memory and
 * used for storage of RTP session relevant data.
 *
 * This data segment will increase in size
 * dynamically, if needed.
 */
#define RTP_MIN_DATA_SIZE   4000

/**
 * Type definition of RTP_MEMORY.
 */
typedef struct RTP_MEMORY_ {
    /**
     * The buffer offset.
     */
    RTP_WORD   buffer_offset;

    /**
     * The buffer size.
     */
    RTP_WORD   buffer_size;

    /**
     * Buffer used to disassemble one RTP or RTCP packet at
     * any given time.It holds all data structures allocated by 
     * the rtp_receiver and rtcp_receiver modules. 
     */
    RTP_BYTE  *buffer;

    /**
     * The data buffer offset.
     */    
    RTP_WORD   data_offset;

    /**
     * The data buffer size.
     */   
    RTP_WORD   data_size;

    /**
     * The data buffer holds session relevant data, for 
     * example ssrc table entries. 
     */
    RTP_BYTE  *data;

    /**
     * The ssrc table.
     */
    RTP_SSRC *ssrc_table;   

    /** 
     * audio device ready flag
     */
    RTP_BOOL audio_ready;

    /**
     * video frame ready flag
     */
    RTP_BOOL frame_ready;

    /**
     * Pointer to the RTP connector 
     */
    RTP_CONNECTOR *connector;
} RTP_MEMORY;


/**
 * Packet type definition of RTCP Sender Reports.
 *
 * Contains the constant 200 to identify this as an RTCP SR packet.
 */
#define RTCP_SR 200

/**
 * Packet type definition of RTCP Receiver Reports.
 *
 * Contains the constant 201 to identify this as an RTCP RR packet.
 */
#define RTCP_RR 201

/**
 * Packet type definition of RTCP Sender Descriptions.
 *
 * Contains the constant 202 to identify this as an RTCP SDES packet.
 */
#define RTCP_SDES 202

/**
 * Packet type definition of RTCP Bye Packets.
 *
 * Contains the constant 203 to identify this as an RTCP BYE packet.
 */
#define RTCP_BYE  203

/**
 * Packet type definition of RTCP Application Packets.
 *
 * Contains the constant 204 to identify this as an RTCP APP packet.
 */
#define RTCP_APP  204

/**
 * Type definition of RTP_PACKET.
 */
typedef struct RTP_PACKET_ {
    /**
     * Identifies the format of the RTP payload and
     * determines its interpretation by the application.
     */
    RTP_BYTE   payload_type;

    /**
     * The interpretation of the marker is defined by a profile. It is
     * intended to allow significant events such as frame boundaries to
     * be marked in the packet stream. A profile may define additional
     * marker bits or specify that there is no marker bit by changing
     * the number of bits in the payload type field.
     */
    RTP_BYTE   marker;

    /**
     * The sequence number.
     *
     * The sequence number increments by one for each RTP data packet
     * sent, and may be used by the receiver to detect packet loss and
     * to restore packet sequence.
     */
    RTP_SHORT  seqnum;

    /**
     * The extension flag.
     *
     * If the extension bit is set, the fixed header is followed by
     * exactly one header extension.
     */
    RTP_BOOL   extension_present;

    /**
     * The extension type.
     */
    RTP_SHORT  extension_type;

    /**
     * The extension length in number of bytes.
     */
    RTP_WORD   extlen;

    /**
     * Pointer to the RTP header extension.
     */
    RTP_BYTE  *extension;

    /**
     * The timestamp reflects the sampling instant of the first octet
     * in the RTP data packet. The sampling instant must be derived
     * from a clock that increments monotonically and linearly in time
     * to allow synchronization and jitter calculations.
     */
    RTP_WORD   timestamp;

    /**
     * The synchronization source.
     */
    RTP_WORD   ssrc;

    /**
     * An array of contributing sources.
     */
    RTP_WORD  *csrc;

    /**
     * Payload length in bytes.
     */
    RTP_WORD   payloadlength;

    /**
     * Pointer to RTP payload.
     */
    RTP_BYTE  *payload;
} RTP_PACKET;

/**
 * Type definition of PAYLOAD.
 */
typedef struct PAYLOAD_ {
    /**
     * Payload name.
     */
    RTP_BYTE *name;

    /**
     * Payload type.
     */
    RTP_BYTE  type;
} PAYLOAD;

/**
 * Type definition of RTCP_REPORT_BLOCK.
 */
typedef struct RTCP_REPORT_BLOCK_ {
    /**
     * The synchronization source.
     */
    RTP_WORD  ssrc;

    /**
     * The fraction of RTP data packets from source SSRC_n lost since
     * the previous SR or RR packet was sent.
     */
    RTP_BYTE  fractionlost;

    /**
     * The total number of RTP data packets from source SSRC_n that
     * have been lost since the beginning of reception.
     */
    RTP_WORD  packetslost;

    /**
     * The highest sequence number received in an RTP data packet 
     * from source SSRC_n.
     */
    RTP_WORD  lastseq;

    /**
     * An estimate of the statistical variance of the RTP data packet
     * interarrival time, measured in timestamp units and expressed as
     * an unsigned integer. The interarrival jitter J is defined to be
     * the mean deviation (smoothed absolute value) of the difference D
     * in packet spacing at the receiver compared to the sender for a
     * pair of packets.
     */
    RTP_WORD  jitter;

    /**
     * The middle 32 bits out of 64 in the NTP timestamp (as explained
     * in Section 4) received as part of the most recent RTCP sender
     * report (SR) packet from source SSRC_n.  If no SR has been
     * received yet, the field is set to zero.
     */
    RTP_WORD  lsr;

    /**
     * The delay, expressed in units of 1/65536 seconds, between
     * receiving the last SR packet from source SSRC_n and sending this
     * reception report block.  If no SR packet has been received yet
     * from SSRC_n, the DLSR field is set to zero.
     */
    RTP_WORD  dlsr;
} RTCP_REPORT_BLOCK;


/**
 * RTCP source description item: canonical 
 * end-point identifier.
 */
#define SDES_CNAME 1

/**
 * RTCP source description item: user name
 */
#define SDES_NAME  2

/**
 * RTCP source description item: electronic 
 * mail address
 */
#define SDES_EMAIL 3

/**
 * RTCP source description item: phone number
 */
#define SDES_PHONE 4

/**
 * RTCP source description item: geographic
 * user location.
 */
#define SDES_LOC   5

/**
 * RTCP source description item: application or
 * tool name.
 */
#define SDES_TOOL  6

/**
 * RTCP source description item: notice/status.
 */
#define SDES_NOTE  7

/**
 * RTCP source description item: private extensions.
 */
#define SDES_PRIV  8

/**
 * Type definition of the source description item.
 */
typedef struct RTCP_SDES_ITEM_ {
    /**
     * The SDES type.
     */
    RTP_WORD  type;

    /**
     * Pointer to the SDES data.
     */
    RTP_BYTE *data;

    /**
     * Pointer to the next SDES item.
     */
    struct RTCP_SDES_ITEM_ *next;
} RTCP_SDES_ITEM;


/**
 * Type definition of RTCP_SDES_CHUNK.
 */
typedef struct RTCP_SDES_CHUNK_ {
    /**
     * The synchronization source.
     */
    RTP_WORD ssrc;

    /**
     * A list of source description items.
     */
    RTCP_SDES_ITEM *items;
} RTCP_SDES_CHUNK;
 
/**
 * Type definition of RTCP_SDES_PACKET.
 */
typedef struct RTCP_SDES_PACKET_ {
    /**
     * A list of source description chunks.
     */
    RTCP_SDES_CHUNK *chunks;
} RTCP_SDES_PACKET;

/**
 * Type definition of RTCP_BYE_PACKET.
 */
typedef struct RTCP_BYE_PACKET_ {
    /**
     * The number of SSRC/CSRC identifiers included in this BYE packet.
     * A count value of zero is valid, but useless.
     */
    RTP_WORD *ssrcs;

    /**
     * Reason for leaving.
     */
    RTP_BYTE *reason;
} RTCP_BYE_PACKET;

/**
 * Type definition of RTCP_RR_PACKET.
 */
typedef struct RTCP_RR_PACKET_ {
    RTP_WORD ssrc;
    RTCP_REPORT_BLOCK *reports;
} RTCP_RR_PACKET;

/**
 * Type definition of RTCP_APP_PACKET.
 */
typedef struct RTCP_APP_PACKET_ {
    /**
     * The synchronization or contributing source.
     */
    RTP_WORD  ssrc;

    /**
     * A name chosen by the person defining the set of APP packets to
     * be unique with respect to other APP packets this application
     * might receive. The application creator might choose to use the
     * application name, and then coordinate the allocation of subtype
     * values to others who want to define new packet types for the
     * application.  Alternatively, it is recommended that others
     * choose a name based on the entity they represent, then
     * coordinate the use of the name within that entity.
     */
    RTP_WORD  name;

    /**
     * May be used as a subtype to allow a set of APP packets to be
     * defined under one unique name, or for any application-dependent
     * data.
     */
    RTP_WORD  subtype;

    /**
     * Pointer to APP data.
     */
    RTP_BYTE *data;
} RTCP_APP_PACKET;

/**
 * Type definition of RTCP_SR_PACKET.
 */
typedef struct RTCP_SR_PACKET_ {
    /**
     * The synchronization source.
     */
    RTP_WORD ssrc;

    /**
     * NTP timestamp: 64 bits (most significant word)
     *
     * Indicates the wallclock time when this report was sent so that
     * it may be used in combination with timestamps returned in
     * reception reports from other receivers to measure round-trip
     * propagation to those receivers. Receivers should expect that the
     * measurement accuracy of the timestamp may be limited to far less
     * than the resolution of the NTP timestamp. The measurement
     * uncertainty of the timestamp is not indicated as it may not be
     * known. A sender that can keep track of elapsed time but has no
     * notion of wallclock time may use the elapsed time since joining
     * the session instead. This is assumed to be less than 68 years,
     * so the high bit will be zero. It is permissible to use the
     * sampling clock to estimate elapsed wallclock time. A sender that
     * has no notion of wallclock or elapsed time may set the NTP
     * timestamp to zero
     */
    RTP_WORD ntptimestampmsw;

    /**
     * NTP timestamp: 64 bits (least significant word)
     */
    RTP_WORD ntptimestamplsw;

    /**
     * RTP timestamp: 32 bits
     *
     * Corresponds to the same time as the NTP timestamp (above), but
     * in the same units and with the same random offset as the RTP
     * timestamps in data packets. This correspondence may be used for
     * intra- and inter-media synchronization for sources whose NTP
     * timestamps are synchronized, and may be used by media-
     * independent receivers to estimate the nominal RTP clock
     * frequency. Note that in most cases this timestamp will not be
     * equal to the RTP timestamp in any adjacent data packet. Rather,
     * it is calculated from the corresponding NTP timestamp using the
     * relationship between the RTP timestamp counter and real time as
     * maintained by periodically checking the wallclock time at a
     * sampling instant.
     */
    RTP_WORD rtptimestamp;

    /**
     * sender's packet count: 32 bits
     *
     * The total number of RTP data packets transmitted by the sender
     * since starting transmission up until the time this SR packet was
     * generated.  The count is reset if the sender changes its SSRC
     * identifier.
     */
    RTP_WORD packetcount;

    /**
     * sender's octet count: 32 bits
     *
     * The total number of payload octets (i.e., not including header
     * or padding) transmitted in RTP data packets by the sender since
     * starting transmission up until the time this SR packet was
     * generated. The count is reset if the sender changes its SSRC
     * identifier. This field can be used to estimate the average
     * payload data rate.
     */
    RTP_WORD octetcount;

    /** 
     * The report blocks sent in this SR 
     */
    RTCP_REPORT_BLOCK *reports;
} RTCP_SR_PACKET;

/**
 * Type definition of RTCP_PACKET.
 */
typedef struct RTCP_PACKET_ {
    /**
     * The RTCP packet type.
     */
    RTP_BYTE type;

    /** 
     * RTCP packet specific content.
     *
     * The RTCP Packet contains one of the following
     * components: RTCP_RR_PACKET, RTCP_SR_PACKET,
     * RTCP_SDES_PACKET, RTCP_BYE_PACKET or RTCP_APP_PACKET.
     */
    union {
        /**
         * RTCP Receiver Report.
         */
        RTCP_RR_PACKET rr;

        /**
         * RTCP Sender Report.
         */
        RTCP_SR_PACKET sr;

        /**
         * RTCP Sender Description.
         */
	RTCP_SDES_PACKET sdes;

        /**
         * RTCP Bye Packet.
         */
	RTCP_BYE_PACKET bye;

        /**
         * RTCP Application Packet.
         */
	RTCP_APP_PACKET app;
    } packet;
} RTCP_PACKET;


/** 
 * Parses an RTP packet.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param packet       Pointer to a byte array containing an RTP packet.
 * @param length       The length of the RTP packet.
 * @return             Returns RTP_SUCCESS if the RTP packet was
 *                     processed successfully. Other return values are
 *                     RTP_OUT_OF_MEMORY if not enough memory for the RTP
 *                     packet could be allocated, RTP_INVALID version if
 *                     the the version could not be handled and RTP_BAD_FORMAT
 *                     if a malformed RTP packet has been encountered.
 */

int rtpr_parse_packet(RTP_MEMORY *rtp_memory, 
		      RTP_BYTE *packet, RTP_WORD length);


/** 
 * Parses an RTCP packet.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param packet       Pointer to a byte array containing an RTCP packet.
 * @param length       The length of the RTCP packet.
 * @return             Returns RTP_SUCCESS if the RTCP packet was
 *                     processed successfully. Other return values are
 *                     RTP_OUT_OF_MEMORY if not enough memory for the RTCP
 *                     packet could be allocated, RTP_INVALID version if
 *                     the the version could not be handled and RTP_BAD_FORMAT
 *                     if a malformed RTCP packet has been encountered.
 */

int rtcpr_parse_packet(RTP_MEMORY *rtp_memory, 
		       RTP_BYTE *packet, RTP_WORD length);

/** 
 * Frees RTP memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 */

void rtp_free_memory(RTP_MEMORY *rtp_memory);

/** 
 * Retrieves the RTP_SSRC structure for the specified
 * synchronization source. A new RTP_SSRC will be created
 * if the ssrc does not exist in the table.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param ssrc         The synchronization source.
 *
 * @return A pointer to the RTP_SSRC element or NULL if
 * it cannot be found or allocated.
 */
RTP_SSRC *rtp_get_ssrc(RTP_MEMORY *rtp_memory, RTP_WORD ssrc);

/** 
 * Allocates and adds a new RTP decoder for the specified synchronization 
 * source and payload type.
 *
 * @param rtp_memory     Pointer to the RTP_MEMORY structure.
 * @param ssrc           The synchronization source.
 * @param payload_type   The payload type.
 */
void rtp_add_decoder(RTP_MEMORY *rtp_memory, RTP_SSRC *rtp_ssrc, 
		     int payload_type);

/** 
 * Resets RTP memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 */
void rtp_reset_memory(RTP_MEMORY *rtp_memory);

/** 
 * Allocates a block of RTP memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param size         Size in number of bytes to be allocated.
 * @return             Returns a pointer to the allocated data
 *                     segement or NULL if the memory allocation
 *                     failed.
 * @todo Needs work (see comments).
 */
void *rtp_get_memory(RTP_MEMORY *rtp_memory, RTP_WORD size);

/** 
 * Initializes RTP memory.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @return             Returns RTP_SUCCESS if the could be 
 *                     allocated, RTP_OUT_OF_MEMORY otherwise.
 */

int rtp_init_memory(RTP_MEMORY **rtp_memory);

/** 
 * Decodes the RTP payload according to its payload type.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param rtp_packet   Pointer to the RTP_PACKET structure.
 * @return             Returns RTP_SUCCESS if the payload is
 *                     decoded successfully. RTP_OUT_OF_MEMORY
 *                     indicates that the respective decoder
 *                     memory could not be allocated. An 
 *                     RTP_DECODER_FAILURE is returned if the
 *                     decoding of the payload fails.
 */

int rtp_demux_payload(RTP_MEMORY *rtp_memory, RTP_PACKET *rtp_packet);


/** 
 * Writes audio data to the decoder's audio output.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @return             Returns RTP_SUCCESS if the write call
 *                     to the audio device succeeded, otherwise
 *                     RTP_WRITE_AUDIO_FAILURE.  
 */

int rtp_write_audio(RTP_MEMORY *rtp_memory);


/** 
 * Links a CNAME to the respective entry in the the SSRC table.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param ssrc_val     The value of the SSRC in the SSRC table.
 * @param cname_data   A byte array representing a CNAME.
 * @param cname_length The length of the CNAME byte array.
 *
 * @todo rtp_map_ssrc should return RTP_SUCCESS or RTP_OUT_OF_MEMORY. 
 */

void rtp_map_ssrc(RTP_MEMORY *rtp_memory, RTP_WORD ssrc_val, 
		  RTP_BYTE *cname_data, RTP_WORD cname_length);


/** 
 * Removes an SSRC from the SSRC table.
 *
 * @param rtp_memory   Pointer to the RTP_MEMORY structure.
 * @param ssrc_val     The value of the ssrc to be removed.
 */

void rtp_remove_ssrc(RTP_MEMORY *rtp_memory, RTP_WORD ssrc_val);
