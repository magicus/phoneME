/*
 * @(#)QxFileDialog.h	1.7 06/10/25
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
#ifndef _QxFILE_DIALOG_H_
#define _QxFILE_DIALOG_H_

#include <qvariant.h>
#include <qdialog.h>
#include <qfile.h> 
#include <qdir.h> 
#include <qstringlist.h> 
#include <qstring.h>
#include <qlayout.h>
#include <qlabel.h> 
#include <qlineedit.h>
#include <qlistbox.h>
#include <qpushbutton.h>

/*
 * Note :- We are not using QFileDialog, becuase on the zaurus we wanted
 * to mimic the look and feel of a FileFialog implemented by Jeode
 */

class QxFileDialog: public QDialog
{
Q_OBJECT

public:
  QxFileDialog( QWidget *parent=0, 
                const char *name=0, 
                bool modal=TRUE,
                WFlags f=0 );
 ~QxFileDialog();

  void setDirectoryName(QString dir);
  void setFileName(QString file);
  void populateFileList(bool emptySelection = false);
  void setWarningLabelHeight(int height);

public slots:

signals:
 void fileSelected(QString dir, QString file); 
 void disposed();

protected:
  virtual void accept() { this->okSelected(); }
  virtual void reject() { this->cancelSelected(); }

private:
  void initUI(void);
  static QPixmap* cdUpIcon;
  static QString lastDirPath;

  QLabel* lookInLabel;
  QLabel* fileNameLabel;
  QLineEdit* lookInEdit;
  QLineEdit* fileNameEdit;
  QPushButton* upButton;
  QPushButton* cancelButton;
  QListBox* fileList;
  QPushButton* okButton;
  QDir currentDir;
  QWidget* warningWidgetSpace;
  QGridLayout* layout;


 private slots: 

  void upSelected();

  void listSelection( QListBoxItem * );

  void listClick( QListBoxItem * );

  void okSelected();

  void cancelSelected();
};


#endif
