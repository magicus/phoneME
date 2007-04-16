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
 
#ifndef _FILE_INSTALLER_TEST_H_
#define _FILE_INSTALLER_TEST_H_

#if ENABLE_NUTS_FRAMEWORK

#include <midpNUTS.h>
#include <fileInstallerInternal.h>

#ifdef __cplusplus
extern "C" {
#endif

int registerFileInstallerTests();

int testReadJadFile_1(void);
int testReadJadFile_2(void);
int testInstalFileUsingJad(void);
int testInstalFileUsingJar(void);
int testInstalFileUsingBadJad(void);
int testMidpGetVersion_1(void);
int testMidpGetVersion_2(void);
int testMidpGetVersion_3(void);
int testMidpGetVersion_4(void);
int testMidpCompareVersion_1(void);
int testMidpCompareVersion_2(void);
int testMidpCompareVersion_3(void);
int testCreateRelativeURL_1(void);
int testCreateRelativeURL_2(void);
int testParseJad_1(void);
int testParseJad_2(void);
int testParseManifest_1(void);
int testParseManifest_2(void);

#ifdef __cplusplus
}
#endif

#endif /* ENABLE_NUTS_FRAMEWORK */
#endif /* _FILE_INSTALLER_TEST_H_ */

