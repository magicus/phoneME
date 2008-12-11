/*
 * @(#)QpMultiLineEdit.cc	1.6 06/10/25
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
#include "QpMultiLineEdit.h"
#include "QxMultiLineEdit.h"

typedef struct {
    QString arg;
} qtQStringArg ;

typedef struct {
    int line;
    int col;
    bool mark;
} qtLineColArg;

typedef struct {
    int line;
    int col;
    int position;
} qtLineColPositionArg;

typedef struct {
    struct {
        qtLineColArg from;
        qtLineColArg to;
    } in;
}qtMargsSetSelection;

typedef struct {
    struct {
        qtLineColArg from;
        qtLineColArg to;
        bool rvalue;
    } out;
}qtMargsGetSelectedRegion;

typedef struct {
    struct {
        QString *text;
        qtLineColArg position;
    } in;
}qtMargsInsertAt;

typedef struct {
    struct {
        int position;
    }in;
    struct {
        int row;
        int col;
        bool rvalue;
    }out;
} qtMargsGetRowCol;
/*
 * QpMultiLineEdit Methods
 */
QpMultiLineEdit::QpMultiLineEdit(QpWidget *parent, char *name, int flags) {
    QWidget *widget = QpWidgetFactory::instance()->createMultiLineEdit(parent,
                                                                  name,
                                                                  flags);
    this->setQWidget(widget);
}

void 
QpMultiLineEdit::setReadOnly( bool b ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)b;
    invokeAndWait(QpMultiLineEdit::SetReadOnly,argp);
    QT_METHOD_ARGS_FREE(argp);
}

QString 
QpMultiLineEdit::text() {
    QT_METHOD_ARGS_ALLOC(qtQStringArg, argp);
    invokeAndWait(QpMultiLineEdit::Text,argp);
    QString rv = argp->arg;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpMultiLineEdit::setText( const QString &txt ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&txt;
    invokeAndWait(QpMultiLineEdit::SetText,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMultiLineEdit::getCursorPosition( int *line, int *col ) {
    QT_METHOD_ARGS_ALLOC(qtLineColArg, argp);
    invokeAndWait(QpMultiLineEdit::GetCursorPosition,argp);
    *line = argp->line;
    *col = argp->col;
    QT_METHOD_ARGS_FREE(argp);
}

int 
QpMultiLineEdit::length() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpMultiLineEdit::Length,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpMultiLineEdit::setSelection( int line_from, int col_from,
                               int line_to, int col_to){
    QT_METHOD_ARGS_ALLOC(qtMargsSetSelection, argp);
    argp->in.from.line  = line_from;
    argp->in.from.col   = col_from;
    argp->in.to.line    = line_to;
    argp->in.to.col     = col_to;
    invokeAndWait(QpMultiLineEdit::SetSelection,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMultiLineEdit::setCursorPosition( int line, int col, bool mark ){
    QT_METHOD_ARGS_ALLOC(qtLineColArg, argp);
    argp->line  = line;
    argp->col   = col;
    argp->mark  = mark;
    invokeAndWait(QpMultiLineEdit::SetCursorPosition,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMultiLineEdit::setWrapPolicy( QMultiLineEdit::WrapPolicy policy ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)policy);
    invokeAndWait(QpMultiLineEdit::SetWrapPolicy,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMultiLineEdit::setWordWrap( QMultiLineEdit::WordWrap mode ){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)((int)mode);
    invokeAndWait(QpMultiLineEdit::SetWordWrap,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void  
QpMultiLineEdit::setAutoUpdate(bool enable) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)enable;
    invokeAndWait(QpMultiLineEdit::SetAutoUpdate,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMultiLineEdit::insertAt(const QString &text, 
                          int line, int col, bool mark)  {
    QT_METHOD_ARGS_ALLOC(qtMargsInsertAt, argp);
    argp->in.text          = (QString *)&text;
    argp->in.position.line = line;
    argp->in.position.col  = col;
    argp->in.position.mark = mark;
    invokeAndWait(QpMultiLineEdit::InsertAt,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMultiLineEdit::insert(const QString &text) {
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)&text;
    invokeAndWait(QpMultiLineEdit::Insert,argp);
    QT_METHOD_ARGS_FREE(argp);
}

bool 
QpMultiLineEdit::rowIsVisible(int row){
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)row;
    invokeAndWait(QpMultiLineEdit::RowIsVisible,argp);
    int rv = (int)argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

bool 
QpMultiLineEdit::getSelectedRegion(int* line1, int* col1, 
                                   int* line2, int* col2){
    QT_METHOD_ARGS_ALLOC(qtMargsGetSelectedRegion, argp);
    invokeAndWait(QpMultiLineEdit::GetSelectedRegion,argp);
    *line1 = argp->out.from.line;
    *col1  = argp->out.from.col;
    *line2 = argp->out.to.line;
    *col2  = argp->out.to.col;
    bool rv = argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpMultiLineEdit::getRowCol(int position, int *row, int *col){
    QT_METHOD_ARGS_ALLOC(qtMargsGetRowCol, argp);
    argp->in.position = position;
    invokeAndWait(QpMultiLineEdit::GetRowCol,argp);
    *row = argp->out.row;
    *col = argp->out.col;
    QT_METHOD_ARGS_FREE(argp);
}

int  
QpMultiLineEdit::extraWidth() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpMultiLineEdit::ExtraWidth,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpMultiLineEdit::extraHeight() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpMultiLineEdit::ExtraHeight,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpMultiLineEdit::charsInLine() {
    QT_METHOD_ARGS_ALLOC(qtMethodReturnValue, argp);
    invokeAndWait(QpMultiLineEdit::CharsInLine,argp);
    int rv = (int)argp->rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpMultiLineEdit::charsInLine(int line) {
    QT_METHOD_ARGS_ALLOC(qtMethodArgs, argp);
    argp->in.param = (void *)line;
    invokeAndWait(QpMultiLineEdit::CharsInLine_int,argp);
    int rv = (int)argp->out.rvalue;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

int  
QpMultiLineEdit::getPosition(int line, int col)  {
    QT_METHOD_ARGS_ALLOC(qtLineColPositionArg, argp);
    argp->line  = line;
    argp->col = col;
    invokeAndWait(QpMultiLineEdit::GetPosition,argp);
    int rv = argp->position;
    QT_METHOD_ARGS_FREE(argp);
    return rv;
}

void 
QpMultiLineEdit::setTableOptions(int flag){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)flag;
    invokeAndWait(QpMultiLineEdit::SetTableOptions,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMultiLineEdit::clearTableOptions(int flag){
    QT_METHOD_ARGS_ALLOC(qtMethodParam, argp);
    argp->param = (void *)flag;
    invokeAndWait(QpMultiLineEdit::ClearTableOptions,argp);
    QT_METHOD_ARGS_FREE(argp);
}

void 
QpMultiLineEdit::deleteSelectedText(){
    invokeAndWait(QpMultiLineEdit::DeleteSelectedText,NULL);
}

void
QpMultiLineEdit::execute(int methodId, void *args) {
    QP_METHOD_ID_DECLARE(QpMultiLineEdit, mid);
    QP_INVOKE_SUPER_ON_INVALID_MID(mid, QpWidget);

    switch ( mid ) {
    case QpMultiLineEdit::SetReadOnly:
        execSetReadOnly((bool)(((qtMethodParam *)args)->param));
        break;

    case QpMultiLineEdit::Text:
        ((qtQStringArg *)args)->arg = execText();
        break;

    case QpMultiLineEdit::SetText:{
        QString *txt = (QString *)((qtMethodParam *)args)->param;
        execSetText((const QString &)*txt);
        }
        break;

    case QpMultiLineEdit::GetCursorPosition:{
        qtLineColArg *a = (qtLineColArg *)args;
        execGetCursorPosition(&a->line, &a->col);
        }
        break ;
       
    case QpMultiLineEdit::Length:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execLength();
        break;

    case QpMultiLineEdit::SetSelection: {
        qtMargsSetSelection *a = (qtMargsSetSelection *)args;
        execSetSelection(a->in.from.line,a->in.from.col,
                         a->in.to.line,a->in.to.col);
        }
        break;

    case QpMultiLineEdit::SetCursorPosition: {
        qtLineColArg *a = (qtLineColArg *)args;
        execSetCursorPosition(a->line, a->col, a->mark);
        }
        break;

    case QpMultiLineEdit::SetWrapPolicy:{
        int policy = (int)(((qtMethodParam *)args)->param);
        execSetWrapPolicy((QMultiLineEdit::WrapPolicy)policy);
        }
        break;

    case QpMultiLineEdit::SetWordWrap:{
        int mode = (int)(((qtMethodParam *)args)->param);
        execSetWordWrap((QMultiLineEdit::WordWrap)mode);
        }
        break ;

    case QpMultiLineEdit::SetAutoUpdate:
        execSetAutoUpdate((bool)((qtMethodParam *)args)->param);
        break;

    case QpMultiLineEdit::InsertAt:{
        qtMargsInsertAt *a = (qtMargsInsertAt *)args;
        execInsertAt(*a->in.text, a->in.position.line,
                     a->in.position.col,a->in.position.mark);
        }
        break;

    case QpMultiLineEdit::Insert:{
        QString *txt = (QString *)((qtMethodParam *)args)->param;
        execInsert((const QString &)*txt);
        }
        break;

    case QpMultiLineEdit::RowIsVisible:{
        qtMethodArgs *a = (qtMethodArgs *)args;
        a->out.rvalue = (void *)execRowIsVisible((int)a->in.param);
        }
        break;

    case QpMultiLineEdit::GetSelectedRegion:{
        qtMargsGetSelectedRegion *a = (qtMargsGetSelectedRegion *)args;
        a->out.rvalue = execGetSelectedRegion(&a->out.from.line,
                                              &a->out.from.col,
                                              &a->out.to.line,
                                              &a->out.to.col);
        }
        break;

    case QpMultiLineEdit::GetRowCol:{
        qtMargsGetRowCol *a = (qtMargsGetRowCol *)args;
        execGetRowCol(a->in.position, &a->out.row, &a->out.col);
        }
        break;

    case QpMultiLineEdit::ExtraWidth:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execExtraWidth();
        break;

    case QpMultiLineEdit::ExtraHeight:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execExtraHeight();
        break;

    case QpMultiLineEdit::CharsInLine:
        ((qtMethodReturnValue *)args)->rvalue = (void *)execCharsInLine() ;
        break;

    case QpMultiLineEdit::CharsInLine_int: {
        qtMethodArgs *a = (qtMethodArgs *)args;
        a->out.rvalue = (void *)execCharsInLine((int)a->in.param);
        }
        break;

    case QpMultiLineEdit::GetPosition:{
        qtLineColPositionArg *a = (qtLineColPositionArg *)args;
        a->position = execGetPosition(a->line, a->col);
        }
        break;

    case QpMultiLineEdit::SetTableOptions:
        execSetTableOptions((int)((qtMethodParam *)args)->param);
        break;

    case QpMultiLineEdit::ClearTableOptions:
        execClearTableOptions((int)((qtMethodParam *)args)->param);
        break;

    case QpMultiLineEdit::DeleteSelectedText:
        execDeleteSelectedText();
        break;

    default :
        break;
    }
}
void    
QpMultiLineEdit::execSetReadOnly( bool b ) {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->setReadOnly(b);
}

QString 
QpMultiLineEdit::execText()  {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->text();
}

void    
QpMultiLineEdit::execSetText( const QString &txt ) {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->setText(txt);
}

void    
QpMultiLineEdit::execGetCursorPosition( int *line, int *col )  {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->getCursorPosition(line, col);
}

int 
QpMultiLineEdit::execLength()  {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->length();
}

void    
QpMultiLineEdit::execSetSelection( int line_from, int col_from, 
                                   int line_to, int col_to) {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->setSelection(line_from,col_from,line_to,col_to);
}

void   
QpMultiLineEdit::execSetCursorPosition( int line, int col , bool mark) {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->setCursorPosition(line, col, mark);
}

void  
QpMultiLineEdit::execSetWrapPolicy( QMultiLineEdit::WrapPolicy policy ) {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->setWrapPolicy(policy);
}

void 
QpMultiLineEdit::execSetWordWrap( QMultiLineEdit::WordWrap mode ) {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->setWordWrap(mode);
}

void 
QpMultiLineEdit::execSetAutoUpdate( bool enable) {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->setAutoUpdate(enable);
}

void 
QpMultiLineEdit::execInsertAt(const QString &text,int line,int col,bool mark){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->insertAt(text,line, col, mark);
}

void 
QpMultiLineEdit::execInsert(const QString &text){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->insert(text);
}

bool 
QpMultiLineEdit::execRowIsVisible(int row){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->rowIsVisible(row);
}

bool 
QpMultiLineEdit::execGetSelectedRegion(int* line1, int* col1, 
                                       int* line2, int* col2){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->getSelectedRegion(line1, col1, line2, col2);
}

void 
QpMultiLineEdit::execGetRowCol(int position, int *row, int *col){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->getRowCol(position, row, col);
}

int 
QpMultiLineEdit::execExtraWidth() {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->extraWidth();
}

int 
QpMultiLineEdit::execExtraHeight() {
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->extraHeight();
}

int 
QpMultiLineEdit::execCharsInLine(){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->charsInLine();
}

int 
QpMultiLineEdit::execCharsInLine(int line){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->charsInLine(line);
}

int 
QpMultiLineEdit::execGetPosition(int line, int col){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    return tedit->getPosition(line, col);
}

void 
QpMultiLineEdit::execSetTableOptions(int flag){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->setTableOptions(flag);
}

void 
QpMultiLineEdit::execClearTableOptions(int flag){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->clearTableOptions(flag);
}

void 
QpMultiLineEdit::execDeleteSelectedText(){
    QxMultiLineEdit *tedit = (QxMultiLineEdit *)this->getQWidget();
    tedit->deleteSelectedText();
}
