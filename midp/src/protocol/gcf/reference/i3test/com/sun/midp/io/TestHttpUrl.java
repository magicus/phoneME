/*
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
package com.sun.midp.io;

import com.sun.midp.i3test.TestCase;

public class TestHttpUrl extends TestCase {
    /**
     * Runs all the tests.
     */
    public void runTests() throws Throwable {
        declare("testAbsUrl");
        testAbsUrl();
        declare("testRelUrl");
        testRelUrl();
    }

    /**
     * Tests parsing of different pieces of the absolute URL.
     */
    void testAbsUrl() {
        HttpUrl url;

        info("scheme://machine.domain:8080/path?query#fragment");
        url = new HttpUrl("scheme://machine.domain:8080/path?query#fragment");
        assertEquals("scheme", "scheme", url.scheme);
        assertEquals("authority", "machine.domain:8080", url.authority);
        assertEquals("host", "machine.domain", url.host);
        assertEquals("machine name", "machine", url.machine);
        assertEquals("domain", "domain", url.domain);
        assertEquals("port", 8080, url.port);
        assertEquals("path", "/path", url.path);
        assertEquals("query", "query", url.query);
        assertEquals("fragment", "fragment", url.fragment);

        info("scheme://machine.domain:8080/path?query#");
        url = new HttpUrl("scheme://machine.domain:8080/path?query#");
        assertEquals("scheme", "scheme", url.scheme);
        assertEquals("authority", "machine.domain:8080", url.authority);
        assertEquals("host", "machine.domain", url.host);
        assertEquals("machine name", "machine", url.machine);
        assertEquals("domain", "domain", url.domain);
        assertEquals("port", 8080, url.port);
        assertEquals("path", "/path", url.path);
        assertEquals("query", "query", url.query);
        assertEquals("fragment", null, url.fragment);

        info("scheme://machine.domain:8080/path");
        url = new HttpUrl("scheme://machine.domain:8080/path");
        assertEquals("scheme", "scheme", url.scheme);
        assertEquals("authority", "machine.domain:8080", url.authority);
        assertEquals("host", "machine.domain", url.host);
        assertEquals("machine name", "machine", url.machine);
        assertEquals("domain", "domain", url.domain);
        assertEquals("port", 8080, url.port);
        assertEquals("path", "/path", url.path);
        assertEquals("query", null, url.query);
        assertEquals("fragment", null, url.fragment);

        info("scheme://machine.domain:8080/");
        url = new HttpUrl("scheme://machine.domain:8080/");
        assertEquals("scheme", "scheme", url.scheme);
        assertEquals("authority", "machine.domain:8080", url.authority);
        assertEquals("host", "machine.domain", url.host);
        assertEquals("machine name", "machine", url.machine);
        assertEquals("domain", "domain", url.domain);
        assertEquals("port", 8080, url.port);
        assertEquals("path", "/", url.path);
        assertEquals("query", null, url.query);
        assertEquals("fragment", null, url.fragment);

        info("scheme://machine.domain:8080");
        url = new HttpUrl("scheme://machine.domain:8080");
        assertEquals("scheme", "scheme", url.scheme);
        assertEquals("authority", "machine.domain:8080", url.authority);
        assertEquals("host", "machine.domain", url.host);
        assertEquals("machine name", "machine", url.machine);
        assertEquals("domain", "domain", url.domain);
        assertEquals("port", 8080, url.port);
        assertEquals("path", null, url.path);
        assertEquals("query", null, url.query);
        assertEquals("fragment", null, url.fragment);

        info("scheme://machine.domain");
        url = new HttpUrl("scheme://machine.domain");
        assertEquals("scheme", "scheme", url.scheme);
        assertEquals("authority", "machine.domain", url.authority);
        assertEquals("host", "machine.domain", url.host);
        assertEquals("machine name", "machine", url.machine);
        assertEquals("domain", "domain", url.domain);
        assertEquals("port", -1, url.port);
        assertEquals("path", null, url.path);
        assertEquals("query", null, url.query);
        assertEquals("fragment", null, url.fragment);

        info("scheme://machine");
        url = new HttpUrl("scheme://machine");
        assertEquals("scheme", "scheme", url.scheme);
        assertEquals("authority", "machine", url.authority);
        assertEquals("host", "machine", url.host);
        assertEquals("machine name", "machine", url.machine);
        assertEquals("domain", null, url.domain);
        assertEquals("port", -1, url.port);
        assertEquals("path", null, url.path);
        assertEquals("query", null, url.query);
        assertEquals("fragment", null, url.fragment);

        info("scheme://");
        url = new HttpUrl("scheme://");
        assertEquals("scheme", "scheme", url.scheme);
        assertEquals("authority", null, url.authority);
        assertEquals("host", null, url.host);
        assertEquals("machine name", null, url.machine);
        assertEquals("domain", null, url.domain);
        assertEquals("port", -1, url.port);
        assertEquals("path", null, url.path);
        assertEquals("query", null, url.query);
        assertEquals("fragment", null, url.fragment);

        info("scheme://machine.subdomain.domain");
        url = new HttpUrl("scheme://machine.subdomain.domain");
        assertEquals("authority", "machine.subdomain.domain", url.authority);
        assertEquals("host", "machine.subdomain.domain", url.host);
        assertEquals("machine name", "machine", url.machine);
        assertEquals("domain", "subdomain.domain", url.domain);

        info("scheme://123.domain");
        url = new HttpUrl("scheme://123.domain");
        assertEquals("authority", "123.domain", url.authority);
        assertEquals("host", "123.domain", url.host);
        assertEquals("machine name", "123", url.machine);
        assertEquals("domain", "domain", url.domain);

        info("scheme://1234.5678.901.2345");
        url = new HttpUrl("scheme://1234.5678.901.2345");
        assertEquals("authority", "1234.5678.901.2345", url.authority);
        assertEquals("host", "1234.5678.901.2345", url.host);
        assertEquals("machine name", null, url.machine);
        assertEquals("domain", null, url.domain);

        info("scheme://123");
        url = new HttpUrl("scheme://1234");
        assertEquals("authority", "1234", url.authority);
        assertEquals("host", "1234", url.host);
        assertEquals("machine name", "1234", url.machine);
        assertEquals("domain", null, url.domain);

        // IP v4 address
        info("scheme://123.123");
        url = new HttpUrl("scheme://123.123");
        assertEquals("authority", "123.123", url.authority);
        assertEquals("host", "123.123", url.host);
        assertEquals("machine name", null, url.machine);
        assertEquals("domain", null, url.domain);

        // IP v6 address
        info("scheme://[123]");
        url = new HttpUrl("scheme://[123]");
        assertEquals("authority", "[123]", url.authority);
        assertEquals("host", "[123]", url.host);
        assertEquals("machine name", null, url.machine);
        assertEquals("domain", null, url.domain);

        info("scheme://authority/");
        url = new HttpUrl("scheme://authority/");
        assertEquals("path", "/", url.path);
    }

    /**
     * Tests parsing of different pieces of the relative URL.
     */
    void testRelUrl() {
        HttpUrl url;

        url = new HttpUrl("//authority/path?query#fragment");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", "authority", url.authority);
        assertEquals("path", "/path", url.path);
        assertEquals("query", "query", url.query);
        assertEquals("fragment", "fragment", url.fragment);

        url = new HttpUrl("//authority/path?query");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", "authority", url.authority);
        assertEquals("path", "/path", url.path);
        assertEquals("query", "query", url.query);
        assertEquals("fragment", null, url.fragment);

        url = new HttpUrl("//authority/path");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", "authority", url.authority);
        assertEquals("path", "/path", url.path);
        assertEquals("query", null, url.query);

        url = new HttpUrl("//authority/");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", "authority", url.authority);
        assertEquals("path", "/", url.path);

        url = new HttpUrl("//authority");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", "authority", url.authority);
        assertEquals("path", null, url.path);

        url = new HttpUrl("/path");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", null, url.authority);
        assertEquals("path", "/path", url.path);
        assertEquals("query", null, url.query);

        url = new HttpUrl("/");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", null, url.authority);
        assertEquals("path", "/", url.path);

        url = new HttpUrl("path/subpath");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", null, url.authority);
        assertEquals("path", "path/subpath", url.path);

        url = new HttpUrl("path");
        assertEquals("scheme", null, url.scheme);
        assertEquals("authority", null, url.authority);
        assertEquals("path", "path", url.path);
    }
}
