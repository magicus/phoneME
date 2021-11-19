/*
 * %W% %E%
 *
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

package com.sun.jumpimpl.module.contentstore;

import java.io.File;
import java.io.IOException;
import java.io.ObjectOutputStream;
import java.io.ObjectInputStream;
import java.io.FileOutputStream;
import java.io.FileInputStream;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import com.sun.jump.module.contentstore.*;

public class FileStoreImpl extends JUMPStore {

   //HashMap jumpNodeLists = new HashMap(); // uri, JUMPNode.List

   boolean verbose = false;

   public void load(Map map) {

       Object basedir;
       if ((basedir = System.getProperty("contentstore.root")) != null) { 
          setStoreRoot((String)basedir);
       } else if (map != null && (basedir = map.get("contentstore.root")) != null) {
          setStoreRoot((String)basedir);
       } else { 
          setStoreRoot(".");
       }
   }

   public void unload() {
   }

   public void createDataNode(String uri, JUMPData jumpData) throws IOException {
      File file = uriToDataFile(uri);
      File parentFile = file.getParentFile();

      writeToFile(file, jumpData);
   }

   public void createNode(String uri) throws IOException {
      File file = uriToListFile(uri);
      file.mkdirs();
      if (!file.exists())
         throw new IOException("Could not create: " + file);
   }                  

   public JUMPNode getNode(String uri) throws IOException {
      // getNode(String) needs to return what the createDataNode() above store
      // if the uri parameter represents a data node.
   
      //System.out.println("getNode uri:" + uri);
      if (!isDataUri(uri)) {
          // This URI represents non-leaf node.
          // No caching for now - might want to optimize in the future.
          //JUMPNode node = (JUMPNode) jumpNodeLists.get(uri);
          //if (node == null) {
          //   synchronized(jumpNodeLists) {
          //       node = new JUMPNodeListImpl("list", uri);
          //       jumpNodeLists.put(uri, node);
          //   }
          //}
          return new JUMPNodeListImpl(uri);
      }  else {    
    
          try {
             File file = uriToDataFile(uri);
             String name = getNodeName(uri);

             if (!file.isHidden()) {  // Don't make a node for hidden system files
                JUMPData dataObject = readFromFile(file, name);
                JUMPNode node = new JUMPNodeDataImpl(uri, dataObject);
                return node;
             }

         } catch (IOException e) {
             if (verbose)
                System.err.println(e); // need to do something about exceptions
         }
      }

      return null;
   }

    protected void updateDataNode (String uri, JUMPData data) 
       throws IOException {
          createDataNode(uri, data);
    }


   public void deleteNode(String uri) {
      File file = uriToListFile(uri);
      deleteFile(file, true);
   }

   // deletes everything under this file
   private boolean deleteFile(File file, boolean success) { 
    
       if (file.isDirectory()) {
          File[] list = file.listFiles();
          for (int i = 0; i < list.length; i++) {
             deleteFile(list[i], success);
          }
       } 

       success = file.delete() & success;

       return success;
   }

   String root; // The real path to the store root.

   protected void setStoreRoot(String root) {
      File file = new File(root);
      if (file.exists()) {
          this.root = file.getAbsolutePath();
          return;
      } 
      // What to do about error checking?
      throw new RuntimeException("Cannot set persistent store, "+
                                 "repositoryDir="+root+" does not exist");

   }

   protected File uriToDataFile(String uri) {
      String absolutePath = convertToAbsolutePath(uri, true);
      return new File(absolutePath + File.separatorChar + getNodeName(uri));
   }

   protected File uriToListFile(String uri) {
      String absolutePath = convertToAbsolutePath(uri, false);
      return new File(absolutePath);
   }

   private String getNodeName(String uri) {
      return uri.substring(uri.lastIndexOf(File.separatorChar) + 1);
   }

   private void writeToFile(File file, JUMPData data) throws IOException {
   
      int dataFormat = data.getFormat();
      Object rawDataValue = data.getValue();

      // Special processing for the java.util.Properties format
      if (file.getPath().endsWith(".properties")) {

          Properties prop = (Properties) rawDataValue;
          prop.store(new FileOutputStream(file), 
                     "Generated by " + this.getClass().getName());

      } else {

         // All other cases, simply write out the format, then serialized data.
         ObjectOutputStream oout = new ObjectOutputStream(new FileOutputStream(file));
         oout.writeInt(dataFormat);
         oout.writeObject(rawDataValue);
         oout.close();

      }
   }

   private JUMPData readFromFile(File file, String fileName) throws IOException {

      int format;
      Object value;

      // Special processing for the java.util.Properties format
      if (fileName.endsWith(".properties")) {
         format = JUMPData.FORMAT_SERIALIZABLE;
         Properties prop = new Properties();
         prop.load(new FileInputStream(file));
         value = prop;
      } else {

         // All other cases, simply read in the format, then deserialized data. 
         try {
            ObjectInputStream oin = new ObjectInputStream(new FileInputStream(file));
            format =  oin.readInt();
            value = oin.readObject();
         } catch (java.io.StreamCorruptedException e) {  
            // this means we ran into a file format that's not JUMPNode.Data.
            throw new IOException("Failed to create JUMPNode.Data from " + file.getName());
         } catch (ClassNotFoundException e) {  // can't happen!
            e.printStackTrace();
            return null;
         }  
      }

      return new JUMPData(value, format);
   }

    /*
     * URI string need to start with ".".  Any other restrictions?
     * If isDataUri boolean param is true, then the uri represents a leaf node
     * and the absolute uri returns the value with the last item trimmed down
     * ex. convertToAbsolutePath("./apps/App1", false) and
     * convertToAbsolutePath("./apps/App1/title", true) will both yield
     *  "<root dir>/apps/App1" as a return value.
    **/
    private String convertToAbsolutePath(String uri, boolean isData) {

       if (!uri.startsWith("."))
          throw new IllegalArgumentException("Malformed uri, " + uri);
                                                                                         
       String pathUri;
                                                                                         
       if (isData) {
          pathUri = root.concat(uri.substring(1, uri.lastIndexOf(File.separatorChar)));
       } else {
          pathUri = root.concat(uri.substring(1));
       }
                                                                                         
       return pathUri;
    }

    private boolean isDataUri(String uri) {

       if (!uri.startsWith("."))
          return false;

       String absolutePath = convertToAbsolutePath(uri, false);

       File file = new File(absolutePath);

       if (file.exists() && file.isDirectory())
          return false;

       // need a check on data file exists here.

       return true;
    }

    class JUMPNodeDataImpl implements JUMPNode.Data {
       JUMPData data;
       String uri;

       JUMPNodeDataImpl(String uri, JUMPData data) {
          this.uri = uri;
          this.data = data;
       }

       public boolean containsData() { return true; }
       public String getName() { return getNodeName(uri); }
       public String getURI() { return uri; }
       public JUMPData getData() { return data; } 
       public String toString() { 
          return "JUMPNode.Data (" + uri + "," + data + ")"; 
       }
       public boolean equals(Object obj) {
          if (!(obj instanceof JUMPNode.Data)) return false;
          JUMPNode.Data other = (JUMPNode.Data) obj; 
          return (uri.equals(other.getURI()) 
                  && data.equals(other.getData()));
       }
    }

    class JUMPNodeListImpl implements JUMPNode.List {
       String uri;
       ArrayList children;
       JUMPNodeListImpl(String uri) {
          this.uri = uri;
       }
       public boolean containsData() { return false; }
       public String getName() { return getNodeName(uri); }
       public String getURI() { return uri; }
       public Iterator getChildren() { 
          //if (children == null) {  // should we cache?  What if dir content changes? 
              children = new ArrayList();
              File file = uriToListFile(uri);
              String[] names = file.list();
              for (int i = 0; names != null && i < names.length; i++) {
                 try {
                    JUMPNode node = getNode(uri + File.separatorChar + names[i]);
                    if (node != null)
                        children.add(node);
                 } catch (IOException e) {}
              }
          //}
          return children.iterator();
       }
       public String toString() { 
          return "JUMPNode.List (" + uri +")"; 
       }
       public boolean equals(Object obj) {
          if (!(obj instanceof JUMPNode.List)) return false;
          JUMPNode.List other = (JUMPNode.List) obj; 
          return (uri.equals(other.getURI()));
       }
    }
} 
