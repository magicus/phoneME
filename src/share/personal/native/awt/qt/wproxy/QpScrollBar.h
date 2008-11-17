/*
 * @(#)QpScrollBar.h	1.6 06/10/25
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
#ifndef _QpSCROLL_BAR_H_
#define _QpSCROLL_BAR_H_

#include "QpWidget.h"

class QpScrollBar : public QpWidget {
public :
    QpScrollBar(int type, QpWidget *parent, 
                char *name="QtScrollBar",int flags = 0);

    /* QScrollBar Methods */
    void setTracking(bool);
    void setPageStep(int);
    void setLineStep(int);
    int  pageStep(void);
    int  lineStep(void);
    void setMinValue(int);
    void setMaxValue(int);
    void setValue(int);
    
protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        SetTracking = QpScrollBar::SOM,
        SetPageStep,
        SetLineStep,
        PageStep,
        LineStep,
        SetMinValue,
        SetMaxValue,
        SetValue,
        EOM
    };

    virtual void execute(int method, void *args);
private :
    void execSetTracking(bool);
    void execSetPageStep(int);
    void execSetLineStep(int);
    int  execPageStep(void);
    int  execLineStep(void);
    void execSetMinValue(int);
    void execSetMaxValue(int);
    void execSetValue(int);
};
#endif
