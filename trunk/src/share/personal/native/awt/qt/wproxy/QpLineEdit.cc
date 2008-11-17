/*
 * @(#)QpLineEdit.cc	1.7 06/10/25
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

#include "QpWidgetFactory.h"
#include "QpLineEdit.h"

typedef struct {
    QString arg;
} qtQStringArg ;

typedef struct {
    struct {
        int start;
        int length;
    } in;
}qtMargsSetSelection;

/*
 * QpLineEdit Methods
 */
QpLineEdit::QpLineEdit(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createLineEdit(parent,
                                                                  name,
                                                                  flags);
    this->setQWidget(widget);
}

void 
QpLineEdit::setReadOnly( bool b ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)b;
    invokeAndWait(QpLineEdit::SetReadOnly,argp);
    QT_METHOD_ARGS_FREE(argp);
}

int  
QpLineEdit::cursorPosition()  {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpLineEdit::CursorPosition,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpLineEdit::setCursorPosition( int position ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)position;
    invokeAndWait(QpLineEdit::SetCursorPosition,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpLineEdit::setSelection( int start, int length) {
    QT_METHOD_ARGS_ALLOC(qtMargsSetSelection, argp);
    argp->in.start  = start;
    argp->in.length = length;
    invokeAndWait(QpLineEdit::SetSelection,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpLineEdit::setEchoMode( QLineEdit::EchoMode mode ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)mode);
    invokeAndWait(QpLineEdit::SetEchoMode,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpLineEdit::setText( const QString &txt ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&txt;
    invokeAndWait(QpLineEdit::SetText,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QString 
QpLineEdit::getString() {
    QT_METHOD_ARGS_ALLOC(qtQStringArg, argp);
    invokeAndWait(QpLineEdit::GetString,argp);
    QString rv = argp->arg;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpLineEdit::setString(QString str) {
    QT_METHOD_ARGS_ALLOC(qtQStringArg, argp);
    argp->arg = str;
    invokeAndWait(QpLineEdit::SetString,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpLineEdit::setDirty( bool b ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)b;
    invokeAndWait(QpLineEdit::SetDirty,argp);
    QT_METHOD_ARGS_FREE(argp);
}

bool 
QpLineEdit::needsRefresh() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpLineEdit::NeedsRefresh,argp);
    bool rv = (bool)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpLineEdit::selectionStart()  {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpLineEdit::SelectionStart,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

#if (QT_VERSION >= 0x030000)
QString 
QpLineEdit::selectedText() {
    QT_METHOD_ARGS_ALLOC(qtQStringArg, argp);
    invokeAndWait(QpLineEdit::SelectedText,argp);
    QString rv = argp->arg;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

bool 
QpLineEdit::hasSelectedText(){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpLineEdit::HasSelectedText,argp);
    bool rv = (bool)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}
#else
int  
QpLineEdit::selectionEnd()  {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpLineEdit::SelectionEnd,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpLineEdit::setSelectionPt(int start, int end) {
    QT_METHOD_ARGS_ALLOC(qtMargsSetSelection, argp);
    argp->in.start  = start;
    argp->in.length = end;
    invokeAndWait(QpLineEdit::SetSelectionPt,argp);   //6231035
    QT_METHOD_ARGS_FREE(argp);
}

#endif /* QT_VERSION */

void
QpLineEdit::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpLineEdit, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpLineEdit::SetReadOnly:
        execSetReadOnly((bool)(((qtMethodParam *)args)->param));
        break;

    case QpLineEdit::CursorPosition:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execCursorPosition();
        break;

    case QpLineEdit::SetCursorPosition:
        execSetCursorPosition((int)((qtMethodParam *)args)->param);
        break;

    case QpLineEdit::SetSelection:{
        qtMargsSetSelection *a = (qtMargsSetSelection *)args;
        execSetSelection(a->in.start, a->in.length);
        }
        break;
        
    case QpLineEdit::SetEchoMode:{
        int mode = (int)(((qtMethodParam *)args)->param);
        execSetEchoMode((QLineEdit::EchoMode)mode);
        }
        break;

    case QpLineEdit::SetText:{
        QString *txt = (QString *)((qtMethodParam *)args)->param;
        execSetText((const QString &)*txt);
        }
        break;

    case QpLineEdit::SelectionStart:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execSelectionStart();
        break;

    case QpLineEdit::GetString:
        ((qtQStringArg*)args)->arg = execGetString();
        break;

    case QpLineEdit::SetString:
        execSetString(((qtQStringArg*)args)->arg);
        break;

    case QpLineEdit::SetDirty:
        execSetDirty((bool)(((qtMethodParam *)args)->param));
        break;

    case QpLineEdit::NeedsRefresh:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execNeedsRefresh();
        break;
#if (QT_VERSION >= 0x030000)
    case QpLineEdit::SelectedText:
        ((qtQStringArg*)args)->arg = execSelectedText();
        break;
        
    case QpLineEdit::HasSelectedText:
        ((qtMethodReturnValue *)args)->rvalue = (void *)
            execHasSelectedText();
        break;
#else
    case QpLineEdit::SelectionEnd:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execSelectionEnd();
        break;

    case QpLineEdit::SetSelectionPt:{
        qtMargsSetSelection *a = (qtMargsSetSelection *)args;
        execSetSelectionPt(a->in.start, a->in.length);
        }
        break;
        
#endif /* QT_VERSION */


    default :
        break;
    }
}

void    
QpLineEdit::execSetReadOnly( bool b ) {
    QLineEdit *ledit = (QLineEdit *)this->getQWidget();
    ledit->setReadOnly(b);
}

int 
QpLineEdit::execCursorPosition(){
    QLineEdit *ledit = (QLineEdit *)this->getQWidget();
    return ledit->cursorPosition();
}

void 
QpLineEdit::execSetCursorPosition( int pos){
    QLineEdit *ledit = (QLineEdit *)this->getQWidget();
    ledit->setCursorPosition(pos);
}

void 
QpLineEdit::execSetSelection( int start, int length ){
    QLineEdit *ledit = (QLineEdit *)this->getQWidget();
    ledit->setSelection(start, length);
}

void 
QpLineEdit::execSetEchoMode( QLineEdit::EchoMode mode){
    QLineEdit *ledit = (QLineEdit *)this->getQWidget();
    ledit->setEchoMode(mode);
}

void    
QpLineEdit::execSetText( const QString &txt ) {
    QLineEdit *ledit = (QLineEdit *)this->getQWidget();
    ledit->setText(txt);
}

QString 
QpLineEdit::execGetString(){
    QxLineEdit *ledit = (QxLineEdit *)this->getQWidget();
    return ledit->getString();
}

void 
QpLineEdit::execSetString( QString text ){
    QxLineEdit *ledit = (QxLineEdit *)this->getQWidget();
    ledit->setString(text);
}

void 
QpLineEdit::execSetDirty( bool tf ){
    QxLineEdit *ledit = (QxLineEdit *)this->getQWidget();
    ledit->setDirty(tf);
}

bool 
QpLineEdit::execNeedsRefresh(){
    QxLineEdit *ledit = (QxLineEdit *)this->getQWidget();
    return ledit->needsRefresh();
}

int
QpLineEdit::execSelectionStart( void) {
    QxLineEdit *ledit = (QxLineEdit *)this->getQWidget();
    return ledit->selectionStart();
}

#if (QT_VERSION >= 0x030000)
QString 
QpLineEdit::execSelectedText(){
    QLineEdit *ledit = (QLineEdit *)this->getQWidget();
    return ledit->selectedText();
}

bool    
QpLineEdit::execHasSelectedText()  {
    QLineEdit *ledit = (QLineEdit *)this->getQWidget();
    return ledit->hasSelectedText();
}
#else
int 
QpLineEdit::execSelectionEnd(){
    QxLineEdit *ledit = (QxLineEdit *)this->getQWidget();
    return ledit->selectionEnd();
}

void 
QpLineEdit::execSetSelectionPt(int start, int end){
    QxLineEdit *ledit = (QxLineEdit *)this->getQWidget();
    ledit->setSelectionPt(start,end);
}
#endif /* QT_VERSION */
