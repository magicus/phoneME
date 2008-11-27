/*
 * @(#)QpScrollView.cc	1.12 06/10/25
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

#include  <qscrollview.h>
#include "QpWidgetFactory.h"
#include "QpScrollView.h"

typedef struct {
    QPoint arg;
} qtQPointArg ;

typedef struct {
    struct {
        int line;
        int page;
    } in ;
} qtMArgsSetScrollBarSteps ;

typedef struct {
    struct {
        int min;
        int max;
    } in ;
} qtMArgsSetScrollBarRange ;

typedef struct {
    struct {
        int *hBarHeight;
        int *vBarWidth;
    } out;
} qtMargsScrollBarWidthAndHeight;

typedef struct {
    struct {
        const char *signal;
        QObject *receiver;
        const char *member;
    } in ;
} qtMargsConnectScrollBar;

typedef struct {
    struct {
        QPoint *pt;
    } in ;
    struct {
        QPoint rvalue;
    } out ;
} qtMargsViewportToContents;

typedef struct {
    struct {
        int width;
        int height;
    } in ;
} qtMargsChildResized;

int QpScrollView::HScrollBarHeight = -1 ;
int QpScrollView::VScrollBarWidth  = -1 ;

int
QpScrollView::defaultHScrollBarHeight() {
    if ( QpScrollView::HScrollBarHeight < 0 ) {
        QpScrollView sview(NULL) ;
        sview.getScrollBarWidthAndHeight(&QpScrollView::HScrollBarHeight,
                                         &QpScrollView::VScrollBarWidth);
    }
    return HScrollBarHeight;
}

int
QpScrollView::defaultVScrollBarWidth() {
    if ( QpScrollView::VScrollBarWidth< 0 ) {
        QpScrollView sview(NULL) ;
        sview.getScrollBarWidthAndHeight(&QpScrollView::HScrollBarHeight,
                                         &QpScrollView::VScrollBarWidth);
    }
    return VScrollBarWidth;
}

/*
 * QpScrollView Methods
 */
QpScrollView::QpScrollView(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createScrollView(parent,
                                                                    name,
                                                                    flags);
    this->setQWidget(widget);
}

void 
QpScrollView::addWidget(QpWidget *child) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)child;
    invokeAndWait(QpScrollView::AddWidget,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QWidget *
QpScrollView::viewport() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpScrollView::Viewport,argp);
    QWidget *rv = (QWidget *)argp->rvalue ; 
    QT_METHOD_ARGS_FREE(argp);
    return rv;     
}   

void
QpScrollView::connectHScrollBar(const char *signal, QObject *receiver, const char *member){
    QT_METHOD_ARGS_ALLOC(qtMargsConnectScrollBar, argp);
    argp->in.signal   = signal ;
    argp->in.receiver = receiver ;
    argp->in.member   = member ;
    invokeAndWait(QpScrollView::ConnectHScrollBar, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpScrollView::connectVScrollBar(const char *signal, QObject *receiver, const char *member){
    QT_METHOD_ARGS_ALLOC(qtMargsConnectScrollBar, argp);
    argp->in.signal   = signal ;
    argp->in.receiver = receiver ;
    argp->in.member   = member ;
    invokeAndWait(QpScrollView::ConnectVScrollBar, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void  
QpScrollView::setVScrollBarMode( QScrollView::ScrollBarMode mode){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)mode);
    invokeAndWait(QpScrollView::SetVScrollBarMode,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void  
QpScrollView::setHScrollBarMode( QScrollView::ScrollBarMode mode){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)mode);
    invokeAndWait(QpScrollView::SetHScrollBarMode,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollView:: scrollBy( int dx, int dy ){
    QT_METHOD_ARGS_ALLOC(qtQPointArg, argp);
    QPoint pt(dx,dy);
    argp->arg = pt;
    invokeAndWait(QpScrollView::ScrollBy,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QPoint 
QpScrollView:: viewportToContents( QPoint p ){
    QT_METHOD_ARGS_ALLOC(qtMargsViewportToContents, argp);
    argp->in.pt = (QPoint*) &p;
    invokeAndWait(QpScrollView::ViewportToContents,argp);
    QPoint rp = (QPoint)argp->out.rvalue; 
    QT_METHOD_ARGS_FREE(argp);
    return rp;
}

void 
QpScrollView::setHScrollBarTracking(bool track){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)track;
    invokeAndWait(QpScrollView::SetHScrollBarTracking,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollView::setVScrollBarTracking(bool track){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)track;
    invokeAndWait(QpScrollView::SetVScrollBarTracking,argp);
    QT_METHOD_ARGS_FREE(argp);
}

bool 
QpScrollView::isHScrollBarVisible(void){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpScrollView::IsHScrollBarVisible,argp);
    bool rv = (bool)argp->rvalue ; 
    QT_METHOD_ARGS_FREE(argp);
    return rv;     
}

bool 
QpScrollView::isVScrollBarVisible(void){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpScrollView::IsVScrollBarVisible,argp);
    bool rv = (bool)argp->rvalue ; 
    QT_METHOD_ARGS_FREE(argp);
    return rv;     
}

void 
QpScrollView::setHScrollBarSteps(int line,int page){
    QT_METHOD_ARGS_ALLOC(qtMArgsSetScrollBarSteps, argp);
    argp->in.line = line;
    argp->in.page = page;
    invokeAndWait(QpScrollView::SetHScrollBarSteps,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollView::setVScrollBarSteps(int line,int page){
    QT_METHOD_ARGS_ALLOC(qtMArgsSetScrollBarSteps, argp);
    argp->in.line = line;
    argp->in.page = page;
    invokeAndWait(QpScrollView::SetVScrollBarSteps,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollView::setHScrollBarRange(int min,int max){
    QT_METHOD_ARGS_ALLOC(qtMArgsSetScrollBarRange, argp);
    argp->in.min = min;
    argp->in.max = max;
    invokeAndWait(QpScrollView::SetHScrollBarRange,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpScrollView::setVScrollBarRange(int min,int max){
    QT_METHOD_ARGS_ALLOC(qtMArgsSetScrollBarRange, argp);
    argp->in.min = min;
    argp->in.max = max;
    invokeAndWait(QpScrollView::SetVScrollBarRange,argp);
    QT_METHOD_ARGS_FREE(argp);
}

int 
QpScrollView::setHScrollBarValue(int value){
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)value;
    invokeAndWait(QpScrollView::SetHScrollBarValue,argp);
    int rv = (int)argp->out.rvalue ; 
    QT_METHOD_ARGS_FREE(argp);
    return rv;     
}

int 
QpScrollView::setVScrollBarValue(int value){
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)value;
    invokeAndWait(QpScrollView::SetVScrollBarValue,argp);
    int rv = (int)argp->out.rvalue ; 
    QT_METHOD_ARGS_FREE(argp);
    return rv;     
}

void
QpScrollView::getScrollBarWidthAndHeight(int *hHeight, int *vWidth){
    QT_METHOD_ARGS_ALLOC(qtMargsScrollBarWidthAndHeight, argp);
    argp->out.hBarHeight = hHeight;
    argp->out.vBarWidth  = vWidth;
    invokeAndWait(QpScrollView::GetScrollBarWidthAndHeight,argp);
    QT_METHOD_ARGS_FREE(argp);
}

/* 6249842 */
void 
QpScrollView::updateScrollBars(void){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    invokeAndWait(QpScrollView::UpdateScrollBars, argp);
    QT_METHOD_ARGS_FREE(argp);
}

/* 6228838 */
void 
QpScrollView::childResized(int width, int height){
    QT_METHOD_ARGS_ALLOC(qtMargsChildResized, argp);
    argp->in.width  = width;
    argp->in.height = height;
    invokeAndWait(QpScrollView::ChildResized, argp);
    QT_METHOD_ARGS_FREE(argp);
}

void
QpScrollView::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpScrollView, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpScrollView::AddWidget:
        execAddWidget((QpWidget *)(((qtMethodParam *)args)->param));
        break;
                         
    case QpScrollView::Viewport:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execViewport();
        break ;

    case QpScrollView::ConnectHScrollBar: {
        qtMargsConnectScrollBar *a = (qtMargsConnectScrollBar *)args ;
        execConnectHScrollBar(a->in.signal, a->in.receiver, a->in.member);
        }
        break;

    case QpScrollView::ConnectVScrollBar: {
        qtMargsConnectScrollBar *a = (qtMargsConnectScrollBar *)args ;
        execConnectVScrollBar(a->in.signal, a->in.receiver, a->in.member);
	}
        break;

    case QpScrollView::SetVScrollBarMode:{
        int mode = (int)(((qtMethodParam *)args)->param);
        execSetVScrollBarMode((QScrollView::ScrollBarMode)mode);
        }
        break;
                         
    case QpScrollView::SetHScrollBarMode:{
        int mode = (int)(((qtMethodParam *)args)->param);
        execSetHScrollBarMode((QScrollView::ScrollBarMode)mode);
        }
        break;
                         
    case QpScrollView::ScrollBy:
        execScrollBy(((qtQPointArg *)args)->arg.x(),
                     ((qtQPointArg *)args)->arg.y());
        break;
                         
    case QpScrollView::ViewportToContents:{
        qtMargsViewportToContents *a = (qtMargsViewportToContents *)args;
        a->out.rvalue = execViewportToContents((const QPoint &)*a->in.pt);
        }
        break;
                         
    case QpScrollView::SetHScrollBarTracking:
        execSetHScrollBarTracking((bool)(((qtMethodParam *)args)->param));
        break;
                         
    case QpScrollView::SetVScrollBarTracking:
        execSetVScrollBarTracking((bool)(((qtMethodParam *)args)->param));
        break;
                         
    case QpScrollView::IsHScrollBarVisible:
        ((qtMethodReturnValue *)args)->rvalue = (void *)
            execIsHScrollBarVisible();
        break;
                         
    case QpScrollView::IsVScrollBarVisible:
        ((qtMethodReturnValue *)args)->rvalue = (void *)
            execIsVScrollBarVisible();
        break;
                         
    case QpScrollView::SetHScrollBarSteps: {
        qtMArgsSetScrollBarSteps *a = (qtMArgsSetScrollBarSteps *)args;
        execSetHScrollBarSteps(a->in.line, a->in.page);
        }
        break;

    case QpScrollView::SetVScrollBarSteps:{
        qtMArgsSetScrollBarSteps *a = (qtMArgsSetScrollBarSteps *)args;
        execSetVScrollBarSteps(a->in.line, a->in.page);
        }
        break;
                         
    case QpScrollView::SetHScrollBarRange:{
        qtMArgsSetScrollBarRange *a = (qtMArgsSetScrollBarRange*)args;
        execSetHScrollBarRange(a->in.min, a->in.max);
        }
        break;
                         
    case QpScrollView::SetVScrollBarRange:{
        qtMArgsSetScrollBarRange *a = (qtMArgsSetScrollBarRange*)args;
        execSetVScrollBarRange(a->in.min, a->in.max);
        }
        break;

    case QpScrollView::SetHScrollBarValue:
        ((qtMethodArgs *)args)->out.rvalue = (void *)
            execSetHScrollBarValue((int)(((qtMethodArgs *)args)->in.param));
        break;
                         
    case QpScrollView::SetVScrollBarValue:
        ((qtMethodArgs *)args)->out.rvalue = (void *)
            execSetVScrollBarValue((int)(((qtMethodArgs *)args)->in.param));
        break;

    case QpScrollView::GetScrollBarWidthAndHeight: {
        QScrollView sview;
        sview.setHScrollBarMode(QScrollView::AlwaysOn);
        sview.setVScrollBarMode(QScrollView::AlwaysOn);
        qtMargsScrollBarWidthAndHeight *a = 
            (qtMargsScrollBarWidthAndHeight *)args;
        *(a->out.vBarWidth) = sview.verticalScrollBar()->width();
        *(a->out.hBarHeight) = sview.horizontalScrollBar()->height();
        }
        break;

    case QpScrollView::UpdateScrollBars: {
        //6249842                         
        execUpdateScrollBars();
        }
        break;

    case QpScrollView::ChildResized: {
        //6228838     
        qtMargsChildResized* a = (qtMargsChildResized*)args;
        execChildResized(a->in.width, a->in.height);
        }
        break;

    default :
        break;
    }
}
       
void
QpScrollView::execAddWidget(QpWidget *child) {
    QScrollView *scrollView = (QScrollView *)this->getQWidget();
    QWidget *viewport = scrollView->viewport();
    if (viewport != NULL) {
        QWidget *compWidget = AWT_GET_QWIDGET(QpScrollView, child);
        QPoint pt(0, 0);
        compWidget->reparent(viewport, 0, pt, TRUE /* showIt */);
        scrollView->addChild(compWidget);
    }
}

QWidget *
QpScrollView::execViewport() {
    QScrollView *sview = (QScrollView *)this->getQWidget();
    return sview->viewport();
}

void
QpScrollView::execConnectHScrollBar(const char *signal,
                      QObject *receiver,
                      const char *member){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    QObject::connect(sview->horizontalScrollBar(), signal, receiver, member);
}

void
QpScrollView::execConnectVScrollBar(const char *signal,
                      QObject *receiver,
                      const char *member){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    QObject::connect(sview->verticalScrollBar(), signal, receiver, member);
}

void 
QpScrollView::execSetVScrollBarMode( QScrollView::ScrollBarMode mode){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->setVScrollBarMode(mode);
}

void 
QpScrollView::execSetHScrollBarMode( QScrollView::ScrollBarMode mode){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->setHScrollBarMode(mode);
}

void 
QpScrollView::execScrollBy( int dx, int dy ){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->scrollBy(dx,dy);
}

QPoint 
QpScrollView::execViewportToContents( QPoint p ){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    return sview->viewportToContents(p);
}

void 
QpScrollView::execSetHScrollBarTracking(bool track){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->horizontalScrollBar()->setTracking(track);
}

void
QpScrollView:: execSetVScrollBarTracking(bool track){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->verticalScrollBar()->setTracking(track);
}

bool
QpScrollView:: execIsHScrollBarVisible(void){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    switch(sview->hScrollBarMode()) {
    case QScrollView::Auto:
        return (sview->contentsWidth() > sview->width());

    case QScrollView::AlwaysOn:
        return true;

    default:
        return false;
    }
}

bool
QpScrollView:: execIsVScrollBarVisible(void){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    switch(sview->vScrollBarMode()) {
    case QScrollView::Auto:
        return (sview->contentsHeight() > sview->height());

    case QScrollView::AlwaysOn:
        return true;

    default:
        return false;
    }
}

void
QpScrollView:: execSetHScrollBarSteps(int line,int page){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->horizontalScrollBar()->setSteps(line,page);
}

void
QpScrollView:: execSetVScrollBarSteps(int line,int page){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->verticalScrollBar()->setSteps(line,page);
}

void
QpScrollView:: execSetHScrollBarRange(int min,int max){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->horizontalScrollBar()->setRange(min,max);
}

void
QpScrollView:: execSetVScrollBarRange(int min,int max){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->verticalScrollBar()->setRange(min,max);
}

int
QpScrollView:: execSetHScrollBarValue(int value) {
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->horizontalScrollBar()->setValue(value);
    return sview->horizontalScrollBar()->value();
}

int
QpScrollView:: execSetVScrollBarValue(int value) {
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->verticalScrollBar()->setValue(value);
    return sview->verticalScrollBar()->value();
}

//6249842
void
QpScrollView:: execUpdateScrollBars(){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->updateScrollBars();
}

//6228838
void
QpScrollView:: execChildResized(int width, int height){
    QScrollView *sview = (QScrollView *)this->getQWidget();
    sview->resizeContents(width, height);
}
