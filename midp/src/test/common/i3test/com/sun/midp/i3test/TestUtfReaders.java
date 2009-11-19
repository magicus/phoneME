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

package com.sun.midp.i3test;
import com.sun.cldc.i18n.*;
import java.io.*;

public class TestUtfReaders extends TestCase {


    public String teststr1 = "你好世界";
    public String teststr2 = "привет,мир";
    public String teststr3 = "hello, world!";

    public String hex1(int x) {
        return Integer.toHexString(0xff&x);
    }
    public String hex2(int x) {
        return Integer.toHexString(0xffff&x);
    }
    public void dumpString(String s) {
        String t = "String: ";
        for(int i=0;i<s.length();i++) t += "["+hex2((int)s.charAt(i))+"] ";
        info(t);
    }
    public void dumpBytes(byte[]b) {
        String t = "byte[]: ";
        for(int i=0;i<b.length;i++) t += "["+hex1(b[i])+"] ";
        info(t);
    }

    public void test2way(int strId, String s, String e) {
        try{
            declare("test2way string#"+strId+" "+s+" "+e);
            //dumpString(s);
            byte[] b = s.getBytes(e);
            //dumpBytes(b);
            String t = new String(b,e);
            //dumpString(t);
            int diff = s.compareTo(t);
            assertTrue("strings different", diff == 0);
        }catch(Throwable t) { t.printStackTrace();}
    }

    public void testMark(int strId, String s, String e) {
        try{
            declare("test mark string#"+strId+" "+s+" "+e);
            byte[] b = s.getBytes(e);
            final StreamReader r = //new ReaderUTF16(new ByteArrayInputStream(b));
                    (StreamReader)Class.forName("com.sun.cldc.i18n.j2me."+e+"_Reader").newInstance();
            final ByteArrayInputStream bais = new ByteArrayInputStream(b);
            r.open( bais, "UTF_16");
            //info("bais.markSupported() = "+bais.markSupported());
            assertTrue("markSupported() must return the same value", r.markSupported() == bais.markSupported());
            assertTrue("markSupported() must return true", r.markSupported());
            r.mark(2);
            int c1 = r.read();
            int c2 = r.read();
            r.reset();
            int c3 = r.read();
            int c4 = r.read();
            assertEquals("the first character must be the same",c1,c3);
            assertEquals("the second character must be the same",c2,c4);
            r.reset();
            int c;
            String s2 = "";
            while(-1!=(c=r.read())){s2+=(char)c;}
            //dumpString(s2);
            assertEquals("strings (original and read) must be equal",s,s2);
        }catch(Throwable t) { t.printStackTrace(); }
    }

    /**
     * Runs all the tests.
     */
    public void runTests() {
        String[] enc = {"UTF_16", "UTF_16LE", "UTF_16BE","UTF_8"};
        String[] str = {teststr1, teststr2, teststr3};
        for (int i=0;i<enc.length;i++) {
            for (int j=0;j<str.length;j++) {
                test2way(j,str[j],enc[i]);
                testMark(j,str[j],enc[i]);
            }
        }
    }

}

