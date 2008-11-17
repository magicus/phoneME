/*
 *  @(#)QpWidget.cc	1.5 04/12/20
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
#include "QpWidget.h"
#include "QpWidgetFactory.h"
#include "QtComponentPeer.h"
#include <qapplication.h>
#include <qobjectlist.h>
#include <stdio.h>

typedef struct {
    struct {
        QpWidget *widget;
        int index;
    } in ;
} qtMargsInsertWidgetAt;
        
typedef struct {
    struct {
        QpWidget *p;
        Qt::WFlags f;
        QPoint *pt;
        bool showIt;
    } in ;
} qtMargsReparent ;

typedef struct {
    QFont arg ;
} qtQFontArg;

typedef struct {
    QPalette arg ;
} qtQPaletteArg;

typedef struct {
    QPoint arg ;
} qtQPointArg ;

typedef struct {
    QSize arg ;
} qtQSizeArg ;

typedef struct {
    QRect arg ;
} qtQRectArg ;

typedef struct {
    struct {
        QPoint *pt;
    } in ;
    struct {
        QPoint rvalue;
    } out ;
} qtMargsMapToGlobal;

typedef struct {
    struct {
        QPoint *pt;
    } in ;
    struct {
        QPoint rvalue;
    } out ;
} qtMargsMapFromGlobal;

typedef struct {
    struct {
        const char *name;
    } in;
    struct {
        bool rvalue;
    } out;
} qtMargsIsA;

/*
 * QpWidget methods
 */

QpWidget::QpWidget(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createWidget(parent,
                                                                name,
                                                                flags);
    this->setQWidget(widget);
    warningStringLabel = NULL;   //6233632, 6393054
}

QpWidget::~QpWidget(void) {
}

/*
  6233632, 6393054
*/
void
QpWidget::createWarningLabel(QString warningString) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&warningString;
    invokeAndWait(QpWidget::CreateWarningLabel,argp);
    QT_METHOD_ARGS_FREE(argp);
}
                                                                                                                            
/*
  6233632, 6393054
*/
void
QpWidget::resizeWarningLabel(void) {
    //if (statusBar != NULL) {
    if (warningStringLabel != NULL) {
        QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
        invokeAndWait(QpWidget::ResizeWarningLabel,argp);
        QT_METHOD_ARGS_FREE(argp);
    }
}

/*
  6233632, 6393054
*/
int
QpWidget::warningLabelHeight(void) {
    //if (statusBar == NULL) {
    if (warningStringLabel == NULL) {
        return 0;
    } else {
        QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
        invokeAndWait(QpWidget::WarningLabelHeight, argp);
        int rv = (int)argp->rvalue;
        QT_METHOD_ARGS_FREE(argp);
        return rv;
    }
}

void 
QpWidget::insertWidgetAt(QpWidget *widget, int index) {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertWidgetAt, argp);
    argp->in.widget = widget;
    argp->in.index  = index;
    invokeAndWait(QpWidget::InsertWidgetAt, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void *
QpWidget::parentPeer() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::ParentPeer,argp);
    void *rv = argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv ;
}

QColor &
QpWidget::foregroundColor() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::ForegroundColor, argp);
    QColor *rv = (QColor *)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return *rv;
}

QColor &
QpWidget::backgroundColor() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::BackgroundColor, argp);
    QColor *rv = (QColor *)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return *rv;
}

QFont   
QpWidget::font() {
    QT_METHOD_ARGS_ALLOC(qtQFontArg, argp);
    invokeAndWait(QpWidget::Font, argp);
    QFont rv = argp->arg ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void   
QpWidget::setFont(const QFont &font){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&font ;
    invokeAndWait(QpWidget::SetFont,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QPalette 
QpWidget::palette() {
    QT_METHOD_ARGS_ALLOC(qtQPaletteArg, argp);
    invokeAndWait(QpWidget::Palette, argp);
    QPalette rv = argp->arg ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpWidget::setPalette(const QPalette &palette) {
    QT_METHOD_ARGS_ALLOC(qtQPaletteArg, argp);
    argp->arg = palette ;
    invokeAndWait(QpWidget::SetPalette,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpWidget::setCursor(const QCursor &cursor){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&cursor ;
    invokeAndWait(QpWidget::SetCursor,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QPoint
QpWidget::pos() {
    QT_METHOD_ARGS_ALLOC(qtQPointArg, argp);
    invokeAndWait(QpWidget::Pos, argp);
    QPoint rv = argp->arg;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QPoint
QpWidget::mapToGlobal( const QPoint &pt ) {
    QT_METHOD_ARGS_ALLOC(qtMargsMapToGlobal, argp);
    argp->in.pt = (QPoint *)&pt;
    invokeAndWait(QpWidget::MapToGlobal,argp);
    QPoint rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QPoint
QpWidget::mapFromGlobal( const QPoint &pt ) {
    QT_METHOD_ARGS_ALLOC(qtMargsMapFromGlobal, argp);
    argp->in.pt = (QPoint *)&pt;
    invokeAndWait(QpWidget::MapFromGlobal,argp);
    QPoint rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QSize
QpWidget::sizeHint() {
    QT_METHOD_ARGS_ALLOC(qtQSizeArg, argp);
    invokeAndWait(QpWidget::SizeHint, argp);
    QSize rv = argp->arg;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QRect &
QpWidget::geometry() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::Geometry, argp);
    QRect *rv = (QRect *)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return *rv;
}

void
QpWidget::setGeometry(int x, int y, int w, int h ) {
    QT_METHOD_ARGS_ALLOC(qtQRectArg, argp);
    argp->arg.setRect(x,y,w,h);
    invokeAndWait(QpWidget::SetGeometry,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpWidget::show() {
    invokeAndWait(QpWidget::Show,NULL);
}

void
QpWidget::hide() {
    invokeAndWait(QpWidget::Hide,NULL);
}

void
QpWidget::setEnabled(bool enabled) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)enabled;
    invokeAndWait(QpWidget::SetEnabled,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpWidget::setFocusable(bool enabled) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)enabled;
    invokeAndWait(QpWidget::SetFocusable,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpWidget::setFocus() {
    invokeAndWait(QpWidget::SetFocus,NULL);
}

void
QpWidget::clearFocus() {
    invokeAndWait(QpWidget::ClearFocus,NULL);
}
bool
QpWidget::hasFocus() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::HasFocus, argp);
    bool rv = (bool)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}
    

void
QpWidget::reparent(QpWidget *p, Qt::WFlags f, const QPoint &pt, bool showIt) {
    QT_METHOD_ARGS_ALLOC(qtMargsReparent, argp);
    argp->in.p      = p ;
    argp->in.f      = f ;
    argp->in.pt     = (QPoint *)&pt ;
    argp->in.showIt = showIt;
    invokeAndWait(QpWidget::Reparent,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpWidget::resize(int w, int h) {
    QT_METHOD_ARGS_ALLOC(qtQSizeArg, argp);
    argp->arg.setWidth(w);
    argp->arg.setHeight(h);
    invokeAndWait(QpWidget::Resize,argp);
    QT_METHOD_ARGS_FREE(argp);
}

int
QpWidget::x() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::X, argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int
QpWidget::y() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::Y, argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int
QpWidget::height() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::Height, argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int
QpWidget::heightForWidth(int w) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)w;
    invokeAndWait(QpWidget::HeightForWidth, argp);
    int rv = (int)argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpWidget::raise(){
    invokeAndWait(QpWidget::Raise,NULL);
}

void
QpWidget::lower() {
    invokeAndWait(QpWidget::Lower,NULL);
}

void
QpWidget::stackUnder(QpWidget *w) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)w;
    invokeAndWait(QpWidget::StackUnder,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QSize
QpWidget::frameSize() {
    QT_METHOD_ARGS_ALLOC(qtQSizeArg, argp);
    invokeAndWait(QpWidget::FrameSize, argp);
    QSize rv = argp->arg;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpWidget::setFixedSize(int width, int height) {
    QT_METHOD_ARGS_ALLOC(qtQSizeArg, argp);
    argp->arg.setWidth(width);
    argp->arg.setHeight(height);
    invokeAndWait(QpWidget::SetFixedSize,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpWidget::setMinimumSize(int width, int height) {
    QT_METHOD_ARGS_ALLOC(qtQSizeArg, argp);
    argp->arg.setWidth(width);
    argp->arg.setHeight(height);
    invokeAndWait(QpWidget::SetMinimumSize,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpWidget::setMaximumSize(int width, int height) {
    QT_METHOD_ARGS_ALLOC(qtQSizeArg, argp);
    argp->arg.setWidth(width);
    argp->arg.setHeight(height);
    invokeAndWait(QpWidget::SetMaximumSize,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpWidget::setCaption(QString &caption) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&caption;
    invokeAndWait(QpWidget::SetCaption,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpWidget::showMinimized() {
    invokeAndWait(QpWidget::ShowMinimized,NULL);
}

void
QpWidget:: showNormal() {
    invokeAndWait(QpWidget::ShowNormal,NULL);
}

bool
QpWidget:: isMinimized() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::IsMinimized, argp);
    bool rv = (bool)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpWidget::topLevelWidget() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::TopLevelWidget, argp);
    QWidget * rv = (QWidget *)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int
QpWidget:: testWFlags(int flags) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)flags;
    invokeAndWait(QpWidget::TestWFlags, argp);
    int rv = (int)argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpWidget::update() {
    invokeAndWait(QpWidget::Update,NULL);
}

void
QpWidget::setActiveWindow() {
    invokeAndWait(QpWidget::SetActiveWindow,NULL);
}

bool
QpWidget::isEnabled() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpWidget::IsEnabled, argp);
    bool rv = (bool)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}
bool
QpWidget::isA(const char *name) {
    QT_METHOD_ARGS_ALLOC(qtMargsIsA, argp);
    argp->in.name = (char *)name;
    invokeAndWait(QpWidget::IsA,argp);
    bool rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpWidget::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpWidget, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpObject);

    switch ( mid ) {
    case QpWidget::InsertWidgetAt: {
        qtMargsInsertWidgetAt *a = (qtMargsInsertWidgetAt *)args ;
        execInsertWidgetAt(a->in.widget, a->in.index);
    }
    break ;
    case QpWidget::ParentPeer:
        ((qtMethodReturnValue *)args)->rvalue = execParentPeer();
        break ;
    case QpWidget::ForegroundColor: {
        QColor &rv = execForegroundColor();
        ((qtMethodReturnValue *)args)->rvalue = &rv;
    }
        break ;
    case QpWidget::BackgroundColor: {
        QColor &rv = execBackgroundColor();
        ((qtMethodReturnValue *)args)->rvalue = &rv;
    }
        break ;
    case QpWidget::Font:
        ((qtQFontArg *)args)->arg = execFont();
        break ;
    case QpWidget::SetFont:{
        QFont *font = (QFont *)((qtMethodParam *)args)->param;
        execSetFont(*font);
        }
        break ;
    case QpWidget::Palette:
        ((qtQPaletteArg *)args)->arg = execPalette();
        break ;
    case QpWidget::SetPalette:
        execSetPalette(((qtQPaletteArg *)args)->arg);
        break ;
    case QpWidget::SetCursor: {
        QCursor *cursor = (QCursor *)((qtMethodParam *)args)->param;
        execSetCursor(*cursor);
    }
        break ;
    case QpWidget::Pos:
        ((qtQPointArg *)args)->arg = execPos();
        break ;
    case QpWidget::MapToGlobal: {
        qtMargsMapToGlobal *a = (qtMargsMapToGlobal *)args;
        a->out.rvalue = execMapToGlobal((const QPoint &)*a->in.pt);
    }
        break ;
    case QpWidget::MapFromGlobal: {
        qtMargsMapToGlobal *a = (qtMargsMapToGlobal *)args;
        a->out.rvalue = execMapFromGlobal((const QPoint &)*a->in.pt);
    }
        break ;
    case QpWidget::SizeHint:
        ((qtQSizeArg *)args)->arg = execSizeHint();
        break ;
    case QpWidget::Geometry: {
        QRect &rv = (QRect &)execGeometry();
        ((qtMethodReturnValue *)args)->rvalue = &rv;
    }
        break ;
    case QpWidget::SetGeometry:
        execSetGeometry(((qtQRectArg *)args)->arg.x(),
                        ((qtQRectArg *)args)->arg.y(),
                        ((qtQRectArg *)args)->arg.width(),
                        ((qtQRectArg *)args)->arg.height());
            
        break ;
    case QpWidget::Show:
        execShow();
        break ;
    case QpWidget::Hide:
        execHide();
        break ;
    case QpWidget::SetEnabled:
        execSetEnabled((bool)((qtMethodParam *)args)->param);
        break ;
    case QpWidget::SetFocusable:
        execSetFocusable((bool)((qtMethodParam *)args)->param);
	break;
    case QpWidget::SetFocus:
        execSetFocus();
        break ;
    case QpWidget::ClearFocus:
        execClearFocus();
        break ;
    case QpWidget::HasFocus:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execHasFocus();
        break ;
    case QpWidget::Reparent:{
        qtMargsReparent *a = (qtMargsReparent *)args;
        execReparent(a->in.p, a->in.f, *a->in.pt, a->in.showIt);
    }
        break ;
    case QpWidget::Resize:
        execResize(((qtQSizeArg *)args)->arg.width(),
                   ((qtQSizeArg *)args)->arg.height());
        break ;
    case QpWidget::X:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execX();
        break ;
    case QpWidget::Y:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execY();
        break ;
    case QpWidget::Height:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execHeight();
        break ;
    case QpWidget::HeightForWidth:
        ((qtMethodArgs *)args)->out.rvalue = (void *)
            execHeightForWidth((int)((qtMethodArgs *)args)->in.param);
        break ;
    case QpWidget::Raise:
        execRaise();
        break ;
    case QpWidget::Lower:
        execLower();
        break ;
    case QpWidget::StackUnder:
        execStackUnder((QpWidget *)((qtMethodParam *)args)->param);
        break ;
    case QpWidget::FrameSize:
        ((qtQSizeArg *)args)->arg = execFrameSize();
        break ;
    case QpWidget::SetFixedSize:
        execSetFixedSize(((qtQSizeArg *)args)->arg.width(),
                         ((qtQSizeArg *)args)->arg.height());
        break ;
    case QpWidget::SetMinimumSize:
        execSetMinimumSize(((qtQSizeArg *)args)->arg.width(),
                           ((qtQSizeArg *)args)->arg.height());
        break ;
    case QpWidget::SetMaximumSize:
        execSetMaximumSize(((qtQSizeArg *)args)->arg.width(),
                           ((qtQSizeArg *)args)->arg.height());
        break ;
    case QpWidget::SetCaption:
        execSetCaption(*((QString *)((qtMethodParam *)args)->param));
        break ;
    case QpWidget::ShowMinimized:
        execShowMinimized();
        break ;
    case QpWidget::ShowNormal:
        execShowNormal();
        break ;
    case QpWidget::IsMinimized:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execIsMinimized();
        break ;
    case QpWidget::TestWFlags:
        ((qtMethodArgs *)args)->out.rvalue = (void *)
            execTestWFlags((int)((qtMethodArgs *)args)->in.param);
        break ;
    case QpWidget::TopLevelWidget:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execTopLevelWidget();
        break ;
    case QpWidget::Update:
        execUpdate();
        break;
    case QpWidget::SetActiveWindow:
        execSetActiveWindow();
        break;
    case QpWidget::IsEnabled:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execIsEnabled();
        break ;
    case QpWidget::IsA: {
        qtMargsIsA *a = (qtMargsIsA *)args;
        a->out.rvalue = execIsA((const char *)a->in.name);
        }
        break ;
   case QpWidget::CreateWarningLabel:{
        QString *warningString = (QString *)((qtMethodParam *)args)->param;
        execCreateWarningLabel(*warningString);
        }
        break;
    case QpWidget::ResizeWarningLabel:   //6233632, 6393054
        execResizeWarningLabel();
        break;
    case QpWidget::WarningLabelHeight:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execWarningLabelHeight();
        break;

    default :
        break;
    }
}

void 
QpWidget::execInsertWidgetAt(QpWidget *widget, int index) {
    QPoint pt(0, 0);
    QWidget *compWidget = (QWidget *)widget->getQWidget() ;
    QWidget *thisWidget = (QWidget *)this->getQWidget() ;

    // get the list of all child widgets of the panel
    const QObjectList* panelChildren = thisWidget->children();
    int noChildren = (panelChildren != 0) ? panelChildren->count() :
        0;

    // There seems to be an extra child at the top of the
    // stack, but it is not shown.
    if (noChildren == 0 || noChildren == 1 || index == 0) {
        compWidget->reparent(thisWidget, 0, pt, TRUE);
        compWidget->raise();
    } else if (index >= noChildren - 1 || index <= -1) {
        compWidget->reparent(thisWidget, 0, pt, TRUE);
        compWidget->lower();
    } else {
        // Get the widget at the position specified by index - 1.
        // We go from last to first because the top of the stack
        // is the last item in the list.
        QListIterator<QObject> childIt(*panelChildren);
        childIt.toLast();

        for (int i = noChildren - 1; i > index - 1; --i) {
        --childIt;
        }

        QWidget *prevWidget = (QWidget *) childIt.current();
        compWidget->reparent(thisWidget, 0, pt, TRUE);
        compWidget->stackUnder(prevWidget);
    }
    
    if (warningStringLabel != NULL) {
        warningStringLabel->raise();
    }
}

void *
QpWidget::execParentPeer() {
    QWidget *parent = (QWidget *)getQWidget()->parent();
    QtComponentPeer *ppeer = NULL;
    if ( parent != NULL ) {
        ppeer = QtComponentPeer::getPeerForWidget(parent);
    }
    return ppeer ;
}

QColor &
QpWidget::execForegroundColor() {
    return (QColor &)getQWidget()->foregroundColor();
}

QColor &
QpWidget::execBackgroundColor() {
    return (QColor &)getQWidget()->backgroundColor();
}

QFont   
QpWidget::execFont() {
    return getQWidget()->font();
}

void   
QpWidget::execSetFont(const QFont &font){
    getQWidget()->setFont(font);
}

QPalette 
QpWidget::execPalette() {
    return getQWidget()->palette();
}

void 
QpWidget::execSetPalette(const QPalette &palette) {
    getQWidget()->setPalette(palette);
}

void 
QpWidget::execSetCursor(const QCursor &cursor){
    getQWidget()->setCursor(cursor);
}

QPoint
QpWidget::execPos() {
    return getQWidget()->pos();
}

QPoint
QpWidget::execMapToGlobal( const QPoint &pt ) {
    return getQWidget()->mapToGlobal(pt);
}

QPoint
QpWidget::execMapFromGlobal( const QPoint &pt ) {
    return getQWidget()->mapFromGlobal(pt);
 }

QSize
QpWidget::execSizeHint() {
    return getQWidget()->sizeHint();
}

const QRect &
QpWidget::execGeometry() {
    return getQWidget()->geometry();
}

void
QpWidget::execSetGeometry(int x, int y, int w, int h ) {
    getQWidget()->setGeometry(x,y,w,h);
}

void
QpWidget::execShow() {
    getQWidget()->show();
}

void
QpWidget::execHide() {
    getQWidget()->hide();
}

void
QpWidget::execSetEnabled(bool enabled) {
    getQWidget()->setEnabled(enabled);
}

void 
QpWidget::execSetFocusable(bool focusable) {
    getQWidget()->setFocusPolicy(focusable?QWidget::StrongFocus:QWidget::NoFocus);
}

void 
QpWidget::setFocus(QWidget *widget) {
    QFocusEvent::setReason(QFocusEvent::Other);
    // if it's a frame, don't check before requesting focus
    if (widget->isA("QFrame") ||
        (widget->isFocusEnabled() && !widget->hasFocus())) {
        widget->setFocus();
    } 
}

void
QpWidget:: execSetFocus() {
    setFocus(getQWidget());
}

void 
QpWidget::execClearFocus() {
    QFocusEvent::setReason(QFocusEvent::Other);
    getQWidget()->clearFocus();
}

bool
QpWidget:: execHasFocus() {
    return getQWidget()->hasFocus();
}

void
QpWidget::execReparent(QpWidget *p, Qt::WFlags f, QPoint &pt, bool showIt) {
    QWidget *parent = (p != NULL) ? p->getQWidget() : NULL;
    getQWidget()->reparent(parent, f, pt, showIt);
}

void
QpWidget::execResize(int w, int h) {
    getQWidget()->resize(w,h);
}

int
QpWidget::execX() {
    return getQWidget()->x();
}

int
QpWidget::execY() {
    return getQWidget()->y();
}

int
QpWidget::execHeight() {
    return getQWidget()->height();
}

int
QpWidget::execHeightForWidth(int w) {
    return getQWidget()->heightForWidth(w);
}

void
QpWidget::execRaise(){
    getQWidget()->raise();
    
    if (warningStringLabel != NULL) {
        warningStringLabel->raise();
    }
}

void
QpWidget::execLower() {
    getQWidget()->lower();
}

void
QpWidget::execStackUnder(QpWidget *w) {
    getQWidget()->stackUnder(w->getQWidget());
}

QSize
QpWidget::execFrameSize() {
    return getQWidget()->frameSize();
}

void
QpWidget::execSetFixedSize(int width, int height) {
    getQWidget()->setFixedSize(width, height);
}

void
QpWidget::execSetMinimumSize(int width, int height) {
    getQWidget()->setMinimumSize(width, height);
}

void
QpWidget::execSetMaximumSize(int width, int height) {
    getQWidget()->setMaximumSize(width, height);
}

void
QpWidget::execSetCaption(QString &caption) {
    getQWidget()->setCaption(caption);
}

void
QpWidget::execShowMinimized() {
    getQWidget()->showMinimized();
}

void
QpWidget:: execShowNormal() {
    getQWidget()->showNormal();
}

bool
QpWidget:: execIsMinimized() {
    return getQWidget()->isMinimized();
}

int
QpWidget::execTestWFlags(int flags) {
   return getQWidget()->testWFlags(flags);
}

QWidget *
QpWidget:: execTopLevelWidget() {
    return getQWidget()->topLevelWidget();
}

void
QpWidget::execUpdate() {
    getQWidget()->update();
}

void
QpWidget::execSetActiveWindow() {
    getQWidget()->setActiveWindow();
}

bool
QpWidget::execIsEnabled() {
   return getQWidget()->isEnabled();
}

bool
QpWidget::execIsA(const char *name) {
    return getQWidget()->isA(name);
}

/*
  6233632, 6393054
*/
void
QpWidget::execCreateWarningLabel(QString warningString) {
    QWidget *widget = (QWidget *)this->getQWidget();

    // Create the warning label
    warningStringLabel = new QLabel(warningString, widget, 0, Qt::WStyle_StaysOnTop);
    warningStringLabel->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
    warningStringLabel->setAlignment(Qt::AlignHCenter);
                                                                                                                            
    // Set the color to slightly darker then the widget.
    QColor color = widget->backgroundColor();
    QColor darkColor = color.dark(125);
    warningStringLabel->setBackgroundColor(darkColor);
    warningStringLabel->setGeometry(0,
                                    widget->height() - warningStringLabel->height(),
                                    widget->width(),
                                    warningStringLabel->height());
    warningStringLabel->raise();
}


/*
  6233632, 6393054
*/
void
QpWidget::execResizeWarningLabel(void) {
printf("resize");
    QWidget *widget = (QWidget *)this->getQWidget();
    warningStringLabel->setGeometry(0,
                                    widget->height() - warningStringLabel->height(),
                                    widget->width(),
                                    warningStringLabel->height());
    warningStringLabel->raise();
}
                                                                                                                            
/*
  6233632, 6393054
*/
int
QpWidget::execWarningLabelHeight(void) {
    return warningStringLabel->height();
}
