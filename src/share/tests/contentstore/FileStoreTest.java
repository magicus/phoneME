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
import java.util.HashMap;
import java.util.Iterator;
import java.util.Properties;
 
import com.sun.jump.module.contentstore.*;

public class FileStoreTest {
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

    static String[] sampleListUris = {
       "./Apps/Amark",
       "./Apps/Amark/data"
    };

    public static void main(String[] args) {
       new FileStoreTest();
    }

    public FileStoreTest() {

       // This one line should be called by the executive, but doing it here for time being.
       new com.sun.jumpimpl.module.contentstore.StoreFactoryImpl();

       JUMPStore store = JUMPStoreFactory.getInstance().getModule(JUMPStoreFactory.TYPE_FILE);
       String repositoryRoot = "repository";
       // test setup, make a repository root 
       File file = new File(repositoryRoot);
       if (!file.exists()) { 
          System.out.println(repositoryRoot + " directory not found");  
          System.out.println("Creating " + repositoryRoot + " dir to be used for the store's root"); 
          file.mkdirs();
       }

       // These three lines below should have happened in the executive setup,
       // but for the testing purpose, emulating load() call here.
       HashMap map = new HashMap();
       map.put("cdcams.repository", repositoryRoot);
       store.load(map);
       // end of store setup.
   
       System.out.println("Got a type of JUMPStore: " + store);

       // first, try to create list nodes.
       for (int i = 0; i < sampleListUris.length; i++) {
          store.createNode(sampleListUris[i]);
       }

       // then, create all data nodes.
       for (int i = 0; i < sampleDataUris.length; i++) {
          store.createDataNode(sampleDataUris[i], datas[i]);
       }

       // get back all the data nodes we just created and compare with original nodes.
       System.out.println("All data created, testing equality");
       for (int i = 0; i < sampleDataUris.length; i++) {
          JUMPData data = ((JUMPNode.Data)store.getNode(sampleDataUris[i])).getData();
          if (!data.equals(datas[i])) 
             System.out.println("node mismatch: " + data + "," + datas[i]);
       }

       // try get the listing of the store.
       JUMPNode.List dirnode = (JUMPNode.List) store.getNode(".");

       System.out.println("List of store content");
       printChildren(dirnode, "   ");

       // now, delete everything.
       System.out.println("Delete all");
       for (int i = 0; i < sampleListUris.length; i++) {
          System.out.println("Deletion successful? " + store.deleteNode(sampleListUris[i]));
       }

       System.out.println("List of store content");
       printChildren(dirnode, "   ");

       System.out.println("Test done, exiting");
    }

    void printChildren(JUMPNode.List list, String tab) {
       for (Iterator itn = list.getChildren(); itn.hasNext(); ) {
           JUMPNode node = (JUMPNode) itn.next();
           System.out.println(tab + node);
           if (node != null && !node.containsData()) 
              printChildren((JUMPNode.List)node, tab + "   ");
       }
      
    }
}
