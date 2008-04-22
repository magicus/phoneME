/*
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
 */

package com.sun.tck.wma;

import java.io.*;

/* Referenced classes of package com.sun.tck.wma: SizeExceededException*/

public class MessagePart
{

    void construct(byte abyte0[], int i, int j, String s, String s1, String s2, String s3)
        throws SizeExceededException
    {
        if(j > MAX_PART_SIZE_BYTES)
            throw new SizeExceededException("InputStream data exceeds MessagePart size limit");
        if(s == null)
            throw new IllegalArgumentException("mimeType must be specified");
        checkContentID(s1);
        checkContentLocation(s2);
        if(j < 0)
            throw new IllegalArgumentException("length must be >= 0");
        if(abyte0 != null && i + j > abyte0.length)
            throw new IllegalArgumentException("offset + length exceeds contents length");
        if(i < 0)
            throw new IllegalArgumentException("offset must be >= 0");
        checkEncodingScheme(s3);
        if(abyte0 != null)
        {
            content = new byte[j];
            System.arraycopy(abyte0, i, content, 0, j);
        }
        mimeType = s;
        contentID = s1;
        contentLocation = s2;
        encoding = s3;
    }

    public MessagePart(byte abyte0[], int i, int j, String s, String s1, String s2, String s3)
        throws SizeExceededException
    {
        construct(abyte0, i, j, s, s1, s2, s3);
    }

    public MessagePart(byte abyte0[], String s, String s1, String s2, String s3)
        throws SizeExceededException
    {
        construct(abyte0, 0, abyte0 != null ? abyte0.length : 0, s, s1, s2, s3);
    }

    public MessagePart(InputStream inputstream, String s, String s1, String s2, String s3)
        throws IOException, SizeExceededException
    {
        byte abyte0[] = new byte[0];
        if(inputstream != null)
        {
            ByteArrayOutputStream bytearrayoutputstream = new ByteArrayOutputStream();
            byte abyte1[] = new byte[2048];
            for(int i = 0; (i = inputstream.read(abyte1)) != -1;)
                bytearrayoutputstream.write(abyte1, 0, i);

            abyte0 = bytearrayoutputstream.toByteArray();
        }
        construct(abyte0, 0, abyte0.length, s, s1, s2, s3);
    }

    public byte[] getContent()
    {
        if(content == null)
        {
            return null;
        } else
        {
            byte abyte0[] = new byte[content.length];
            System.arraycopy(content, 0, abyte0, 0, content.length);
            return abyte0;
        }
    }

    public InputStream getContentAsStream()
    {
        if(content == null)
            return new ByteArrayInputStream(new byte[0]);
        else
            return new ByteArrayInputStream(content);
    }

    public String getContentID()
    {
        return contentID;
    }

    public String getContentLocation()
    {
        return contentLocation;
    }

    public String getEncoding()
    {
        return encoding;
    }

    public int getLength()
    {
        return content != null ? content.length : 0;
    }

    public String getMIMEType()
    {
        return mimeType;
    }

    static void checkContentID(String s)
        throws IllegalArgumentException
    {
        if(s == null)
            throw new IllegalArgumentException("contentId must be specified");
        if(s.length() > 100)
            throw new IllegalArgumentException("contentId exceeds 100 char limit");
        if(containsNonUSASCII(s))
            throw new IllegalArgumentException("contentId must not contain non-US-ASCII characters");
        else
            return;
    }

    static void checkContentLocation(String s)
        throws IllegalArgumentException
    {
        if(s != null)
        {
            if(containsNonUSASCII(s))
                throw new IllegalArgumentException("contentLocation must not contain non-US-ASCII characters");
            if(s.length() > 100)
                throw new IllegalArgumentException("contentLocation exceeds 100 char limit");
        }
    }

    static void checkEncodingScheme(String s)
        throws IllegalArgumentException
    {
    }

    static boolean containsNonUSASCII(String s)
    {
        int i = s.length();
        for(int j = 0; j < i; j++)
        {
            char c = s.charAt(j);
            if(c < ' ' || c != (c & 0x7f))
                return true;
        }

        return false;
    }

    static int MAX_PART_SIZE_BYTES = 30720;
    static final int BUFFER_SIZE = 2048;
    byte content[];
    String contentID;
    String contentLocation;
    String encoding;
    String mimeType;
    static final char US_ASCII_LOWEST_VALID_CHAR = 32;
    static final char US_ASCII_VALID_BIT_MASK = 127;

}
