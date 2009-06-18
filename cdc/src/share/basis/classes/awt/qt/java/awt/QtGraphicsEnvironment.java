/*
 * @(#)QtGraphicsEnvironment.java	1.5 06/10/10
 * 
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

package java.awt;

import java.util.Locale;
import java.awt.image.BufferedImage;
import java.security.*;


/**
* This is an implementation of a GraphicsEnvironment object Qtfor the
 * default local GraphicsEnvironment used by the JavaSoft JDK in Qt 
 * environments.
 *
 * @see GraphicsDevice
 * @see GraphicsConfiguration
 * @version 10 Feb 1997
 */

class QtGraphicsEnvironment extends GraphicsEnvironment {
	
    QtGraphicsDevice defaultScreenDevice;
	
    private static native void init();
    private static native int initfb();
    private native void destroy();

    private static Thread eventThread;	

    static native void QTprocessEvents();

    private native void QTexec();
    private native void QTshutdown();
    private boolean shutdown = false; // is qt shutdown ?

    QtGraphicsEnvironment() {
		
		java.security.AccessController.doPrivileged(
             new sun.security.action.LoadLibraryAction("qtawt"));
		
		Runtime.getRuntime().addShutdownHook(createShutdownHook());

		init();

		eventThread = createEventThread();
		eventThread.setPriority(Thread.NORM_PRIORITY+1);
		eventThread.start();

        // initfb() creates the QApplication and the QWidget that backs
        // the screen and must be done within the Qt event thread. so 
        // we wait till the graphics device is created (See eventThread.run())

        synchronized (this) {
            while ( this.defaultScreenDevice == null ) {
                try { this.wait() ; }
                catch ( Exception ex ) {}
            }
        }
	}
	
    public synchronized GraphicsDevice[] getScreenDevices() {
        return new GraphicsDevice[] {defaultScreenDevice};
    }
	
    /**
		* Returns the default screen graphics device.
     */
	
    public GraphicsDevice getDefaultScreenDevice() {
        return defaultScreenDevice;
    }
	
    public String[] getAvailableFontFamilyNames() {
        return QtFontMetrics.getFontList();
    }
	
    public String[] getAvailableFontFamilyNames(Locale l) {
        return QtFontMetrics.getFontList();
    }
	
    void setWindow(QtGraphicsDevice device, Window window) {
		
        pSetWindow(device.psd, window);
    }
	
    private native void pSetWindow(int psd, Window window);
	
    /**
		* Returns a <code>Graphics2D</code> object for rendering into the
     * specified {@link BufferedImage}.
     * @param img the specified <code>BufferedImage</code>
     * @return a <code>Graphics2D</code> to be used for rendering into
     * the specified <code>BufferedImage</code>.
     */
    public Graphics2D createGraphics(BufferedImage img){
        return img.createGraphics();
    }

    Thread createEventThread() {
		Thread thread = 
        new Thread("AWT-Qt-EventThread") {
            public void run() {
                // create the graphics device. initfb() creates the 
                // QApplication
		        QtGraphicsEnvironment.this.defaultScreenDevice = 
                new QtGraphicsDevice(QtGraphicsEnvironment.this, initfb());

                // notify that the graphics device has been created.
                synchronized (QtGraphicsEnvironment.this) {
                    QtGraphicsEnvironment.this.notifyAll();
                }

                // start the event loop
				while(true) {
                    QtGraphicsEnvironment.this.QTexec();
                    if ( QtGraphicsEnvironment.this.shutdown ) {
                        break;
                    }
				}
			}
		};

        return thread;
    }

    Thread createShutdownHook() {
        Thread thread = 
        new Thread("AWT-Qt-ShutdownThread") {
            public void run() {
                QtGraphicsEnvironment.this.shutdown = true;
                QtGraphicsEnvironment.this.QTshutdown();
                QtGraphicsEnvironment.this.destroy();
            }
        };
        return thread;
    }

// The following two methods support the old qtpoll threads
//
//     Thread createEventThread() {
// 		return new Thread("AWT-Qt-EventThread") {
//             public void run() {
// 				while(!isInterrupted()) {
// 					try { Thread.sleep(60); } 
//                     catch (InterruptedException e) { }
// 					synchronized(QtGraphics.MONSTERLOCK) {
// 						QTprocessEvents();
// 					}
// 				}
// 			}
// 		};
//     }

//     Thread createShutdownHook() {
//         return new Thread("AWT-Qt-ShutdownThread") {
//             public void run() {
//                 eventThread.interrupt();
//                 QtGraphicsEnvironment.this.destroy();
//             }
//        };
//     }

	/*
	 protected void finalize() throws Throwable
	 {
		 destroy();
		 super.finalize();
	 }
	 */
}
