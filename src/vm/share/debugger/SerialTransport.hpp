/*
 *   
 *
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

/** \class SerialTransport
    Empty class that could be used to implement a serial transport for the
    debugger

*/

#if ENABLE_JAVA_DEBUGGER

class SerialTransport : public Transport {

public:

  bool initialized();
  void close();

  bool char_avail(int timeout);
  int write_bytes(void *buf, int len);
  int write_long(void *buf);
  int write_short(void *buf);
  void flush();
  int read_bytes(void *buf, int len, bool blockflag);
  long read_long(void *buf);
  short read_short(void *buf);
  static void init_serial_transport(SerialTransport *st);

  bool init_transport();
  bool connect_transport(ConnectionType, int);

};

#endif
