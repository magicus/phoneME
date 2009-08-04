/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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
package com.sun.midp.util;

import java.io.InputStream;
import java.io.ByteArrayOutputStream;

import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.Permissions;
import com.sun.midp.configurator.Constants;
import com.sun.j2me.security.AccessController;

/**
 * The ResourceHandler class is a system level utility class.
 * Its purpose is to return system resources as an array of bytes
 * based on a unique String identifier. 
 *
 * On the CDC stack these system resources are deployed
 * in lib/resources.jar.
 */
public class ResourceHandler {
    /**
     * Load a resource from the system and return it as a byte array.
     * This method is used to load AMS icons.
     *
     * @param token the SecurityToken to use to grant permission to
     *              execute this method.
     * @param resource a String identifier which can uniquely describe
     *                 the location of the resource to be loaded.
     * @return a byte[] containing the resource retrieved from the
     *         system. null if the resource could not be found.
     */
    public static byte[] getAmsResource(SecurityToken token,
                                        String resource) {
	return getResourceImpl(token, resource);
    }

    /**
     * Load a resource from the system and return it as a byte array.
     * This method is used to load system level resources, such as
     * images, sounds, properties, etc.
     *
     * @param token the SecurityToken to use to grant permission to
     *              execute this method.
     * @param resource a String identifier which can uniquely describe
     *                 the location of the resource to be loaded.
     * @return a byte[] containing the resource retrieved from the
     *         system. null if the resource could not be found.
     */
    public static byte[] getSystemResource(SecurityToken token,
                                           String resource) {
       return getResourceImpl(token, resource);
    }

    /**
     * Load a system image resource from the system and return it as 
     * a byte array. The images are stored in the configuration 
     * directory ($MIDP_HOME/lib).
     *
     * @param token the SecurityToken to use to grant permission to
     *              execute this method.
     * @param imageName name of the image
     * @return a byte[] containing the resource retrieved from the
     *         system. null if the resource could not be found.
     * @throws IllegalArgumentException if imageName contains a "/" or "\\",
     *        or imageName is null or imageName is empty
     */
    public static byte[] getSystemImageResource(SecurityToken token, 
                    String imageName) {

      byte[] imageData = getAmsResource(token, imageName + ".raw");
        if (imageData == null) {
            imageData = getAmsResource(token, imageName + ".png");
        }
        return imageData;
    }   

    /**
     * Load a resource from the system and return it as a byte array.
     * This method is used to load system level resources, such as
     * images, sounds, properties, etc.
     *
     * @param token the SecurityToken to use to grant permission to
     *              execute this method.
     * @param resourceFilename full path to the file containing the resource.
     * @return a byte[] containing the resource retrieved from the
     *         system. null if the resource could not be found.
     */
    private static byte[] getResourceImpl(SecurityToken token,
            String resourceFilename) {
        if (token != null) {
            token.checkIfPermissionAllowed(Permissions.MIDP);
        } else {
            AccessController.checkPermission(Permissions.AMS_PERMISSION_NAME);
        }

        if (resourceFilename == null) {
            throw new IllegalArgumentException("Resource file name is null");
        }

        // converting the file name into the resource name
        int start = resourceFilename.lastIndexOf('/');
        if (start < 0) {
            start = resourceFilename.lastIndexOf('\\');
        }
        if (start < 0) {
            start = 0;
        } else {
            start++;
        }

	// Add a leading slash to fetch top level resources by name
        final String resourceName = "/" + resourceFilename.substring(start,
                resourceFilename.length());

	try {
            InputStream is =
                (InputStream)java.security.AccessController.doPrivileged(
                    new java.security.PrivilegedAction() {
                        public Object run() {
                            return Class.class.getResourceAsStream(
                                resourceName);
                        }
                    });

            if (is == null) {
                /*
                 * Resource could not be found, which is can happen
                 * normally based on how the resources are built, one
                 * example would be if .raw files are used instead of .png.
                 * The code in this case is in SkinResourcesImpl.getImage.
                 */
                return null;
            }

	    ByteArrayOutputStream baos =  new ByteArrayOutputStream(1024);

	    byte[] bytes = new byte[512];
	    int bytesRead;
	    
	    while ((bytesRead = is.read(bytes)) > 0) {
		baos.write(bytes, 0, bytesRead);
	    }
	    
	    return  baos.toByteArray();

	} catch (Exception e) {
	    // Resource found, but could not be read
	    System.err.println("Could not load: " + resourceName);
	    e.printStackTrace();
	}
        return null;
    }
}

