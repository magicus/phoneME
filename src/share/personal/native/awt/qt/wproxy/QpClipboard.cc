/*
 * @(#)QpClipboard.cc	1.8 06/10/25
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

#include <qclipboard.h>
#include <qapplication.h>
#include "QpClipboard.h"

typedef struct {
    QString arg ;
} qtQStringArg ;

// For qt-3.3 setText() and text() API with an added QClipboard::Mode parameter.
#if (QT_VERSION >= 0x030000)
typedef struct {
    struct {
	QString *str;
	QClipboard::Mode mode;
    } in;
} qtMargsSetText_Mode;

typedef struct {
    struct {
	QClipboard::Mode mode;
    } in;
    struct {
	QString rvalue;
    } out;
} qtMargsText_Mode;
#endif

QpClipboard *QpClipboard::SystemClipboard = new QpClipboard(); // 6176847

QpClipboard *
QpClipboard::instance() {
    return QpClipboard::SystemClipboard; // 6176847
}

/*
 * QpClipboard Methods
 */
QpClipboard::QpClipboard() {
    // get the system clip board
    this->m_qobject = QApplication::clipboard();
}

void
QpClipboard::setText(QString *text) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)text;
    invokeAndWait(QpClipboard::SetText, argp);
    QT_METHOD_ARGS_FREE(argp);
}

QString
QpClipboard::text(){
    QT_METHOD_ARGS_ALLOC(qtQStringArg, argp);
    invokeAndWait(QpClipboard::Text, argp);
    QString rv = argp->arg;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

#if (QT_VERSION >= 0x030000)
void
QpClipboard::setText(QString *text, QClipboard::Mode mode) {
    QT_METHOD_ARGS_ALLOC(qtMargsSetText_Mode, argp);
    argp->in.str = text;
    argp->in.mode = mode;
    invokeAndWait(QpClipboard::SetText_Mode, argp);
    QT_METHOD_ARGS_FREE(argp);
}

QString
QpClipboard::text(QClipboard::Mode mode){
    QT_METHOD_ARGS_ALLOC(qtMargsText_Mode, argp);
    argp->in.mode = mode;
    invokeAndWait(QpClipboard::Text_Mode, argp);
    QString rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}
#endif

void
QpClipboard::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpClipboard, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpObject);

    switch ( mid ) {
    case QpClipboard::SetText: {
        qtMethodParam *a = (qtMethodParam *)args ;
        execSetText(((QString *)a->param)) ;
        }
        break;
    case QpClipboard::Text:{
        qtQStringArg *a = (qtQStringArg *)args ;
        a->arg = execText();
        }
        break;
#if (QT_VERSION >= 0x030000)
    case QpClipboard::SetText_Mode: {
        qtMargsSetText_Mode *a = (qtMargsSetText_Mode *)args;
        execSetText(a->in.str, a->in.mode);
        }
        break;
    case QpClipboard::Text_Mode:{
        qtMargsText_Mode *a = (qtMargsText_Mode *)args ;
        a->out.rvalue = execText(a->in.mode);
        }
        break;
#endif
    default :
        break;
    }
}

void
QpClipboard::execSetText(QString *str) {
    QClipboard *clipboard = (QClipboard *)this->m_qobject;
    clipboard->setText(*str);
}

QString
QpClipboard::execText() {
    QClipboard *clipboard = (QClipboard *)this->m_qobject;
    return clipboard->text();
}

#if (QT_VERSION >= 0x030000)
void
QpClipboard::execSetText(QString *str, QClipboard::Mode mode) {
    QClipboard *clipboard = (QClipboard *)this->m_qobject;
    clipboard->setText(*str, mode);
}

QString
QpClipboard::execText(QClipboard::Mode mode) {
    QClipboard *clipboard = (QClipboard *)this->m_qobject;
    return clipboard->text(mode);
}
#endif
