/*
 * @(#)QpTextEdit.cc	1.6 06/10/25
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
#include "QpTextEdit.h"

typedef struct {
    QString arg;
} qtQStringArg ;

typedef struct {
    int para;
    int index;
} qtParaIndexArg;

typedef struct {
    int para;
    int index;
    int position;
} qtParaIndexPositionArg;

typedef struct {
    struct {
        qtParaIndexArg from;
        qtParaIndexArg to;
        int selNum;
    } in;
}qtMargsSetSelection;

typedef struct {
    struct {
        QString *text;
        int position;
    } in;
}qtMargsInsertAt;

typedef struct {
    struct {
        QString *text;
        int startPos;
        int endPos;
    } in;
}qtMargsReplaceAt;

/*
 * QpTextEdit Methods
 */
QpTextEdit::QpTextEdit(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createTextEdit(parent,
                                                                  name,
                                                                  flags);
    this->setQWidget(widget);
}

void 
QpTextEdit::setReadOnly( bool b ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)b;
    invokeAndWait(QpTextEdit::SetReadOnly,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QString 
QpTextEdit::text() {
    QT_METHOD_ARGS_ALLOC(qtQStringArg, argp);
    invokeAndWait(QpTextEdit::Text,argp);
    QString rv = argp->arg;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpTextEdit::setText( const QString &txt ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&txt;
    invokeAndWait(QpTextEdit::SetText,argp);
    QT_METHOD_ARGS_FREE(argp);
}

bool 
QpTextEdit::hasSelectedText(){
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpTextEdit::HasSelectedText,argp);
    bool rv = (bool)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpTextEdit::getCursorPosition( int *parag, int *index ) {
    QT_METHOD_ARGS_ALLOC(qtParaIndexArg, argp);
    invokeAndWait(QpTextEdit::GetCursorPosition,argp);
    *parag = argp->para;
    *index = argp->index;
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpTextEdit::setSelection( int parag_from, int index_from,
                          int parag_to, int index_to, int selNum ){
    QT_METHOD_ARGS_ALLOC(qtMargsSetSelection, argp);
    argp->in.from.para  = parag_from;
    argp->in.from.index = index_from;
    argp->in.to.para    = parag_to;
    argp->in.to.index   = index_to;
    argp->in.selNum     = selNum;
    invokeAndWait(QpTextEdit::SetSelection,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpTextEdit::setCursorPosition( int parag, int index ){
    QT_METHOD_ARGS_ALLOC(qtParaIndexArg, argp);
    argp->para  = parag;
    argp->index = index;
    invokeAndWait(QpTextEdit::SetCursorPosition,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpTextEdit::setWrapPolicy( QTextEdit::WrapPolicy policy ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)policy);
    invokeAndWait(QpTextEdit::SetWrapPolicy,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpTextEdit::setWordWrap( QTextEdit::WordWrap mode ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)mode);
    invokeAndWait(QpTextEdit::SetWordWrap,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void  
QpTextEdit::setVScrollBarMode( QTextEdit::ScrollBarMode mode){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)mode);
    invokeAndWait(QpTextEdit::SetVScrollBarMode,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void  
QpTextEdit::setHScrollBarMode( QTextEdit::ScrollBarMode mode){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)mode);
    invokeAndWait(QpTextEdit::SetHScrollBarMode,argp);
    QT_METHOD_ARGS_FREE(argp);
}

int 
QpTextEdit::length() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpTextEdit::Length,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpTextEdit::positionToParaIndex(int position, int *index) {
    QT_METHOD_ARGS_ALLOC(qtParaIndexPositionArg, argp);
    argp->position = position;
    invokeAndWait(QpTextEdit::PositionToParaIndex,argp);
    int rv = argp->para;
    *index = argp->index;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpTextEdit::paraIndexToPosition(int para, int index)  {
    QT_METHOD_ARGS_ALLOC(qtParaIndexPositionArg, argp);
    argp->para  = para;
    argp->index = index;
    invokeAndWait(QpTextEdit::ParaIndexToPosition,argp);
    int rv = argp->position;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpTextEdit::getCursorPosition()  {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpTextEdit::GetCursorPosition_void,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpTextEdit::getSelectionStartPosition() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpTextEdit::GetSelectionStartPosition,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpTextEdit::getSelectionEndPosition() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpTextEdit::GetSelectionEndPosition,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}
void 
QpTextEdit::insertAt(int position, QString text)  {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertAt, argp);
    argp->in.text     = &text;
    argp->in.position = position;
    invokeAndWait(QpTextEdit::InsertAt,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpTextEdit::replaceAt(QString text, int startPos, int endPos) {
    QT_METHOD_ARGS_ALLOC(qtMargsReplaceAt, argp);
    argp->in.text     = &text;
    argp->in.startPos = startPos;
    argp->in.endPos   = endPos;
    invokeAndWait(QpTextEdit::ReplaceAt,argp);
    QT_METHOD_ARGS_FREE(argp);
}

int  
QpTextEdit::extraWidth() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpTextEdit::ExtraWidth,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpTextEdit::extraHeight() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpTextEdit::ExtraHeight,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

QWidget *
QpTextEdit::viewport() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpTextEdit::Viewport,argp);
    QWidget *rv = (QWidget *)argp->rvalue ;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void
QpTextEdit::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpTextEdit, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpTextEdit::SetReadOnly:
        execSetReadOnly((bool)(((qtMethodParam *)args)->param));
        break;

    case QpTextEdit::Text:
        ((qtQStringArg *)args)->arg = execText();
        break;

    case QpTextEdit::SetText:{
        QString *txt = (QString *)((qtMethodParam *)args)->param;
        execSetText((const QString &)*txt);
        }
        break;

    case QpTextEdit::HasSelectedText:
        ((qtMethodReturnValue *)args)->rvalue = (void *)
            execHasSelectedText();
        break;

    case QpTextEdit::GetCursorPosition:{
        qtParaIndexArg *a = (qtParaIndexArg *)args;
        execGetCursorPosition(&a->para, &a->index);
        }
        break ;
       
    case QpTextEdit::SetSelection: {
        qtMargsSetSelection *a = (qtMargsSetSelection *)args;
        execSetSelection(a->in.from.para,a->in.from.index,
                         a->in.to.para,a->in.to.index,
                         a->in.selNum);
        }
        break;

    case QpTextEdit::SetCursorPosition: {
        qtParaIndexArg *a = (qtParaIndexArg *)args;
        execSetCursorPosition(a->para, a->index);
        }
        break;

    case QpTextEdit::SetWrapPolicy:{
        int policy = (int)(((qtMethodParam *)args)->param);
        execSetWrapPolicy((QTextEdit::WrapPolicy)policy);
        }
        break;

    case QpTextEdit::SetWordWrap:{
        int mode = (int)(((qtMethodParam *)args)->param);
        execSetWordWrap((QTextEdit::WordWrap)mode);
        }
        break ;

    case QpTextEdit::SetVScrollBarMode:{
        int mode = (int)(((qtMethodParam *)args)->param);
        execSetVScrollBarMode((QTextEdit::ScrollBarMode)mode);
        }
        break;
                         
    case QpTextEdit::SetHScrollBarMode:{
        int mode = (int)(((qtMethodParam *)args)->param);
        execSetHScrollBarMode((QTextEdit::ScrollBarMode)mode);
        }
        break;

    case QpTextEdit::Length:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execLength();
        break;

    case QpTextEdit::PositionToParaIndex:{
        qtParaIndexPositionArg *a = (qtParaIndexPositionArg *)args;
        a->para = execPositionToParaIndex(a->position, &a->index);
        }
        break;
    case QpTextEdit::ParaIndexToPosition:{
        qtParaIndexPositionArg *a = (qtParaIndexPositionArg *)args;
        a->position = execParaIndexToPosition(a->para, a->index);
        }
        break;
    case QpTextEdit::GetCursorPosition_void:
        ((qtMethodReturnValue *)args)->rvalue = (void *)
            execGetCursorPosition();
        break;
    case QpTextEdit::GetSelectionStartPosition:
        ((qtMethodReturnValue *)args)->rvalue = (void *)
            execGetSelectionStartPosition();
        break;
    case QpTextEdit::GetSelectionEndPosition:
        ((qtMethodReturnValue *)args)->rvalue = (void *)
            execGetSelectionEndPosition();
        break;
    case QpTextEdit::InsertAt:{
        qtMargsInsertAt *a = (qtMargsInsertAt *)args;
        execInsertAt(a->in.position, *a->in.text);
        }
        break;
    case QpTextEdit::ReplaceAt:{
        qtMargsReplaceAt *a = (qtMargsReplaceAt *)args;
        execReplaceAt(*a->in.text, a->in.startPos, a->in.endPos);
        }
        break;
    case QpTextEdit::ExtraWidth:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execExtraWidth();
        break;
    case QpTextEdit::ExtraHeight:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execExtraHeight();
        break;
    case QpTextEdit::Viewport:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execViewport();
        break ;
    default :
        break;
    }
}
void    
QpTextEdit::execSetReadOnly( bool b ) {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->setReadOnly(b);
}

QString 
QpTextEdit::execText()  {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    return tedit->text();
}

void    
QpTextEdit::execSetText( const QString &txt ) {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->setText(txt);
}

bool    
QpTextEdit::execHasSelectedText()  {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    return tedit->hasSelectedText();
}

void    
QpTextEdit::execGetCursorPosition( int *parag, int *index )  {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->getCursorPosition(parag, index);
}

void    
QpTextEdit::execSetSelection( int parag_from, int index_from, 
                              int parag_to, int index_to, int selNum) {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->setSelection(parag_from,index_from,parag_to,index_to,selNum);
}

void   
QpTextEdit::execSetCursorPosition( int parag, int index ) {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->setCursorPosition(parag, index);
}

void  
QpTextEdit::execSetWrapPolicy( QTextEdit::WrapPolicy policy ) {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->setWrapPolicy(policy);
}

void 
QpTextEdit::execSetWordWrap( QTextEdit::WordWrap mode ) {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->setWordWrap(mode);
}

void 
QpTextEdit::execSetVScrollBarMode( QTextEdit::ScrollBarMode mode){
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->setVScrollBarMode(mode);
}

void 
QpTextEdit::execSetHScrollBarMode( QTextEdit::ScrollBarMode mode){
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    tedit->setHScrollBarMode(mode);
}

int 
QpTextEdit::execLength()  {
    QTextEdit *tedit = (QTextEdit *)this->getQWidget();
    return tedit->length();
}

int
QpTextEdit::execPositionToParaIndex(int position, int *index)  {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    return tedit->positionToParaIndex(position, index);
}

int
QpTextEdit::execParaIndexToPosition(int para, int index)  {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    return tedit->paraIndexToPosition(para, index);
}

int
QpTextEdit::execGetCursorPosition()  {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    return tedit->getCursorPosition();
}

int
QpTextEdit::execGetSelectionStartPosition() {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    return tedit->getSelectionStartPosition();
}

int 
QpTextEdit::execGetSelectionEndPosition() {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    return tedit->getSelectionEndPosition();
}

void 
QpTextEdit::execInsertAt(int position, QString text)  {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    tedit->insertAt(position, text);
}

void 
QpTextEdit::execReplaceAt(QString text, int startPos, int endPos) {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    tedit->replaceAt(text, startPos, endPos);
}

int 
QpTextEdit::execExtraWidth() {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    return tedit->extraWidth();
}

int 
QpTextEdit::execExtraHeight() {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    return tedit->extraHeight();
}

QWidget *
QpTextEdit::execViewport() {
    QxTextEdit *tedit = (QxTextEdit *)this->getQWidget();
    return tedit->viewport();
}

