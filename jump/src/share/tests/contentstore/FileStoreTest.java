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

import java.io.*;
import java.io.IOException;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;
import java.util.Properties;
 
import com.sun.jump.module.contentstore.*;
import com.sun.jump.executive.JUMPExecutive;

public class FileStoreTest extends JUMPContentStoreSubClass {

    // data URIs for tests to use.
    static String[] sampleDataUris = {
       "./Apps/Amark/title",
       "./Apps/Amark/size",
       "./Apps/Amark/icon",
       "./Apps/Amark/descriptor",
       "./Apps/Amark/data/Subdata.properties",
       "./Apps/Amark/Amark.properties"
    };

    // Data files corresponding to sampleDataUris above.
    static JUMPData[] datas = {
       new JUMPData(true),
       new JUMPData(1.0f),
       new JUMPData(12345),
       new JUMPData("String"),
       new JUMPData(new Properties()),
       new JUMPData(new Properties())
       //new JUMPData(System.getProperties())
    };

    // list URIs for tests to use.
    static String[] sampleListUris = {
       "./Apps/Amark",
       "./Apps/Amark/data"
    };

    // The directory name which "." of this content store will be using.
    String repositoryRoot = "repository";

    public static void main(String[] args) {
       new FileStoreTest();
    }

    public FileStoreTest() {
       setupTest();

       new Thread() { 
          public void run() { runTest(); }
       }.start();
    }

    void setupTest() { 

       // For the standalone test run, emulate the executive setup
       if (JUMPExecutive.getInstance() == null) {
          JUMPStoreFactory factory = new com.sun.jumpimpl.module.contentstore.StoreFactoryImpl();
	  factory.load(com.sun.jumpimpl.process.JUMPModulesConfig.getProperties());
       }

       // test setup, make a repository root if it doesn't exist
       File file = new File(repositoryRoot);
       if (!file.exists()) { 
          System.out.println(repositoryRoot + " directory not found");  
          System.out.println("Creating " + file.getAbsolutePath() + " directory to be used for the storeHandle's root"); 
          System.out.println("The directory will be removed at VM exit");
          file.mkdirs();
          file.deleteOnExit();
       }

       // Done with the setup.
    }


    void runTest() { 

       System.out.println("Starting the test run");

       // Get the store handle from the JUMPContentStoreSubClass. 
       JUMPStoreHandle storeHandle = openStore(true);

       System.out.println("Got a type of JUMPStore: " + storeHandle);

       try {
           // first, try to create list nodes.
           for (int i = 0; i < sampleListUris.length; i++) {
              storeHandle.createNode(sampleListUris[i]);
           }

           // then, create all data nodes.
           for (int i = 0; i < sampleDataUris.length; i++) {
              storeHandle.createDataNode(sampleDataUris[i], datas[i]);
           }

       // get back all the data nodes we just created and check the data content.
       System.out.println("All data created, testing equality");
       for (int i = 0; i < sampleDataUris.length; i++) {
          JUMPData data = ((JUMPNode.Data)storeHandle.getNode(sampleDataUris[i])).getData();
          if (!data.equals(datas[i])) 
             System.out.println("node mismatch: " + data + "," + datas[i]);
       }

           // get the listing of all nodes starting at the root.
           JUMPNode.List dirnode = (JUMPNode.List) storeHandle.getNode(".");

           System.out.println("List of storeHandle content");
           printChildren(dirnode, "   ");

           // try updating a node content.
           System.out.println("Testing update of storeHandle content");
           storeHandle.updateDataNode(sampleDataUris[0], datas[1]);
           JUMPData data = ((JUMPNode.Data)storeHandle.getNode(sampleDataUris[0])).getData();
           if (!data.equals(datas[1]))
             System.out.println("update failed : " + data + "," + datas[1]);

           // now, delete all list nodes.
           System.out.println("Delete all");
           for (int i = 0; i < sampleListUris.length; i++) {
               storeHandle.deleteNode(sampleListUris[i]);
           }

           System.out.println("List of storeHandle content");
           printChildren(dirnode, "   ");
       }
       catch (IOException ioe) {
           System.out.println("IOException was thrown!");
           ioe.printStackTrace(System.out);
       }

       System.out.println("Closing the store");
       closeStore(storeHandle);

       System.out.println("Done.");
    }


    // Recursively print all the nodes in the tree.
    void printChildren(JUMPNode jumpNode, String tab) {
       if (jumpNode != null) {
          System.out.println(tab + jumpNode);
          if (jumpNode instanceof JUMPNode.List) {
             JUMPNode.List list = (JUMPNode.List) jumpNode;
             for (Iterator itn = list.getChildren(); itn.hasNext(); ) {
                JUMPNode node = (JUMPNode) itn.next();
                printChildren(node, tab + "   ");
             }
          }
       }
    }
}

class JUMPContentStoreSubClass extends JUMPContentStore {
                                                                                  
   protected JUMPStore getStore() {
      JUMPStore store = JUMPStoreFactory.getInstance().getModule(
                                 JUMPStoreFactory.TYPE_FILE);
                                                                                  
      return store;
   }
                                                                                  
   public void load(Map map) {}
   public void unload() {}
}
