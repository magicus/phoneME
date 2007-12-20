/*
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

package com.sun.jumpimpl.client.module.serviceregistry;

import java.rmi.Remote;
import java.rmi.RemoteException;
import java.rmi.NotBoundException;
import javax.microedition.xlet.Xlet;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;
import javax.microedition.xlet.ixc.StubException;

import com.sun.jumpimpl.ixc.XletContextFactory;
import com.sun.jumpimpl.ixc.ConnectionReceiver;

import com.sun.jumpimpl.process.JUMPModulesConfig;
import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;

public class ServiceRegistryClient {

    IxcRegistry registry;

    /**
     *
     * @param classloader class loader to use
     */
    public ServiceRegistryClient(
            ClassLoader classloader, int portNumber) {

       /*
	* First set the port number that the executive VM's registry is 
	* accepting connection at. 
	* 
	* FIXME: this port number setup should go away once IXC is on messaging.
	*/

       ConnectionReceiver.setExecVMServicePort(portNumber);

       /*
	* Grab the IxcRegistry instance for the client VM.
	* Use the system classloader to get the registry.  This classloader
	* would later be used for loading the stub classes for the remote objects.
	*/
       XletContext context = XletContextFactory.getXletContext(classloader);

       registry = IxcRegistry.getRegistry(context);
    }

    public Remote getRemoteService(String name) {
       try {
          return registry.lookup(name);
       } catch (StubException e) {
          e.printStackTrace();
       } catch (NotBoundException e) {
          System.err.println(name + " not found in the registry");
       } catch (SecurityException e) { // shouldn't happen
          e.printStackTrace();
       }

       return null;
    }
}
