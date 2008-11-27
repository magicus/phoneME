/*
 * @(#)QtBackEnd.h	1.8 06/10/25
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

#ifndef _QT_BACKEND_H_
#define _QT_BACKEND_H_

#include "QtInterface.h"
#include "awt.h"

#ifndef __QT_BACKEND_CPP__
extern QtGraphDesc *QtGraphDescPool;
extern QtImageDesc *QtImageDescPool;

extern int NumGraphDescriptors;
extern int NumImageDescriptors;
#endif

typedef struct
{
        long keyCode;
        int qtKey;
        int printable;
} KeymapEntry;

extern KeymapEntry keymapTable[];

class QtWindow : public QWidget
{
	QBitmap *transMask;
	
public:
	QtWindow(int flags,
             const char *name = "Sun AWT/Qt",
             QWidget *parent = NULL) ;
	
	virtual void setMask(const QBitmap &);
	virtual QBitmap *mask() const;
	
protected:
	virtual void mousePressEvent(QMouseEvent *);
	virtual void mouseReleaseEvent(QMouseEvent *);
	virtual void mouseMoveEvent(QMouseEvent *);
	virtual void keyPressEvent(QKeyEvent *);
	virtual void keyReleaseEvent(QKeyEvent *);
	/*
	 virtual void paintEvent(QPaintEvent *);
	 */
	
	
};

class QtScreen 
{
protected :
    int m_x;
    int m_y;
    int m_width;
    int m_height;
    QtWindow *m_window;
    bool m_bounds_restricted;
    virtual char **getArgs(int *argc);
    virtual void computeBounds();
    virtual void createQtWindow();
    virtual void windowShown();

public :
    QtScreen();
    bool init();
    void close();
    void showWindow();
    void setMouseCursor(int cursor);
    void beep();
    int width();
    int height();
    int dotsPerInch();
    QtWindow *window();
    int x();
    int y();
};


#endif
