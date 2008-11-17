/*
 * @(#)awt.cc	1.8 06/10/25
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
#include "awt.h"

/*
 * WARNING: JNI_OnLoad and JNI_OnUnload must remain empty because they
 * will not be called in fully romized builds.
 */
#if 0
JNIEXPORT jint JNICALL 
JNI_OnLoad(JavaVM *jvm, void *reserved) 
{
    return JNI_VERSION_1_2;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM *jvm, void *reserved)
{
}
#endif

QString *
awt_convertToQString(JNIEnv *env, jstring str)
{
    if (str == NULL) {
	return NULL;
    }
    

    jsize jstrLength = env->GetStringLength(str);

    if (jstrLength == 0)
	return new QString("");

    jchar *jstr = (jchar *) env->GetStringChars(str, NULL);

    // do we need this if jstrLength is 0?
    if (jstr == NULL)
	return new QString("");

    QChar *jstrArray = new QChar[jstrLength];   
    for (int i = 0; i < jstrLength; i++) {
	QChar ch((ushort)jstr[i]);
	jstrArray[i] = ch;
    }

    QString *qstring_return = new QString(jstrArray, (uint)jstrLength);
    env->ReleaseStringChars(str, jstr);
    delete [] jstrArray;   //6228129

    // Don't forget to delete() this reference in your code when finished.
    return qstring_return;  
}
  
jstring
awt_convertToJstring(JNIEnv *env, QString &str)
{
   
    jsize jstrLength = str.length();

    if (jstrLength == 0) {
	jstring newJstring = env->NewStringUTF("");
	return newJstring;    
    }
    else {
	jchar *jstr = new jchar[jstrLength];
	for (int i = 0; i < jstrLength; i++) {
	    QChar c = str.at(i);
	    jstr[i] = c.unicode();   
	}
	jstring newJstring = env->NewString(jstr, jstrLength);
	return newJstring;
    }
} 
