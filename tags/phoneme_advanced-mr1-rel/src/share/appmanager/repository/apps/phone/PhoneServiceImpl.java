/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

import com.sun.appmanager.impl.client.CDCAmsClient;

import com.sun.appmanager.client.Client;

import java.rmi.RemoteException;
import java.rmi.AlreadyBoundException;
import java.rmi.registry.Registry;
import javax.microedition.xlet.XletContext;
import javax.microedition.xlet.ixc.IxcRegistry;

public class PhoneServiceImpl implements PhoneService
{
    public PhoneServiceImpl() throws RemoteException
    {
	super();
    }
    
    public boolean dial(String number) 
    {
	System.err.println("..... DIALING " + number);
	return true;
    }

    public static void startService(XletContext context) 
    {
	Registry manager = IxcRegistry.getRegistry(context);
	try {
	    PhoneServiceImpl psi = new PhoneServiceImpl();
	    manager.bind("phone/dial", psi);
	    // Done
	    System.err.println("-- PhoneServiceImpl bound in registry: "+psi);
	} catch (AlreadyBoundException aee) {
            // Ignore - PhoneService already being bound. 
        } catch (SecurityException se) {
            System.err.println("-- Unexpected SecurityException while binding \"phone/dial\" name");
	    se.printStackTrace();
	} catch (Exception e) {
	    e.printStackTrace();
        }
    }
}
