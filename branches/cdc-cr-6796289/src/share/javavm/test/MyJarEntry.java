/*
 * @(#)MyJarEntry.java	1.6 06/10/10
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
import java.io.*;
import java.util.jar.*;

public class MyJarEntry {

  public static void main(String argv[]) {
      File file = new File("/net/novo55/export/home/kvm", "test.jar");
      JarFile jarFile = null;
      String fileName = file.getPath();
      JarEntry jarEntry = null;
      java.io.InputStream is = null;
      byte[] buff = new byte[1000];
      int ret = 0;
      try {
           jarFile = new JarFile(file, true);
           jarEntry = jarFile.getJarEntry("entry1.dat");
           System.out.println("jarEntry: "+jarEntry.toString());
           is = jarFile.getInputStream(jarEntry);
           ret = 0;
           while (ret != -1) {
                ret = is.read(buff, 0, 1000);
                System.out.println("ret="+ret);
           }
           if (jarEntry.getCertificates() == null) {
               jarFile.close();
               System.out.println("FAILD: JarEntry.getCertificates() == null ");
           } else {
                jarFile.close();
                System.out.println("OKAY");
           }
      } catch (Exception e) {
            System.out.println("FAILED: Unexpected IOException "+e);
      }
  }
}

