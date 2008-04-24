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
 *
 */

package com.sun.jumpimpl.ixc;

import java.awt.Container;
import java.util.HashMap;
import javax.microedition.xlet.*;

/*
 * An 'fake' XletContext impl for JMP system to utilize.
 * getClassLoader() method returns the ClassLoader passed in 
 * at the constructor time.
 */

public class XletContextFactory { 

   /**
    * Returns an 'fake' XletContext to be used for 
    * IxcRegistry.getRegistry(XletContext).
    *
    * It is important for this method to convert the parameter classloader
    * to the right instance before creating a DummyXletContext with it, 
    * because the ClassLoader in the XletContext is later used as a defining 
    * ClassLoader for the generated stubs.
    *
    * We want to make sure that the system driven IXC calls result in 
    * the stubs loaded by the system ClassLoader, and application
    * driven IXC calls to result i the stubs loaded by the ClassLoader
    * associated with the calling application.
    *
    * Invoke this method with the calling Class's ClassLoader as a param,
    * such as getClassLoader(this.getClass().getClassLoader());
   **/ 

   public static XletContext getXletContext(ClassLoader loader) {
        return createXletContext(loader);
   }

   private static HashMap contextHash = new HashMap(); // CL, DummyXletContext

   private static XletContext createXletContext(ClassLoader loader) { 
      /*
       * If loader is null, use system class loader which must be
       * not null
       */
      if (loader == null) {
          loader = ClassLoader.getSystemClassLoader();
          if (loader == null) {
              throw new RuntimeException("Failed to obtain " +
                      "valid class loader for fake xlet context");
          }
      }

      synchronized(contextHash) {
          XletContext context = (XletContext)contextHash.get(loader);
          if (context == null) {
              context = new DummyXletContext(loader);
              contextHash.put(loader, context);
          }
          return context;
      }
   }
   
   static class DummyXletContext 
       implements javax.microedition.xlet.XletContext{
   
       ClassLoader loader;
   
       public DummyXletContext(ClassLoader loader) {
          //if (loader == null)
          //   throw new IllegalArgumentException("ClassLoader is null");
          this.loader = loader;
       }
    
       public void notifyDestroyed() {}
       public void notifyPaused() {}
       public Object getXletProperty(String key) { return null; }
       public void resumeRequest() {}
       public Container getContainer() throws UnavailableContainerException {
          return null;
       }
       public java.lang.ClassLoader getClassLoader() {
          return loader;
       }
   }

}
