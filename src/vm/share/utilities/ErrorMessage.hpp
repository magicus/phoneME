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

/**
 * Obtain a string representation of an error code.
 * This may be a textual description or simply a number in string form
 * depending on whether we deem enough space available for the whole text.
 *
 * See also the declaration of 'ErrorMsgTag'.
 */
class ErrorMessage : public AllStatic {
 public:
  static char* get(ErrorMsgTag err) {
#if !USE_VERBOSE_ERROR_MSG
    jvm_sprintf(_buf, "%3d", err);
    return _buf;
#else
    return (char*)err;
#endif
  }

 private:
#if !USE_VERBOSE_ERROR_MSG
  static char _buf[];
#endif
};
