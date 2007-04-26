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

import com.sun.jump.common.JUMPContent;
import com.sun.jump.common.JUMPAppModel;
import com.sun.jump.executive.JUMPExecutive;
import com.sun.jump.module.installer.*;
import com.sun.midp.jump.MIDletApplication;

import java.util.Enumeration;

/*
 * A simple test to retrieve the installed midlet information
 * using the JUMPInstallerModule.
 */
public class AppIDTest {
    public static void main(String[] args) {
        new AppIDTest().doList();
    }

    private void doList() {
       if (JUMPExecutive.getInstance() == null) {
           new com.sun.jumpimpl.module.installer.InstallerFactoryImpl();
       }

       JUMPInstallerModuleFactory factory = JUMPInstallerModuleFactory.getInstance();
       JUMPInstallerModule installer = factory.getModule(JUMPAppModel.MIDLET);

       JUMPContent[] content = installer.getInstalled();
       if (content != null) {
           for(int j = 0; j < content.length; j++) {  
              System.out.println("=======");
              MIDletApplication midlet = (MIDletApplication)content[j];
              Enumeration _enum	= midlet.getPropertyNames();       
	      while (_enum.hasMoreElements()) {
                 String key = (String)_enum.nextElement();
		 System.out.println(key + ":" + midlet.getProperty(key));
              }
	      int id = midlet.getId();
              System.out.println("  SuiteID calculation: " + 
		    MIDletApplication.convertToSuiteID(id));
              //System.out.println("  MIDlet number calculation: " + 
              //      (id & 0x000000ff)); 
           }
       }
    }
}
