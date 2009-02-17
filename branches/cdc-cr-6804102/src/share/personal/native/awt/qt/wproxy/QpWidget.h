/*
 *  @(#)QpWidget.h	1.5 04/12/20
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
#ifndef _QpWIDGET_H_
#define _QpWIDGET_H_

#include <qwidget.h>
#include <qlabel.h>   //6233632
#include "QpObject.h"

// QpWidget::getQWidget() is "protected" and C++ allows only references
// of a concrete class to access the method from the concere class context
// It does not allow to reference the method if the object is of supertype
// (say QpWidget) unlike java. The following solves the problem without
// relaxing the accessibility for the methos
#define AWT_GET_QWIDGET(_class, _qtwidget) ((_class *)_qtwidget)->getQWidget()

/**
 */
class QpWidget : public QpObject {
    // The following classes are friends to get access to the protected
    // accessor and mutator methods.
    friend class QtApplication ;
    friend class QtComponentPeer ;
    friend class QtGraphics;

    // To get access to getQWidget() method
    friend class QpWidgetFactory;

    // To get access to setFocus(QWidget *) method
    friend class QpRobot;
private :
    static void setFocus(QWidget *widget);

public :
    QpWidget(QpWidget *parent, char *name = NULL, int flags = 0);
    virtual ~QpWidget();

    /*
     * Insert the widget specified at the position within "this" widget
     */  
    void insertWidgetAt(QpWidget *widget, int index);

    /*
     * Returns the "QtComponentPeer" associated with the "this" object's
     * paren QWidget.
     */
    void *parentPeer() ;

    /* QWidget Methods */
    void createWarningLabel(QString warningString); //6233632
    void resizeWarningLabel(void);                  //6233632
    int  warningLabelHeight(void);                  //6233632
    QColor & foregroundColor() ;
    QColor & backgroundColor() ;
    QFont  font()            ;
    void   setFont(const QFont &);
    QPalette palette() ;
    void setPalette(const QPalette &);
    void setCursor(const QCursor &);
    QPoint pos() ;
    QPoint mapToGlobal( const QPoint & ) ;
    QPoint mapFromGlobal( const QPoint & ) ;
    QSize   sizeHint() ;
    QRect & geometry() ;
    void setGeometry(int x, int y, int w, int h );
    void show();
    void hide();
    void setEnabled(bool);
    void setFocusable(bool);
    void setFocus();
    void clearFocus();
    bool hasFocus();
    void reparent(QpWidget *, Qt::WFlags,
                  const QPoint &, bool showIt=FALSE );
    void resize(int w, int h);
    int  x();
    int  y();
    int  height();
    int  heightForWidth(int) ;
    void raise();
    void lower();
    void stackUnder(QpWidget *);
    QSize frameSize();
    void setFixedSize(int width, int height); 
    void setMinimumSize(int width, int height);
    void setMaximumSize(int width, int height);
    void setCaption(QString &);
    void showMinimized();
    void showNormal();
    bool isMinimized() ;
    int  testWFlags(int flags) ;
    QWidget *topLevelWidget();
    void update();
    void setActiveWindow();
    bool isEnabled();
    bool isA(const char *name);

protected :
    QpWidget() { warningStringLabel = NULL; }
    enum MethodId {
        SOM = QpObject::EOM,
        InsertWidgetAt = QpWidget::SOM,
        ParentPeer,
        ForegroundColor,
        BackgroundColor,
        Font,
        SetFont,
        Palette,
        SetPalette,
        SetCursor,
        Pos,
        MapToGlobal,
        MapFromGlobal,
        SizeHint,
        Geometry,
        SetGeometry,
        Show,
        Hide,
        SetEnabled,
        SetFocusable,
        SetFocus,
        ClearFocus,
        HasFocus,
        Reparent,
        Resize,
        X,
        Y,
        Height,
        HeightForWidth,
        Raise,
        Lower,
        StackUnder,
        FrameSize,
        SetFixedSize,
        SetMinimumSize,
        SetMaximumSize,
        SetCaption,
        ShowMinimized,
        ShowNormal,
        IsMinimized,
        TestWFlags,
        TopLevelWidget,
        Update,
        SetActiveWindow,
        IsEnabled,
        IsA,
        CreateWarningLabel,
        ResizeWarningLabel,
        WarningLabelHeight,
        EOM
    };

    QWidget *getQWidget() { return (QWidget *)m_qobject;}
    virtual QWidget *getQWidgetKey() { return getQWidget(); }
    void setQWidget(QWidget *widget) { this->m_qobject = widget ; }
    virtual void execute(int methodId, void *args) ;

private:
    QLabel *warningStringLabel;   //6233632

    void execCreateWarningLabel(QString warningString); //6233632
    void execResizeWarningLabel(void);                  //6233632
    int  execWarningLabelHeight(void);                  //6233632
    void execInsertWidgetAt(QpWidget *widget, int index);
    void *execParentPeer() ;
    QColor & execForegroundColor() ;
    QColor & execBackgroundColor() ;
    QFont    execFont()            ;
    void     execSetFont(const QFont &);
    QPalette execPalette() ;
    void execSetPalette(const QPalette &);
    void execSetCursor(const QCursor &);
    QPoint execPos() ;
    QPoint execMapToGlobal( const QPoint & ) ;
    QPoint execMapFromGlobal( const QPoint & ) ;
    QSize  execSizeHint() ;
    const QRect &execGeometry() ;
    void execSetGeometry(int x, int y, int w, int h );
    void execShow();
    void execHide();
    void execSetEnabled(bool);
    void execSetFocusable(bool);
    void execSetFocus();
    void execClearFocus();
    bool execHasFocus();
    void execReparent(QpWidget *, Qt::WFlags,
                      QPoint &, bool showIt=FALSE );
    void execResize(int w, int h);
    int  execX();
    int  execY();
    int  execHeight();
    int  execHeightForWidth(int) ;
    void execRaise();
    void execLower();
    void execStackUnder(QpWidget *);
    QSize execFrameSize();
    void execSetFixedSize(int width, int height); 
    void execSetMinimumSize(int width, int height);
    void execSetMaximumSize(int width, int height);
    void execSetCaption(QString &);
    void execShowMinimized();
    void execShowNormal();
    bool execIsMinimized() ;
     int  execTestWFlags(int flags) ;
    QWidget *execTopLevelWidget();
    void execUpdate(void);
    void execSetActiveWindow(void);
    bool execIsEnabled();
    bool execIsA(const char *name);   
};

#endif /* _QpWIDGET_H_ */
