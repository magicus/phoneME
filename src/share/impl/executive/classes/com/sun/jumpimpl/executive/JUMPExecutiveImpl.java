/*
 * %W% %E%
 *
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

package com.sun.jumpimpl.executive;

import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.executive.JUMPUserInputManager;
import com.sun.jump.executive.JUMPIsolateProxy;
import com.sun.jump.executive.JUMPApplicationProxy;
import com.sun.jump.message.JUMPMessagingService;
import com.sun.jump.message.JUMPMessageDispatcher;
import com.sun.jump.message.JUMPOutgoingMessage;
import com.sun.jump.message.JUMPMessage;
import com.sun.jump.common.JUMPAppModel;

import com.sun.jump.module.lifecycle.JUMPLifeCycleModuleFactory;
import com.sun.jump.module.lifecycle.JUMPLifeCycleModule;

import com.sun.jumpimpl.process.JUMPProcessProxyImpl;

import com.sun.jump.os.JUMPOSInterface;

import com.sun.jump.common.JUMPContent;
import com.sun.jump.common.JUMPApplication;
import com.sun.jump.module.installer.JUMPInstallerModuleFactory;
import com.sun.jump.module.installer.JUMPInstallerModule;

import java.util.Properties;
import java.io.IOException;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.BufferedInputStream;

public class JUMPExecutiveImpl extends JUMPExecutive {
    private JUMPProcessProxyImpl pp;
    private JUMPOSInterface os;

    private static void
    overrideDefaultConfig(String fname) {
        Properties props = new Properties();

        InputStream in = null;
        try {
            in = new BufferedInputStream(new FileInputStream(fname));
            props.load(in);
        } catch(IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if(in != null) {
                    in.close();
                }
            } catch(IOException e) {
                e.printStackTrace();
            }
        }

        JUMPFactories.getDefaultConfig().putAll(props);
    }

    private void
    handleCommandLine(String[] args) {
        for(int i = 0; i < args.length; ++i) {
            if("--config-file".equals(args[i])) {
                if(!(++i < args.length)) {
                    throw new IllegalArgumentException(
                        "configuration file not specified");
                }
                overrideDefaultConfig(args[i]);
            }
        }
    }

    public JUMPExecutiveImpl() {
	super();
    }

    /*
     * Main entry point to the executive
     */
    public static void main(String[] args) {
	JUMPExecutiveImpl jei = new JUMPExecutiveImpl();

        System.loadLibrary("jumpmesg");

        jei.handleCommandLine(args);

	// Initialize os interface
	new com.sun.jumpimpl.os.JUMPOSInterfaceImpl();

	// Get critical objects
	jei.os = JUMPOSInterface.getInstance();
	jei.pp = JUMPProcessProxyImpl.createProcessProxyImpl(jei.os.getProcessID());

        JUMPFactories.init();

	if (true) {
	    // Sample code to create blank isolate upon startup
	    JUMPLifeCycleModuleFactory lcmf =
		JUMPLifeCycleModuleFactory.getInstance();
	    JUMPLifeCycleModule lcm = lcmf.getModule();
	    JUMPIsolateProxy ip = lcm.newIsolate(JUMPAppModel.MAIN);
	    System.err.println("New isolate created="+ip);

	    JUMPInstallerModuleFactory imf = 
		JUMPInstallerModuleFactory.getInstance();
	    JUMPInstallerModule xletInstaller = 
		imf.getModule(JUMPAppModel.MAIN);
	    JUMPContent[] content = xletInstaller.getInstalled();
	    if (content != null) {
		for (int i = 0; i < content.length; i++) {
		    JUMPApplication app = (JUMPApplication)content[i];
		    System.err.println("App["+i+"] = "+app);
		}
		JUMPApplicationProxy appProxy = ip.startApp((JUMPApplication)content[0], null);
		System.err.println("Executive started app="+appProxy);
	    } else {
		System.err.println("No content available");
		System.exit(1);
	    }
	}

	// Take it away, someone -- presentation mode?
	try {
	    Thread.sleep(0L);
	} catch(Throwable e) {
	}
    }

    public int
    getProcessId() {
        return os.getProcessID();
    }

    public JUMPMessageDispatcher
    getMessageDispatcher() {
        return pp.getMessageDispatcher();
    }

    public JUMPOutgoingMessage
    newOutgoingMessage(String mesgType) {
        return pp.newOutgoingMessage(mesgType);
    }

    public JUMPOutgoingMessage
    newOutgoingMessage(JUMPMessage requestMessage) {
        return pp.newOutgoingMessage(requestMessage);
    }

    public JUMPMessage
    newMessage(byte[] rawData) {
        return pp.newMessage(rawData);
    }

    public JUMPUserInputManager getUserInputManager() {
        return null;
    }
}
