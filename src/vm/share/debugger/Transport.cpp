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

# include "incls/_precompiled.incl"
# include "incls/_Transport.cpp.incl"

#if ENABLE_JAVA_DEBUGGER

struct Transport::transport_type Transport::transport_types[1] = {
  {"socket",(void (*)(void *x JVM_TRAPS))SocketTransport::init_transport, (ReturnOop (*)(JVM_SINGLE_ARG_TRAPS))SocketTransport::new_transport},
};

ReturnOop Transport::new_transport(const char *name JVM_TRAPS)
{
  for (int i = 0; i < (sizeof(transport_types)/sizeof(transport_type)); i++) {
    if (strcmp(transport_types[i].name, name) == 0) {
      return transport_types[i]._new_transport(JVM_SINGLE_ARG_CHECK_0);
    }
  }
  return NULL;
}
void Transport::init_all_transports(JVM_SINGLE_ARG_TRAPS) {
  for (int i = 0; i < (sizeof(transport_types)/sizeof(transport_type)); i++) {
    transport_types[i]._init_transport(NULL JVM_CHECK);
  }
}
#endif
