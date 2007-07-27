/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */
package com.sun.mmedia;

import javax.microedition.media.*;
import javax.microedition.media.control.VideoControl;

/**
 * A player for Real-time Transport Protocol streams.
 *
 * @created    September 11, 2002
 */
public final class RTPPlayer extends BasicPlayer implements Runnable {
    /**
     * RTP error codes reported by the native peer.
     *
     * Please note: these values are defined in rtp.h. The
     * Java definitions must match those in rtp.h.
     */
    private final int RTP_SUCCESS = 1;


    /**
     * RTP error code set by native code.
     *
     * This variable must be called 'rtp_error' for native
     * library to find it.
     */
    protected int rtp_error = RTP_SUCCESS;


    /**
     * The RGB buffer.
     *
     * This variable must be called 'rgbBuffer' for native
     * library to find it.
     */
    protected byte[] rgbBuffer = null;


    /**
     * The video frame width.
     *
     * This variable is set by 'nGetFrameSize' and must be called
     * 'frameWidth' for native library to find it.
     */
    protected int frameWidth;


    /**
     * The video frame height.
     *
     * This variable is set by 'nGetFrameSize' and must be called
     * 'frameHeight' for native library to find it.
     */
    protected int frameHeight;


    /**
     * Opens the RTP connector.
     *
     * @param  host       The hostname, i.e. 129.145.166.64
     * @param  port       The port (default port = 554)
     * @param  multicast  True, if this is this player subscribes
     *                    to a multicast group.
     * @return            Returns a pointer to the native peer.
     */
    private native int nConnectorOpen(String host, int port,
                                      boolean multicast);


    /**
     * Closes RTP connector.
     *
     * @param  peer       Pointer to the native peer.
     * @param  nativeRGB  Description of the Parameter
     */
    private native void nConnectorClose(int peer, int nativeRGB);


    /**
     * Receives RTP packets.
     *
     * @param  peer  Pointer to the native peer.
     * @return       The RTP error code.
     */
    private native int nReceiveRTP(int peer);


    /**
     * Receives RTP packets.
     *
     * @param  peer  Pointer to the native peer.
     */
    private native void nReceiveRTCP(int peer);


    /**
     * Gets the payload type from the RTP connector.
     *
     * @param  peer  Pointer to the native peer.
     * @return       The payload type.
     */
    private native int nGetPayloadType(int peer);


    /**
     * Copies the native frame buffer into the RGB buffer.
     *
     * @param  peer       Pointer to the native peer.
     * @param  buffer     The RGB buffer.
     * @param  nativeRGB  A native pointer to the RGB buffer
     * @return            True if a frame has been copied into the
     *                    RGB buffer, otherwise false.
     */
    private native boolean nCopyRGBBuffer(int peer, byte buffer[],
                                          int nativeRGB);


    /**
     * Retrieves the frame size into frameWidth and frameHeight.
     *
     * @param  peer  Pointer to the native peer.
     * @return       Returns true, if the frame size was obtained
     *               successfully, otherwise false.
     */
    private native boolean nGetFrameSize(int peer);


    /**
     * Resets the native peer.
     *
     * @param  peer  Pointer to the native peer.
     */
    private native void nReset(int peer);


    /**
     * Determines if native rendering is enabled.
     *
     * @return true if native rendering is enabled, otherwise false.
     */
    private native boolean nNativeRendering();


    /**
     * Pointer to the RTP connector.
     */
    private int peer;


    /**
     * State variable indicates that the play thread is running.
     */
    private boolean started;

    /**
     * Tells the play thread to abort if set to true.
     */
    private boolean interrupted;

    /**
     * The play thread.
     */
    private Thread playThread;

    /**
     * The video control object.
     */
    private VideoRenderer videoControl;

    /**
     * The volume control object.
     */
    private VolCtrl volumeControl;

    /**
     * Native rendering flag.
     *
     * If set to true video is not passed up to the Java
     * layer but rendered directly to the screen by
     * the native decoder.
     */
    private boolean nativeRendering = false;

    /**
     *  PCMU payload type.
     */
    private final static int PT_PCMU = 0;

    /**
     *  MPEG-Audio payload type.
     */
    private final static int PT_MPA = 14;

    /**
     *  JPEG payload type.
     */
    private final static int PT_JPEG = 26;

    /**
     *  H.263 payload type.
     */
    private final static int PT_H263 = 34;

    private Object playLock = new Object();

    /**
     * Native video transfer flag.
     *
     * true means that video is not copied to the Java layer but
     * directly transferred through native code.
     *
     * Needs to be package private, set by native code.
     */
    boolean nativeVideoTransfers = false;

    /**
     * Pointer to native RGB buffer.
     */
    int nativeRGB = 0;

    /**
     *  the RTP payload type
     */
    private int payload_type;

    /**
     *  the RTP debug flag
     */
    private static boolean RTP_DEBUG = false;


    /**
     * Initialized flag.
     *
     * True if RTP data has been received and the RTP player
     * could be initialized successfully, otherwise false.
     */
    private boolean initialized = false;


    /**
     * Timeout value in milliseconds.
     *
     * This is the duration the RTPPlayer waits for data to arrive
     * before it aborts the initialization phase.
     */
    private static final int TIMEOUT = 10000;


    /**
     * Realizes the RTP player.
     *
     * @exception  MediaException  Throws a MediaException if either
     *                             the RTP connector cannot be opened
     *                             or if the RTP connection times out,
     *                             i.e. no data packets have been
     *                             received within 'TIMEOUT'.
     */
    protected void doRealize() throws MediaException {
	if (RTP_DEBUG) System.err.println( "[RTPPlayer] doRealize");

	initialized = false;

        String locator = source.getLocator();

        String host = getHost(locator);
        int port = getPort(locator);


	nativeRendering = nNativeRendering();

        // TODO:
        // The multicast check below is not available
        // on MIDP, needs to be handled in native code
        boolean multicast = false;

        peer = nConnectorOpen(host, port, multicast);

        if (rtp_error != RTP_SUCCESS) {
            if (RTP_DEBUG) {
                System.err.println("ERROR: Failure in nConnectorOpen!!!");
            }
            throw new MediaException("cannot open RTP connector");
        }

        /*
         *  try {
         *  boolean multicast = InetAddress.getByName(host).isMulticastAddress();
         *  peer = nConnectorOpen(host, port, multicast);
         *  } catch (Exception e) {
         *  System.err.println("Unknown host: " + host);
         *  }
         */


	new InitThread(this, TIMEOUT).start();

	synchronized (playLock) {
	    try {
		playLock.wait();
	    } catch (InterruptedException e) {
	    }
	}

	if (RTP_DEBUG) System.err.println( "[RTPPlayer] initialized: " + initialized);

        if (!initialized) {
            throw new MediaException("no RTP data received");
        }

	if (RTP_DEBUG) System.err.println( "[RTPPlayer] doRealize done");
    }


    /**
     * Prefetches the RTP Player.
     *
     * This method is empty since the players have been fully
     * initialized during realize time. RTP needs to receive
     * some data before it can determine the payload type and
     * export the respective controls. Consequently there is
     * nothing to do during prefetch time.
     *
     * @exception  MediaException  Description of the Exception
     */
    protected void doPrefetch() throws MediaException {
    }


    /**
     * Gets the duration of the media stream.
     *
     * @return  The duration in microseconds or <code>TIME_UNKNOWN</code>.
     */
    public long doGetDuration() {
        return TIME_UNKNOWN;
    }


    /**
     * Starts the RTP Player.
     *
     * @return  Returns true if the player was started successfully,
     *          otherwise false.
     */
    protected boolean doStart() {
        if (RTP_DEBUG) System.err.println("[RTPPlayer] doStart");

        if (!started) {
	    if (payload_type == PT_MPA) {
		// reset the native MPA decoder
		nReset(peer);
	    }

	    if (playThread == null) {
		playThread = new Thread(this);

		if (playThread != null) {
		    // set to almost MAX_PRIORITY
		    playThread.setPriority(Thread.NORM_PRIORITY
		                           + ((Thread.MAX_PRIORITY
		                              - Thread.NORM_PRIORITY) * 4) / 5);
		    playThread.start();
		    started = true;
		} else {
		    started = false;
		}
	    } else {
		synchronized (playLock) {
		    playLock.notifyAll();
		    started = true;
		}
	    }
	}

        if (RTP_DEBUG) System.err.println("[RTPPlayer] doStart done");

        return started;
    }


    /**
     * Stops the RTP Player.
     *
     * @see                      Player#stop()
     * @exception MediaException Thrown if the <code>Player</code> cannot
     *                           be stoppped.
     */
    protected void doStop() throws MediaException {
	if (RTP_DEBUG) System.err.println("[RTPPlayer] doStop");
        started = false;

	synchronized (playLock) {
	    try {
		if (playThread != null) {
		    playLock.notifyAll();
		    playLock.wait();
		}
	    } catch(Exception e) {
		throw new MediaException (e.getMessage());
	    }
	}

	if (RTP_DEBUG) System.err.println("[RTPPlayer] doStop done");
    }


    /**
     * Deallocates the RTP Player.
     */
    protected void doDeallocate() {
	if (RTP_DEBUG) System.err.println( "[RTPPlayer] doDeallocate");
        started = false;
	interrupted = true;

	if (playThread != null) {
	    try {
		synchronized (playLock) {
		    playLock.notifyAll();
		    playLock.wait();
		}
	    } catch (Exception e) {
	    }
	}

        playThread = null;

        nConnectorClose(peer, nativeRGB);

	if (RTP_DEBUG) System.err.println( "[RTPPlayer] doDeallocate done");
    }


    /**
     * Closes the RTP Player.
     */
    protected void doClose() {
	if (RTP_DEBUG) System.err.println( "[RTPPlayer] doClose");
        started = false;
	interrupted = true;
	if (RTP_DEBUG) System.err.println( "[RTPPlayer] doClose done");
    }


    /**
     * Sets the media time of the RTP Player.
     *
     * @param  now the new media time.
     * @exception MediaException thrown if setMediaTime fails.
     * @return the new media time in microseconds.
     */
    protected long doSetMediaTime(long now) throws MediaException {
        return now;
    }


    /**
     * Retrieves the current media time.
     *
     * @return  the media time in microseconds.
     */
    public long doGetMediaTime() {
        return TIME_UNKNOWN;
    }


    /**
     * Retrieves the specified control object for the
     * RTP Player.
     *
     * The following controls are currently implemented:
     * VideoControl and VolumeControl.
     *
     * @param  type       the requested control type.
     * @return            the control object if available,
     *                    otherwise null.
     */
    protected Control doGetControl(String type) {
        if (/*(getState() != UNREALIZED) && */
            type.startsWith(BasicPlayer.pkgName)) {
            
            type = type.substring(BasicPlayer.pkgName.length());
            
            if (type.equals(BasicPlayer.vicName)) {
                return videoControl;
            } else if (type.equals(BasicPlayer.vocName)) {
                return volumeControl;
            }
        }
        return null;
    }


    /**
     * Main processing method for the RTPPlayer object
     */
    public void run() {
	if (RTP_DEBUG) System.err.println( "[RTPPlayer] enter run");

	interrupted = false;

	while (true) {
	    while (started) {
		// trying to receive an RTP packet...
		int read = nReceiveRTP(peer);

		// check if any RTCP packets have arrived...
		// nReceiveRTCP(peer);
		if (!nativeRendering && read > 0) {
		    if (started && nCopyRGBBuffer(peer, rgbBuffer, nativeRGB)) {
			if (nativeVideoTransfers) {
			    // ToDo: disabled for now, mo
			    // videoControl.render(nativeRGB);
			} else {
			    // ToDo: disabled for now, mo
			    // videoControl.render(rgbBuffer);
			}

			try {
			    Thread.sleep(10);
			} catch (InterruptedException e) {
			}
		    }
		}
            }

            synchronized (playLock) {
                if (interrupted) {
                    break;
                }

		playLock.notifyAll();

                try {
		    if (RTP_DEBUG) System.err.println("[RTPPlayer] run loop is paused");
                    playLock.wait();
                } catch (Exception e) {}
            }
        }

        synchronized (playLock) {
            playLock.notify();
        }

	if (RTP_DEBUG) System.out.println("[RTPPlayer] exit run");
    }


    /**
     * Gets the server host address.
     *
     * @param  url  The RTP URL
     * @return      The host address value
     */
    private String getHost(String url) {
        String host = null;

        int idx = url.indexOf("//");

        if (idx != -1) {
            String str = url.substring(idx + 2);

            idx = str.indexOf(':');

            if (idx == -1) {
                idx = str.indexOf('/');

                if (idx == -1) {
                    host = str;
                } else {
                    host = str.substring(0, idx);
                }
            } else {
                host = str.substring(0, idx);
            }
        }

        return host;
    }


    /**
     * Gets the port attribute of the RTPPlayer object
     *
     * @param  url  Description of the Parameter
     * @return      The port value
     */
    private int getPort(String url) {
        int port = -1;

        int start = url.indexOf("//");
        int afterPort;

        if (start != -1) {
            String str = url.substring(start + 2);

            start = str.indexOf(':');
            String portPlus = str.substring(start + 1);
            afterPort = portPlus.indexOf("/");
            if (afterPort == -1) {
                port = Integer.parseInt(portPlus);
            } else {
                port = Integer.parseInt(portPlus.substring(0, afterPort));
            }
        }

        return port;
    }


    /**
     * Initialization thread.
     *
     * Waits for RTP data to arrive and initializes RTP control
     * objects.
     */
    private class InitThread extends Thread implements Runnable {
	private RTPPlayer parent;
	private long timeout;

	public InitThread(RTPPlayer parent, int timeout) {
	    this.parent = parent;
	    this.timeout = timeout;
	}

	public void run() {
	    long startTime = System.currentTimeMillis();

	    while (!initialized) {
		// trying to receive an RTP packet...
		int read = nReceiveRTP(peer);

		// System.out.println("read: " + read);

		if (read > 0) {
		    payload_type = nGetPayloadType(peer);

		    // System.out.println("pt: " + payload_type);

		    if (payload_type == PT_PCMU) {
			volumeControl = new VolCtrl(parent);
			initialized = true;
			break;
		    } else if (payload_type == PT_MPA) {
			volumeControl = new VolCtrl(parent);
			initialized = true;
			break;
		    } else if (payload_type == PT_JPEG || payload_type == PT_H263) {
			initialized = nGetFrameSize(peer);

			if (initialized) {
			    rgbBuffer = new byte[3 * frameWidth * frameHeight + 3];
			    // vidCtrl = new VidCtrl(parent, frameWidth, frameHeight);
			    // vidCtrl.initRendering(VidCtrl.XBGR888, frameWidth, frameHeight);
			    videoControl = Configuration.getConfiguration().getVideoRenderer(parent, frameWidth, frameHeight);

			    videoControl.initRendering(VideoRenderer.XBGR888,
						       frameWidth, frameHeight);

			    // vidCtrl.setMode(VidCtrl.RGB888);
			    break;
			}
		    }
		}

		if (!initialized) {
		    if (System.currentTimeMillis() - startTime > timeout) {
			break;
		    }
		}
	    }

	    synchronized (playLock) {
		playLock.notifyAll();
	    }
	}
    }
}
