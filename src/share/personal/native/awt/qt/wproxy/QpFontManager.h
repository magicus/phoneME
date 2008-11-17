/*
 * @(#)QpFontManager.h	1.8 06/10/25
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
#ifndef _QpFONT_MANAGER_h_
#define _QpFONT_MANAGER_h_

#include <qfont.h>
#include "QpObject.h"

class QpFontManager : public QpObject {
public :
    static QpFontManager *instance() ;
    QpFontManager();

    QFont *createFont(QString family, 
                      int size, 
                      int weight, 
                      bool isItalic,
                      int *ascent,
                      int *descent,
                      int *leading);
    bool isEquals(QFont *f1, QFont *f2);
    void getMetrics(QFont *f, int *ascent, int *descent, int *leading);
    int  charWidth(QFont *f, char c);
    int  stringWidth(QFont *f, QString *str, int stringLen); 
    
protected :
    enum MethodId {
        SOM = QpObject::EOM,
        CreateFont = QpFontManager::SOM,
        IsEquals,
        GetMetrics,
        CharWidth,
        StringWidth,
        EOM
    };

    virtual void execute(int method, void *args);
private :
    QFont *execCreateFont(QString family, 
                          int size, 
                          int weight, 
                          bool isItalic,
                          int *ascent,
                          int *descent,
                          int *leading);
    bool execIsEquals(QFont *f1, QFont *f2);
    void execGetMetrics(QFont *f, int *ascent, int *descent, int *leading);
    int  execCharWidth(QFont *f, char c);
    int  execStringWidth(QFont *f, QString *str, int stringLen); 
    static QpFontManager *DefaultManager; // 6176847
};

#endif
