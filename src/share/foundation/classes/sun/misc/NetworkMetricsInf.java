/*
 * @(#)NetworkMetricsImpl.java	1.1 09/06/10
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
 *
 */

package sun.misc;

import javax.microedition.io.StreamConnection;
import java.net.URL;
import java.net.Socket;

public interface NetworkMetricsInf {

    public static final int HTTP  = 0;
    public static final int HTTPS = 1;

    public static final int GET   = 0;
    public static final int POST  = 1;
    public static final int HEAD  = 2;

    public void initReq(int proto, String host, int port, String file,
                        String ref, String query);

    public void initReq(int proto, URL url);

    public void sendMetricReq(StreamConnection sc, int methodType,
                              int contentLength);

    public void sendMetricReq(Socket serverSocket, int methodType,
                              int contentLength);

    public void sendMetricResponse(StreamConnection sc,
                                   int code, long size);

    public void sendMetricResponse(Socket serverSocket,
                                   int code, long size);

}