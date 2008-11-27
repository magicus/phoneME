/*
 * @(#)time_md.cpp	1.4 06/10/10
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
 *
 */

extern "C" {
#include "javavm/include/porting/time.h"
}
#include "time_impl.h"

TInt64 SYMBIANjavaEpoc;

CVMInt64
CVMtimeMillis(void)
{
    TTime now;
    now.HomeTime();
    TInt64 t = now.Int64();
    TInt64 millis = (t - SYMBIANjavaEpoc) / 1000;
#ifdef EKA2
    return (CVMInt64)millis;
#else
    CVMInt64 delta = millis.Low();
    delta += (CVMInt64)millis.High() << 32;
    return delta;
#endif
}
