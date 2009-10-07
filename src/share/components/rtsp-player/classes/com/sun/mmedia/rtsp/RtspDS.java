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

package com.sun.mmedia.rtsp;

import java.io.IOException;
import java.io.InputStream;
import java.util.Random;
import javax.microedition.media.Player;
import javax.microedition.media.MediaException;
import javax.microedition.media.protocol.SourceStream;

import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;

import com.sun.mmedia.protocol.BasicDS;

import com.sun.mmedia.sdp.*;

public class RtspDS extends BasicDS {

    private static final int RESPONSE_TIMEOUT = 5000;

    private static final int MIN_UDP_PORT = 1024;  // inclusive
    private static final int MAX_UDP_PORT = 65536; // exclusive

    private static Random rnd = new Random(System.currentTimeMillis());

    private Object msgWaitEvent = new Object();
    private RtspIncomingMessage response = null;

    private RtspConnection connection = null;
    private boolean started = false;
    private RtspUrl url = null;
    private int seqNum = 0;
    private String sessionId = null;
    private RtspRange range = null;

    // in seconds, 60 is default according to the spec
    private int sessionTimeout = 60;
    private KeepAliveThread ka_thread;

    // select UDP or inbound interleaved TCP for RTP transport
    // IMPL_NOTE: currently using only TCP, although UDP is also implemented
    private boolean usingUdp = false; 

    private RtspSS[] streams = null;

    private static int nextUdpPort = MIN_UDP_PORT;
    private int nextInterleavedChannel = 0;

    public RtspUrl getUrl() {
        return url;
    }

    public void setLocator(String ml) throws MediaException {
        try {
            url = new RtspUrl(ml);
            super.setLocator(ml);
        } catch (IOException ioe) {
            throw new MediaException(ioe.toString());
        }
    }

    private void setupTrack(SdpSessionDescr sdp, int trk) 
        throws IOException, InterruptedException {

        String mediaControlString = null;

        // used only in UDP mode
        RtpConnection conn = null; 

        // in UDP mode -- UDP prt number;
        // in TCP mode -- interleaved channel number
        int clientPort = allocPort(); 

        if (usingUdp) {
            int firstCP = clientPort;
            do {
                try {
                    conn = new RtpConnection(clientPort);
                    conn.startListening();
                } catch (IOException e) {
                    clientPort = allocPort();
                }
            } while (null == conn && clientPort != firstCP);

            if (null == conn) {
                throw new IOException("Unable to allocate client UDP port");
            }
        }

        SdpMediaDescr md = sdp.getMediaDescription(trk);
        SdpMediaAttr a_control = md.getMediaAttribute("control");
        SdpMediaAttr a_rtpmap = md.getMediaAttribute("rtpmap");

        if (null != a_control) {
            mediaControlString = a_control.getValue();
        }

        if (null != a_rtpmap) {
            new RtpPayloadType(a_rtpmap.getValue());
        }

        RtspOutgoingRequest setupRequest
            = RtspOutgoingRequest.SETUP(seqNum, url, mediaControlString,
                                        sessionId, clientPort, usingUdp);

        if (!sendRequest(setupRequest)) {
            throw new IOException("SETUP request failed");
        }

        if (null == sessionId) {
            sessionId = response.getSessionId();
            Integer timeout = response.getSessionTimeout();
            if (null != timeout) {
                sessionTimeout = timeout.intValue();
            }
        }

        RtspTransportHeader th = response.getTransportHeader();

        if (usingUdp) {
            if (th.getClientDataPort() != clientPort) {
                // Returned value for client data port is different.
                // An attempt is made to re-allocate UDP port accordingly.
                if (null != conn) {
                    conn.stopListening();
                }
                clientPort = th.getClientDataPort();
                conn = new RtpConnection(clientPort);
                conn.startListening();
            }
        }

        streams[trk] = new RtspSS(this);

        if (usingUdp) {
            conn.setSS(streams[trk]);
        }

        if (-1 != md.payload_type) {
            RtpPayloadType pt = RtpPayloadType.get(md.payload_type);
            if (null != pt) {
                streams[trk].setContentDescriptor(pt.getDescr());
            }
        }
    }

    public synchronized void connect() throws IOException {
        if (null == connection) {
            try {
                connection = new RtspConnection(this);

                seqNum = rnd.nextInt();

                if (!sendRequest(RtspOutgoingRequest.DESCRIBE(seqNum, url))) {
                    throw new IOException("RTSP DESCRIBE request failed");
                }

                SdpSessionDescr sdp = response.getSdp();
                if (null == sdp) throw new IOException("no SDP data received");

                SdpMediaAttr range_attr = sdp.getSessionAttribute("range");

                if (null != range_attr) {
                    try {
                        range = new RtspRange(range_attr.getValue());
                    } catch (NumberFormatException e) {
                        range = null;
                    }
                }

                int num_tracks = sdp.getMediaDescriptionsCount();
                if (0 == num_tracks) throw new IOException("no media descriptions received");

                streams = new RtspSS[num_tracks];

                // sessionId is null at this point
                for (int trk = 0; trk < num_tracks; trk++) {
                    setupTrack( sdp, trk );
                }

                start();

                ka_thread = new KeepAliveThread();
                ka_thread.start();

            } catch (InterruptedException e) {
                connection = null;
                Thread.currentThread().interrupt();
                throw new IOException("connect to " +
                                locator + " aborted: " + e.getMessage());
            } catch (IOException e) {
                connection = null;
                throw new IOException("failed to connect to " +
                                locator + " : " + e.getMessage());
            }
        }
    }

    public synchronized void disconnect() {
        if( null != connection ) {
            if (null != ka_thread && ka_thread.isAlive() ) {
                synchronized( ka_thread ) {
                    ka_thread.interrupt();
                }
            }
            try {
                sendRequest( RtspOutgoingRequest.TEARDOWN( seqNum, url, sessionId ) );
            } catch( InterruptedException e ) {
                Thread.currentThread().interrupt();
            } catch (IOException e) {
                if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                    Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                        "IOException in RtspDS.disconnect(): " + e.getMessage());
                }
            } finally {
                connection.close();
                connection = null;
            }
        }
    }

    public synchronized void start() throws IOException {
        if (null == connection) throw new IllegalStateException("RTSP: Not connected");
        if (!started) {
            try {
                started = sendRequest(RtspOutgoingRequest.PLAY(seqNum, url, sessionId));
            } catch (InterruptedException e) {
                throw new IOException("start aborted: " + e.getMessage());
            }
        }
    }

    public synchronized void stop() throws IOException {
        if (null == connection || !started) return;
        try {
            sendRequest(RtspOutgoingRequest.PAUSE(seqNum, url, sessionId));
        } catch (InterruptedException e) {
            throw new IOException("stop aborted: " + e.getMessage());
        }
    }

    public synchronized SourceStream[] getStreams() {
        if (null == connection) throw new IllegalStateException("RTSP: Not connected");
        return streams;
    }

    public synchronized long getDuration() {
        if (null != range) {
            float from = range.getFrom();
            float to = range.getTo();
            if (RtspRange.NOW != from && RtspRange.END != to) {
                return (long)((to - from) * 1.E6);
            }
        }

        return Player.TIME_UNKNOWN;
    }

    public String getContentType() {
        if (null == connection) throw new IllegalStateException("RTSP: Not connected");
        return null;
    }

    //=========================================================================

    private int allocPort() {
        if (usingUdp) {
            int retVal = nextUdpPort;
            nextUdpPort += 2;
            if (nextUdpPort >= MAX_UDP_PORT) {
                nextUdpPort = MIN_UDP_PORT;
            }
            return retVal;
        } else {
            int retVal = nextInterleavedChannel;
            nextInterleavedChannel += 2;
            return retVal;
        }
    }

    //=========================================================================

    /** 
     * This method is called by RtspConnection when RTP/RTCP packet is received.
     * Used only in TCP mode (usingUdp=false).
     */
    protected void processRtpPacket(int channel, byte[] pkt) {
        // Only RTP packets (they arrive on even channels) are processed.
        // RTCP packets (on odd channels) are discarded for now.
        if (0 == channel % 2) {
            int n_stream = channel / 2;
            streams[n_stream].processPacket(new RtpPacket(pkt,pkt.length));
        }
    }

    /** 
     * This method is called by RtspConnection when message is received
     */
    protected void processIncomingMessage(byte[] bytes) {
        synchronized (msgWaitEvent) {
            RtspIncomingMessage msg;
            try {
                msg = new RtspIncomingMessage(bytes);
            } catch (Exception e) {
                if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                    Logging.report(Logging.INFORMATION, LogChannels.LC_MMAPI,
                        "Exception in RtspDS.processIncomingMessage(): " + e);
                }
                msg = null;
            }
            Integer cseq = msg.getCSeq();
            // response may not have CSeq defined if status is not '200 OK'.
            if (null == cseq || cseq.intValue() == seqNum) {
                response = msg;
                msgWaitEvent.notifyAll();
                seqNum++;
            }
        }
    }

    /**
     * blocks until response is received or timeout period expires
     */
    private boolean sendRequest(RtspOutgoingRequest request) 
        throws InterruptedException, IOException {

        boolean ok = false;

        synchronized (msgWaitEvent) {
            response = null;
            if (connection.sendData(request.getBytes())) {
                try {
                    msgWaitEvent.wait(RESPONSE_TIMEOUT);
                    ok = (null != response);
                } catch (InterruptedException e) {
                    throw e;
                }
            }
        }

        if (ok && !response.getStatusCode().equals("200")) {
            throw new IOException("RTSP error " + response.getStatusCode()
                                   + ": '" + response.getStatusText() + "'");
        }

        return ok;
    }

    //=========================================================================

    private class KeepAliveThread extends Thread {
        private boolean terminate;
        public void run() {
            while (!terminate) {
                try {
                    Thread.sleep(3000 * sessionTimeout / 4); // 3/4, in milliseconds
                    synchronized (this) {
                        terminate |= !sendRequest(RtspOutgoingRequest.GET_PARAMETER(seqNum, url, sessionId));
                    }
                } catch (InterruptedException e) {
                    terminate = true;
                } catch (IOException e) {
                    // ignore for now assuming it's '451 Parameter Not Understood'
                } catch (Exception e) {
                    terminate = true;
                }
            }
        }
    }
}
