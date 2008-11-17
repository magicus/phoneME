/*
 * @(#)WrappedRemote.java	1.14 05/03/12
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
 *
 */

package com.sun.xlet.ixc;

import java.rmi.Remote;
import java.rmi.RemoteException;
import java.rmi.UnexpectedException;
import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import javax.microedition.xlet.XletContext;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.PrivilegedActionException;
import java.security.PrivilegedExceptionAction;


/**
 * A remote object that has been wrapped to be safely accessible from
 * a client.  In other words, an instance of WrappedRemote is a stub
 * object that delegates its calls to the true remote object.  The remote
 * calls are executed in a thread that is in the remote ResourceDomain.
 * This can be done by transferring the call to a worker thread on the
 * remote side, or by changing the owner attribute of the thread.
 */


public abstract class WrappedRemote implements Remote {
    private Remote target;
    private ImportRegistry registry;
    private RegistryKey key;
    private static Method hashCodeMethod;
    private static Method equalsMethod;
    private static Method toStringMethod;
    static {
        try {
            Class obj = Object.class;
            hashCodeMethod = obj.getMethod("hashCode", new Class[0]);
            equalsMethod = obj.getMethod("equals", new Class[] { obj }
                    );
            toStringMethod = obj.getMethod("toString", new Class[0]);
        } catch (Exception ex) {
            // assert(false);
            ex.printStackTrace();
        }
    }
    protected WrappedRemote(Remote target, ImportRegistry registry,
        RegistryKey key) {
        this.target = target;
        this.registry = registry;
        this.key = key;
    }

    protected void finalize() {
        registry.unregisterStub(key);
    }

    //
    // Execute a remote method.  Note that this Method object must refer
    // to the method loaded by the target classloader (and not the
    // classloader of the stub class).
    //
    protected final Object 
    com_sun_xlet_execute(final Method remoteMethod, final Object[] args) 
        throws java.lang.Exception {
        //XletContextImpl localXlet = registry.importer;
        //XletContextImpl remoteXlet = registry.target;
        XletContext localXlet = registry.importer;
        XletContext remoteXlet = registry.target;
        if (remoteXlet == null) {
            throw new RemoteException("Remote target killed");
        }
        IxcRegistryImpl ixcRegis = 
            IxcRegistryImpl.getIxcRegistryImpl(remoteXlet);
        ImportRegistry remoteIR = (ImportRegistry)
            ixcRegis.getImportRegistry(localXlet);
        final AccessControlContext context = ixcRegis.acc;

        // Now wrap or copy arguments...
        for (int i = 0; i < args.length; i++) {
            args[i] = remoteIR.wrapOrCopy(args[i]);
        }
        final Object[] result = new Object[2];
        final Remote targetNow = target;
        if (targetNow == null) {
            // This should never happen, but be conservative
            throw new RemoteException("Remote target killed");
        }
        if (remoteMethod == null) {
            // Stub has been destroyed in this case, must be reexported object 
            throw new RemoteException("Remote target killed");
        }
        ixcRegis.executeForClient(new Runnable() {
                public void run() {
		    try {
		       AccessController.doPrivileged(
			  new PrivilegedExceptionAction() {
			     public Object run() throws RemoteException {
				Throwable err = null;
				try {
				    result[0] = remoteMethod.invoke(targetNow, args);
				} catch (InvocationTargetException ite) {
				    err = ite.getTargetException();
				} catch (Throwable t) {
				    err = t;
				}
				if (err == null) {
				    try {
					result[0] = registry.wrapOrCopy(result[0]);
				    } catch (RemoteException ex) {
					result[1] = ex;
				    }
				} else {
				    try {
					result[1] = registry.wrapOrCopy(err);
				    } catch (RemoteException ex) {
					result[1] = ex;
				    }
				}
				return null;
			    }
			}
		       , context);
		    } catch (PrivilegedActionException pae) {
			// assert(false)
			pae.getCause().printStackTrace();
		    }
                }
            }
        );
        if (result[1] != null) {
            // fix for 4653779, if the exception is checked, throw it
            // directly, else wrap it in UnsupportedException
            Class[] exceptions = remoteMethod.getExceptionTypes();
            for (int i = 0; i < exceptions.length; i++) {
                if (exceptions[i].isInstance(result[1])) {
                    throw (Exception) result[1];
                }
            }
            // fix for 4641529, just throw if it's a RuntimeException,
            // else wrap it in RemoteException

            if (result[1] instanceof RuntimeException) {
                throw (RuntimeException) result[1];
            } else if (result[1] instanceof RemoteException) {
                throw (RemoteException) result[1];
            } else {
                throw new UnexpectedException("Non-declared exception", (Exception) result[1]);
            }
        }
        return registry.wrapOrCopy(result[0]);
    }

    void destroy() {
        target = null;
    }

    Remote getTarget() {
        return target;
    }

    public int hashCode() {
        try {
            Object result 
                = com_sun_xlet_execute(hashCodeMethod, new Object[0]);
            return ((Integer) result).intValue();
            //} catch (RemoteException ex) {
        } catch (Exception ex) {
            // Client has died.  This is the same behavior that
            // java.rmi.server.RemoteObject gives.
            return super.hashCode();
        }
    }

    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        } else if (!(obj instanceof WrappedRemote)) {
            if (obj == null) {
                return false;
            } else {
                return obj.equals(this);
                // cf. bug 4099660, and java.rmi.server.RemoteObject.equals().
            }
        }
        try {
            Object result 
                //= com_sun_tvimpl_execute(equalsMethod, new Object[] { obj });
                = com_sun_xlet_execute(equalsMethod, new Object[] { obj }
                );
            return ((Boolean) result).booleanValue();
        } catch (Exception ex) {
            // Client has died, or o is an incompatible type.
            return false;
        }
    }

    public String toString() {
        try {
            String classname = this.getClass().getName();
            Object result 
                //= com_sun_tvimpl_execute(toStringMethod, new Object[0]);
                = com_sun_xlet_execute(toStringMethod, new Object[0]);
            return classname + "[" + result + "]";
            //} catch (RemoteException ex) {
        } catch (Exception ex) {
            return super.toString();
        }
    }
}
