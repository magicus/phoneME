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

package com.sun.jumpimpl.isolate.jvmprocess;

import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.isolate.jvmprocess.JUMPIsolateProcess;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainer;
import com.sun.jump.isolate.jvmprocess.JUMPAppContainerContext;

import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.Map;

public class AppContainerFactoryImpl {

   private Map config = JUMPIsolateProcess.getInstance().getConfig();

   private static String PREFIX = "com.sun.jumpimpl.isolate.jvmprocess";
   private static String CONTAINER_CLASSNAME   = "AppContainerImpl";
   private static String FACTORY_METHODNAME  = "getInstance";

   public JUMPAppContainer getAppContainer(JUMPAppModel model,
                                           JUMPAppContainerContext context) {
      
       String defaultPackageName = PREFIX + "." + model.toString();

       String packageName = null;

       if (config != null) {
           packageName = (String) config.get("appcontainerfactory.packagename." + model.toString());
       }
       
       if (packageName == null) {
           packageName = defaultPackageName;
       }	        
       
       String fullClassName = packageName + "." + CONTAINER_CLASSNAME;


       try { 
          Class clazz = Class.forName(fullClassName);

          Method m = clazz.getMethod(FACTORY_METHODNAME,
                        new Class[] {JUMPAppContainerContext.class});

          return (JUMPAppContainer) m.invoke(null, new Object[] {context});

       } catch (ClassNotFoundException e) {
          System.out.println(fullClassName + " not found");
          e.printStackTrace();
       } catch (NoSuchMethodException e) {
          System.out.println(FACTORY_METHODNAME + "() not found in " + fullClassName);
          e.printStackTrace();
       } catch (IllegalAccessException e) {
          e.printStackTrace();
       } catch (InvocationTargetException e) {
          e.printStackTrace();
       } catch (IllegalArgumentException e) {
          e.printStackTrace();
       }
       return null;
   }

}
