/*
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

#include "PPCUnicode.h"


LPWSTR J2WHelper1(LPWSTR lpw, LPWSTR lpj, int offset, int nChars) {
    memcpy(lpw, lpj + offset, nChars*2);
    lpw[nChars] = '\0';
    return lpw;
}

LPWSTR JNI_J2WHelper1(JNIEnv *env, LPWSTR lpwstr, jstring jstr) {

    int len = env->GetStringLength(jstr);

    env->GetStringRegion(jstr, 0, len, (jchar *)lpwstr);
    lpwstr[len] = '\0';

    return lpwstr;
}

jstring JNI_W2JHelper1(JNIEnv *env, LPWSTR lpwstr) {

  jsize jstrLength = wcslen(lpwstr);

  if (jstrLength == 0) {
    jstring newJstring = env->NewStringUTF("");
	return newJstring;    
  }
  else {
	jchar *jstr = new jchar[jstrLength];
	for (int i = 0; i < jstrLength; i++) {
          jstr[i] = lpwstr[i];   
	}
	jstring newJstring = env->NewString(jstr, jstrLength);
	return newJstring;
  }

}

LPWSTR J2WHelper(LPWSTR lpw, LPWSTR lpj,  int nChars) {
    return J2WHelper1(lpw, lpj, 0, nChars);
}

// Translate an ANSI string into a Unicode one.
LPWSTR A2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars) {
    lpw[0] = '\0';
    VERIFY(::MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars));
    return lpw;
}

// Translate a Unicode string into an ANSI one.
LPSTR W2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars) {
    lpa[0] = '\0';
    VERIFY(::WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL));
    return lpa;
}
