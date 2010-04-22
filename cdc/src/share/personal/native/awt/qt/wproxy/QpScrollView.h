/*
 * @(#)QpScrollView.h	1.11 06/10/25
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
#ifndef _QpSCROLL_VIEW_H_
#define _QpSCROLL_VIEW_H_

#include <qscrollview.h>
#include "QpWidget.h"

class QpScrollView : public QpWidget {
public :
    QpScrollView(QpWidget *parent, char *name="QtScrollView",int flags = 0);

    void addWidget(QpWidget *child);

    /* QScrollView Methods */
    QWidget *viewport();
    void  connectHScrollBar(const char *signal,
                     QObject *receiver, const char *member);
    void  connectVScrollBar(const char *signal,
                     QObject *receiver, const char *member);
    void  setVScrollBarMode( QScrollView::ScrollBarMode );
    void  setHScrollBarMode( QScrollView::ScrollBarMode );
    void  scrollBy( int dx, int dy );
    QPoint  viewportToContents( QPoint p );

    /* QScrollBar Methods (flattened the object model) */
    void setHScrollBarTracking(bool);
    void setVScrollBarTracking(bool);
    bool isHScrollBarVisible(void);
    bool isVScrollBarVisible(void);
    void setHScrollBarSteps(int,int);
    void setVScrollBarSteps(int,int);
    void setHScrollBarRange(int,int);
    void setVScrollBarRange(int,int);
    int  setHScrollBarValue(int);   //6255265
    int  setVScrollBarValue(int);   //6255265
    void updateScrollBars(void);    //6249842
    void childResized(int,int);     //6228838

    static int defaultHScrollBarHeight();
    static int defaultVScrollBarWidth();
protected :
    enum MethodId {
        SOM = QpWidget::EOM,
        AddWidget = QpScrollView::SOM,
        Viewport,
        ConnectHScrollBar,
        ConnectVScrollBar,
        SetVScrollBarMode,
        SetHScrollBarMode,
        ScrollBy,
        ViewportToContents,
        SetHScrollBarTracking,
        SetVScrollBarTracking,
        IsHScrollBarVisible,
        IsVScrollBarVisible,
        SetHScrollBarSteps,
        SetVScrollBarSteps,
        SetHScrollBarRange,
        SetVScrollBarRange,
        SetHScrollBarValue,
        SetVScrollBarValue,
        UpdateScrollBars,   //6249842
        ChildResized,       //6228838
        GetScrollBarWidthAndHeight, // private 
        EOM
    };

    virtual void execute(int method, void *args);
    virtual QWidget *getQWidgetKey() {
        return this->viewport();
    }
private :
    void getScrollBarWidthAndHeight(int *hHeight, int *vWidth);

    static int HScrollBarHeight;
    static int VScrollBarWidth;

    void execAddWidget(QpWidget *child);
    QWidget *execViewport();
    void execConnectHScrollBar(const char *signal,
                     QObject *receiver, const char *member);
    void execConnectVScrollBar(const char *signal,
                     QObject *receiver, const char *member);
    void execSetVScrollBarMode( QScrollView::ScrollBarMode );
    void execSetHScrollBarMode( QScrollView::ScrollBarMode );
    void execScrollBy( int dx, int dy );
    QPoint execViewportToContents( QPoint p );
    void execSetHScrollBarTracking(bool);
    void execSetVScrollBarTracking(bool);
    bool execIsHScrollBarVisible(void);
    bool execIsVScrollBarVisible(void);
    void execSetHScrollBarSteps(int,int);
    void execSetVScrollBarSteps(int,int);
    void execSetHScrollBarRange(int,int);
    void execSetVScrollBarRange(int,int);
    int  execSetHScrollBarValue(int);   //6255265
    int  execSetVScrollBarValue(int);   //6255265
    void execUpdateScrollBars(void);    //6249842
    void execChildResized(int,int);     //6228838
};

#endif
