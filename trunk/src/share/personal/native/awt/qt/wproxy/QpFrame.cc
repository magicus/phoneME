/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

#include "QpWidgetFactory.h"
#include "QpFrame.h"

typedef struct {
    QRect arg ;
} qtQRectArg ;

typedef struct {
    QPoint arg ;
} qtQPointArg ;

/*
 * QpFrame Methods
 */
QpFrame::QpFrame(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createFrame(parent,
                                                               name,
                                                               flags);
    this->setQWidget(widget);
    warningStringLabel = NULL;   //6233632
}

/*
  6233632
*/
void
QpFrame::createWarningLabel(QString warningString) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&warningString;
    invokeAndWait(QpFrame::CreateWarningLabel,argp);
    QT_METHOD_ARGS_FREE(argp);
}

/*
  6233632
*/
void
QpFrame::resizeWarningLabel(void) {
    //if (statusBar != NULL) {
    if (warningStringLabel != NULL) {
        QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
        invokeAndWait(QpFrame::ResizeWarningLabel,argp);
        QT_METHOD_ARGS_FREE(argp);
    }
}

/*
  6233632
*/
int
QpFrame::warningLabelHeight(void) {
    //if (statusBar == NULL) {
    if (warningStringLabel == NULL) {
        return 0;
    } else {
        QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
        invokeAndWait(QpFrame::WarningLabelHeight, argp);
        int rv = (int)argp->rvalue;
        QT_METHOD_ARGS_FREE(argp);
        return rv;
    }
}

void
QpFrame::setLineWidth(int width) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)width ;
    invokeAndWait(QpFrame::SetLineWidth,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpFrame::setFrameStyle(int style) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)style ;
    invokeAndWait(QpFrame::SetFrameStyle,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QRect 
QpFrame::frameRect(void){
    QT_METHOD_ARGS_ALLOC(qtQRectArg, argp);
    invokeAndWait(QpFrame::FrameRect,argp);
    QRect rv = argp->arg ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void  
QpFrame::setFrameRect(QRect rect){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&rect ;
    invokeAndWait(QpFrame::SetFrameRect,argp);
    QT_METHOD_ARGS_FREE(argp);
}

#ifdef QWS
QPoint 
QpFrame::getOriginWithDecoration(){
    QT_METHOD_ARGS_ALLOC(qtQPointArg, argp);
    invokeAndWait(QpFrame::GetOriginWithDecoration,argp);
    QPoint rv = argp->arg ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}
#endif /* QWS */

QRect
QpFrame::frameGeometry() {
    QT_METHOD_ARGS_ALLOC(qtQRectArg, argp);
    invokeAndWait(QpFrame::FrameGeometry,argp);
    QRect rv = argp->arg ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int
QpFrame::frameStyle() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpFrame::FrameStyle, argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpFrame::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpFrame, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpFrame::SetLineWidth:
        execSetLineWidth((int)(((qtMethodParam *)args)->param));
        break ;
    case QpFrame::SetFrameStyle:
        execSetFrameStyle((int)(((qtMethodParam *)args)->param));
        break ;
    case QpFrame::FrameRect:
        ((qtQRectArg *)args)->arg = execFrameRect();
        break;
    case QpFrame::SetFrameRect:{
        QRect *rect = (QRect *)((qtMethodParam *)args)->param;
        execSetFrameRect(*rect);
        }
        break;
    case QpFrame::FrameGeometry:
        ((qtQRectArg *)args)->arg = execFrameGeometry();
        break;
    case QpFrame::FrameStyle:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execFrameStyle();
        break;
    case QpFrame::CreateWarningLabel:{
        QString *warningString = (QString *)((qtMethodParam *)args)->param;
        execCreateWarningLabel(*warningString);
        }
        break;
    case QpFrame::ResizeWarningLabel:   //6233632
        execResizeWarningLabel();
        break;
    case QpFrame::WarningLabelHeight:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execWarningLabelHeight();
        break;
#ifdef QWS
    case QpFrame::GetOriginWithDecoration:
        ((qtQPointArg *)args)->arg = execGetOriginWithDecoration();
        break;
#endif /* QWS */
    default :
        break;
    }
}

/*
  6233632
*/        
void 
QpFrame::execCreateWarningLabel(QString warningString) {
    QFrame *frame = (QFrame *)this->getQWidget();

    // Create the warning label
    warningStringLabel = new QLabel(warningString, frame);
    warningStringLabel->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum));
    warningStringLabel->setAlignment(Qt::AlignHCenter);

    // Set the color to slightly darker then the frame.
    QColor color = frame->backgroundColor();
    QColor darkColor = color.dark(125);
    warningStringLabel->setBackgroundColor(darkColor);
    warningStringLabel->setGeometry(0, 
                                    frame->height() - warningStringLabel->height(), 
                                    frame->width(), 
                                    warningStringLabel->height());
 }

/*
  6233632
*/
void
QpFrame::execResizeWarningLabel(void) {
    QFrame *frame = (QFrame *)this->getQWidget();
    warningStringLabel->setGeometry(0, 
                                    frame->height() - warningStringLabel->height(), 
                                    frame->width(), 
                                    warningStringLabel->height());
}

int
QpFrame::execWarningLabelHeight(void) {
    return warningStringLabel->height();
}

void
QpFrame::execSetLineWidth(int width) {
    QFrame *frame = (QFrame *)this->getQWidget();
    frame->setLineWidth(width);
}

void 
QpFrame::execSetFrameStyle(int style) {
    QFrame *frame = (QFrame *)this->getQWidget();
    frame->setFrameStyle(style);
}

QRect 
QpFrame::execFrameRect(void){
    QFrame *frame = (QFrame *)this->getQWidget();
    return frame->frameRect();
}

void 
QpFrame::execSetFrameRect(QRect rect){
    QFrame *frame = (QFrame *)this->getQWidget();
    frame->setFrameRect(rect);
}

QRect
QpFrame::execFrameGeometry(void) {
    QFrame *frame = (QFrame *)this->getQWidget();
    return frame->frameGeometry();
}

int
QpFrame::execFrameStyle(void) {
    QFrame *frame = (QFrame *)this->getQWidget();
    return frame->frameStyle();
}

#ifdef QWS
#include <qpe/qpeapplication.h>
QPoint
QpFrame::execGetOriginWithDecoration() {
    QFrame *frame = (QFrame *)this->getQWidget();
    QWSDecoration &dcr = QApplication::qwsDecoration();
	QRegion dcrReg = dcr.region(frame, frame->geometry());
	QRect dcrRegRct = dcrReg.boundingRect();
	
    QPoint rv(frame->geometry().x() - dcrRegRct.x(),
              frame->geometry().y() - dcrRegRct.y());
    return rv;
}

#endif /* QWS */
