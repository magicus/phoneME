/*
 * @(#)ImportRegistry.java	1.20 06/10/10
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

import java.util.Hashtable;
import java.util.Enumeration;
import java.lang.ref.WeakReference;
import java.lang.reflect.Constructor;
import java.lang.reflect.Method;
import java.io.Serializable;
import java.io.ByteArrayOutputStream;
import java.io.ObjectOutputStream;
import javax.microedition.xlet.XletContext;
import java.rmi.Remote;
import java.rmi.RemoteException;
import java.rmi.NotBoundException;
import java.rmi.AlreadyBoundException;
import javax.microedition.xlet.ixc.StubException;

/**
 * Registry for imported objects.  This keeps track of objects that have
 * been imported into our XletContext, called importer, from some other
 * XletContext, called target.
 */

public class ImportRegistry {
    XletContext importer;
    XletContext target;	
    // Xlet we import from.  This gets set null when that Xlet is
    // killed.
    private Hashtable importedObjects = new Hashtable(11);
    // Hashtable<RegistryKey, WrappedRemote>

    ImportRegistry(XletContext importer, XletContext target) {
        this.importer = importer;
        this.target = target;
    }

    // Returns the Remote object which might be wrapped as a Stub.
    // 
    public Remote lookup(Remote rawResult) 
        throws InterruptedException, NotBoundException, RemoteException {
        XletContext t = target;
        if (t == null) {
            throw new RemoteException("Target Xlet has been destroyed");
        }
        if (rawResult == null) {
            throw new NotBoundException();
        }
        if (rawResult instanceof WrappedRemote) { // re-exporting!
            rawResult = ((WrappedRemote) rawResult).getTarget(); 
        }
        return wrapIfImported(rawResult);
    }

    private Remote wrapImportedObject(Remote raw) throws RemoteException {
        Remote result = null;
        synchronized (importedObjects) {
            WeakReference ref = (WeakReference) importedObjects.get(raw);
            if (ref != null) {
                result = (Remote) ref.get();
                if (result == null) {
                    importedObjects.remove(raw);
                    // This makes sure that the old RegistryKey doesn't
                    // get retained in the Hashtable.  If it did, it would
                    // be == to the RegistryKey that pretty soon is
                    // going to be removed, when the stub's finalizer
                    // executes.
                }
            }
            if (result == null) {
                RegistryKey key = new RegistryKey(raw);
                result = doWrap(raw, key);
                importedObjects.put(key, new WeakReference(result));
            }
        }
        return result;
    }

    private Remote wrapIfImported(Remote raw) throws RemoteException {
        if (raw == null) {
            throw new RemoteException("Target Xlet has been destroyed");
        }
        if (isParentLoader(raw.getClass().getClassLoader(),
                           IxcRegistryImpl.getIxcClassLoader(importer))) {
            return raw;		// No need to wrap if it's from us
        } else {
            return wrapImportedObject(raw);
        }
    }

    //
    // Wraps a remote object that we're sure has never been wrapped
    // before.
    //
    private Remote doWrap(Remote raw, RegistryKey key) throws RemoteException {
        Remote r;
        try {
            // First get the target xlet's IxcClassLoader, to wrap.
            IxcClassLoader importerLoader 
                = IxcRegistryImpl.getIxcClassLoader(importer);

            // Then get the stb object wrapped with the importer's CL.
            // The returned wrappedClass is loaded with importerLoader's 
            // parent classloader, which is xlet's ClassLoader.
            Class wrappingClass 
                = importerLoader.getStubClass(
                  IxcRegistryImpl.getIxcClassLoader(target), raw.getClass());

            // Then instanciate the Remote object and return it.
            Class[] types = { Remote.class, 
                              ImportRegistry.class,
                              RegistryKey.class };
            Constructor m = wrappingClass.getConstructor(types);
            Object[] args = { raw, this, key };

            r = (Remote) m.newInstance(args);

        } catch (RemoteException ex) {
            throw ex;
        } catch (Exception ex) {
            //throw new RemoteException("cannot wrap Remote", ex);
            throw new StubException("cannot wrap Remote", ex);
        }

        return r;
    }

    /**
     * Creates a deep copy of a serializable object.
     *
     * @param obj	an object which will be copied
     *
     * @return		a deep copy of its argument
     *
     * @see		#java.io.Serializable
     *			file does not exist or cannt be downloaded.
     */
    private Serializable copyObject(Serializable o) throws RemoteException {
        try {
            ByteArrayOutputStream bos = new ByteArrayOutputStream();
            ObjectOutputStream oos = new ObjectOutputStream(bos);
            oos.writeObject(o);
            oos.flush();
            oos.close();
            byte[] ba = bos.toByteArray();
            // Now, we have to de-serialize the object in a method loaded
            // by the importer's classloader.
            //Method m = importer.loader.getDeserializeMethod();
            Method m = IxcRegistryImpl.getIxcClassLoader(importer).getDeserializeMethod();
            return (Serializable) m.invoke(null, new Object[] { ba }
                );
        } catch (RemoteException ex) {
            throw ex;
        } catch (Exception ex) {
            throw new RemoteException("Cannot copy object", ex);
        }
    }

    Object wrapOrCopy(Object o) throws RemoteException {
        if (o == null) {
            return null;
        } else if (o instanceof WrappedRemote) {
            return wrapIfImported(((WrappedRemote) o).getTarget());
        } else if (o instanceof Remote) {
            return wrapImportedObject((Remote) o);
        } else if (o instanceof Serializable) {
            return copyObject((Serializable) o);
        } else {
            throw new RemoteException("Object is neither Serializable nor Remote");
        }
    }

    void unregisterStub(RegistryKey key) {
        importedObjects.remove(key);
    }

    boolean isParentLoader(ClassLoader parent, ClassLoader child) {
        do {
            if (child == parent) {
                return true;
            }
            child = child.getParent();
        }
        while (child != null);
        return false;
    }

    //
    // Called to notify that our target Xlet has been destroyed.  This means
    // that we shouidl destroy all the imported objects we manage.
    //
    void targetDestroyed() {
        try {
            IxcClassLoader importingLoader = 
                IxcRegistryImpl.getIxcClassLoader(importer);
            IxcClassLoader targetLoader = 
                IxcRegistryImpl.getIxcClassLoader(target);
            importingLoader.forgetStubsFor(targetLoader);
        } catch (Exception e) {
            e.printStackTrace();
        }
        target = null;
        synchronized (importedObjects) {
            for (Enumeration e = importedObjects.elements(); 
                e.hasMoreElements();) {
                WeakReference value = (WeakReference) e.nextElement();
                WrappedRemote r = (WrappedRemote) value.get();
                if (r != null) {
                    r.destroy();
                }
            }
        }
    }
}
