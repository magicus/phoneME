/*
 * @(#)QpFontManager.cc	1.8 06/10/25
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

#include <qfontmetrics.h>
#include <qapplication.h>
#include "QpFontManager.h"

typedef struct {
    struct {
        QString *family;
        int size;
        int weight;
        bool isItalic;
    } in ;
    struct {
        QFont *rvalue;
        int   ascent;
        int   descent;
        int   leading;
    } out ;
} qtMargsCreateFont;

typedef struct {
    struct {
        QFont *font1;
        QFont *font2;
    } in ;
    struct {
        bool rvalue;
    } out ;
} qtMargsIsEquals;

typedef struct {
    struct {
        QFont *font;
    } in ;
    struct {
        int   ascent;
        int   descent;
        int   leading;
    } out ;
} qtMargsGetMetrics;

typedef struct {
    struct {
        QFont *font;
        char  c;
    } in ;
    struct {
        int rvalue;
    } out ;
} qtMargsCharWidth;

typedef struct {
    struct {
        QFont *font;
        QString *str;
        int strLen;
    } in ;
    struct {
        int rvalue;
    } out ;
} qtMargsStringWidth;

QpFontManager *QpFontManager::DefaultManager = new QpFontManager();// 6176847

QpFontManager *
QpFontManager::instance() {
    return QpFontManager::DefaultManager; // 6176847
}

/*
 * QpFontManager Methods
 */
QpFontManager::QpFontManager() {
}

QFont *
QpFontManager::createFont(QString family,
                          int size,
                          int weight,
                          bool isItalic,
                          int *ascent,
                          int *descent,
                          int *leading) {
    QT_METHOD_ARGS_ALLOC(qtMargsCreateFont, argp);
    argp->in.family = &family;
    argp->in.size   = size;
    argp->in.weight = weight;
    argp->in.isItalic = isItalic;
    invokeAndWait(QpFontManager::CreateFont, argp);
    QFont *rv = argp->out.rvalue;
    *ascent   = argp->out.ascent;
    *descent  = argp->out.descent;
    *leading  = argp->out.leading
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

bool 
QpFontManager::isEquals(QFont *font1, QFont *font2){
    QT_METHOD_ARGS_ALLOC(qtMargsIsEquals, argp);
    argp->in.font1 = font1;
    argp->in.font2 = font2;
    invokeAndWait(QpFontManager::IsEquals, argp);
    bool rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpFontManager::getMetrics(QFont *font, int *ascent, int *descent, int *leading){
    QT_METHOD_ARGS_ALLOC(qtMargsGetMetrics, argp);
    argp->in.font = font;
    invokeAndWait(QpFontManager::GetMetrics, argp);
    *ascent   = argp->out.ascent;
    *descent  = argp->out.descent;
    *leading  = argp->out.leading;
    QT_METHOD_ARGS_FREE(argp);
}

int  
QpFontManager::charWidth(QFont *font, char c){
    QT_METHOD_ARGS_ALLOC(qtMargsCharWidth, argp);
    argp->in.font = font;
    argp->in.c    = c;
    invokeAndWait(QpFontManager::CharWidth, argp);
    int rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int 
QpFontManager::stringWidth(QFont *font, QString *str, int strLen){
    QT_METHOD_ARGS_ALLOC(qtMargsStringWidth, argp);
    argp->in.font      = font;
    argp->in.str       = str;
    argp->in.strLen = strLen;
    invokeAndWait(QpFontManager::StringWidth, argp);
    int rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpFontManager::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpFontManager, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpObject);

    switch ( mid ) {
    case QpFontManager::CreateFont: {
        qtMargsCreateFont *a = (qtMargsCreateFont*)args ;
        a->out.rvalue = execCreateFont(*a->in.family,
                                       a->in.size,
                                       a->in.weight,
                                       a->in.isItalic,
                                       &a->out.ascent,
                                       &a->out.descent,
                                       &a->out.leading);
        }
        break;
    case QpFontManager::IsEquals:{
        qtMargsIsEquals *a = (qtMargsIsEquals *)args ;
        a->out.rvalue = execIsEquals(a->in.font1, a->in.font2);
        }
        break;
    case QpFontManager::GetMetrics:{
        qtMargsGetMetrics *a = (qtMargsGetMetrics *)args ;
        execGetMetrics(a->in.font, &a->out.ascent, &a->out.descent, &a->out.leading);
        }
        break;
    case QpFontManager::CharWidth:{
        qtMargsCharWidth *a = (qtMargsCharWidth *)args ;
        a->out.rvalue = execCharWidth(a->in.font, a->in.c);
        }
        break;
    case QpFontManager::StringWidth:{
        qtMargsStringWidth *a = (qtMargsStringWidth *)args ;
        a->out.rvalue = execStringWidth(a->in.font,a->in.str,a->in.strLen);
        }
        break;
    default :
        break;
    }
}
QFont *
QpFontManager::execCreateFont(QString family,
                              int size,
                              int weight,
                              bool isItalic,
                              int *ascent,
                              int *descent,
                              int *leading){
    QFont *font = new QFont(family, size, weight, isItalic);
    if (font == NULL) {
        /* Try to load the default font */
        font = new QFont(QApplication::font());
    }

    if (font != NULL ) {
        QFontMetrics fm(*font);
        *ascent  = fm.ascent();
        *descent = fm.descent();
        *leading = fm.leading();
    }

    return font;
}

bool 
QpFontManager::execIsEquals(QFont *font1, QFont *font2){
    return (*font1 == *font2);
}

void 
QpFontManager::execGetMetrics(QFont *font, int *ascent, int *descent, int *leading){
    QFontMetrics fm(*font);
    *ascent  = fm.ascent();
    *descent = fm.descent();
    *leading = fm.leading();
}

int  
QpFontManager::execCharWidth(QFont *font, char c){
    QFontMetrics fm(*font);
    return fm.width(c);
}

int  
QpFontManager::execStringWidth(QFont *font, QString *str, int strLen){
    QFontMetrics fm(*font);
    return fm.width(*str,strLen);
}
   
