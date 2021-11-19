/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

import dummyCA.Authority;

import java.io.*;
import javax.servlet.http.*;

public class DummyCA extends HttpServlet {

    public void doGet(HttpServletRequest request,
                      HttpServletResponse response) throws IOException {
        response.setContentType("text/html");
        PrintWriter out = response.getWriter();
        out.println("<html><head><title>Last request result</title></head>");
        out.println("<body>" + Status + "</body></html>");
    }

    String Status = "There were no any requests yet.";

    public void doPost(HttpServletRequest request,
                      HttpServletResponse response) throws IOException {

        InputStream is = request.getInputStream();

        byte[] data = new byte[request.getContentLength()];
        is.read(data, 0, data.length);

        Authority.init(this);
        Authority CA = new Authority();

        if (! CA.createCertificate(data)) {
            Status = CA.getStatus();
            response.sendError(HttpServletResponse.SC_INTERNAL_SERVER_ERROR,
                    Status);
            return;
        }

        Status = "<pre><code>" + CA.getStatus() + "</pre></code>";

        response.setContentType("application/octet-stream");
        OutputStream os = response.getOutputStream();

        data = CA.getPkiPath();
        os.write((byte) (data.length >> 8));
        os.write((byte) data.length);
        os.write(data);

        data = CA.getIssuerAndSerialNumber();
        os.write((byte) (data.length >> 8));
        os.write((byte) data.length);
        os.write(data);

        os.flush();
    }
}
