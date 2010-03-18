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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include "lfpport_qte_util.h"
#include <midpMalloc.h>
#include <midpUtilKni.h>
#include <qstring.h>
#include <qfontmetrics.h>
#include <qfont.h>
#include <qwidget.h>
#include <qwsdecoration_qws.h>
#include <qapplication.h>

extern "C" void
pcsl_string2QString(const pcsl_string &pstring, QString &qstring) {
    /*
     * Example From the QT toolkit doc for QString.isNull()
     * QString a;          // a.unicode() == 0,  a.length() == 0
     * QString b = "";     // b.unicode() == "", b.length() == 0
     * a.isNull();         // TRUE, because a.unicode() == 0
     * a.isEmpty();        // TRUE, because a.length() == 0
     * b.isNull();         // FALSE, because b.unicode() != 0
     * b.isEmpty();        // TRUE, because b.length() == 0
     */
    if (pcsl_string_is_null(&pstring)) {
        // we want isNull to be true
	qstring = (const char *)NULL;
    } else if (pcsl_string_length(&pstring) == 0) {
        // we want isEmpty to be true
        qstring = "";
    } else {
        const pcsl_string* const mmstring = &pstring;
        GET_PCSL_STRING_DATA_AND_LENGTH(mmstring)
        qstring.setUnicodeCodes((const ushort *)mmstring_data, mmstring_len);
        RELEASE_PCSL_STRING_DATA_AND_LENGTH
    }
}

extern "C" MidpError
QString2pcsl_string(QString &qstring, pcsl_string &pstring) {
    pcsl_string_status pe;
    if (qstring.isNull()) {
	    pstring = PCSL_STRING_NULL;
    } else if (qstring.isEmpty()) {
	    pstring = PCSL_STRING_EMPTY;
    } else {
        jint mstring_len = qstring.length();
        jchar* mstring_data = (jchar *)midpMalloc(sizeof(jchar) * mstring_len);
        if (mstring_data == NULL) {
	        pstring = PCSL_STRING_NULL;
            return KNI_ENOMEM;
        } else {
            for (int i = 0; i < mstring_len; i++) {
            mstring_data[i] = qstring[i].unicode();
            }
            pe = pcsl_string_convert_from_utf16(mstring_data,
                                                mstring_len, &pstring);
            midpFree(mstring_data);
            if (PCSL_STRING_OK != pe) {
                return KNI_ENOMEM;
            }
        }
    }
    return KNI_OK;
}


PCSL_DEFINE_ASCII_STRING_LITERAL_START(truncmark)
{ 0x2026, 0 }
PCSL_DEFINE_ASCII_STRING_LITERAL_END(truncmark);

void truncateQString(QString & str, QFont font, int widthLimit) {
    QFontMetrics fm(font);
    if (fm.width(str) > widthLimit) {
        QString qTruncMark;
        pcsl_string2QString(truncmark, qTruncMark);
        int truncMarkWidth = fm.width(qTruncMark,1);
        int len = str.length();
        while (len>0 && fm.width(str,len)>widthLimit-truncMarkWidth) {
            len--;
        }
        str.truncate(len);
        str.append(qTruncMark);
    }
}

int calculateCaptionWidth(QWidget * widget) {
    QWSDecoration & decor = QApplication::qwsDecoration();
    QRect winRect = widget->geometry();
    QRect rect = decor.region(widget,winRect,QWSDecoration::Title).boundingRect();
    return rect.width()-PAD_CAPTION; // take into account space between buttons
                           // and around the text
}
