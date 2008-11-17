/*
 * @(#)QxFileDialog.cc	1.7 06/10/25
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
#include "QxFileDialog.h"
#include <unistd.h>
#include <qapplication.h>

static const char* const cdup_xpm_data[]={
  "15 13 2 1",
  ". c None",
  "* c #000000",
  "..*****........",
  ".*.....*.......",
  "***************",
  "*.............*",
  "*....*........*",
  "*...***.......*",
  "*..*****......*",
  "*....*........*",
  "*....*........*",
  "*....******...*",
  "*.............*",
  "*.............*",
  "***************"};

QPixmap* QxFileDialog::cdUpIcon = 0;

QString QxFileDialog::lastDirPath = "";

QxFileDialog::QxFileDialog( QWidget *parent, 
                            const char *name, 
                            bool modal,
                            WFlags f): QDialog(parent, name, modal) 
{
  //6393054: Initialize warning widget
  warningWidgetSpace = NULL;

  if ( cdUpIcon == 0 ) {
    cdUpIcon = new QPixmap( (const char **)cdup_xpm_data);
  }
  initUI();
}

void
QxFileDialog::initUI() {
  QWidget *w = this;

  layout = new QGridLayout( w, 0, 0, 1, 1, "layout" );
  lookInLabel = new QLabel( w, "lookInLabel" );
  lookInLabel->setText( tr( "Look In: " ) );
  layout->addWidget( lookInLabel, 0, 0, Qt::AlignLeft );

  fileNameLabel = new QLabel( w, "fileNameLabel" );
  fileNameLabel->setText( tr( "File name: " ) );
  layout->addWidget( fileNameLabel, 1, 0, Qt::AlignLeft );

  // By not specifying the alignment flags fr the line edits,
  // both shall fill the entire cell.

  lookInEdit = new QLineEdit( w, "lookInEdit" );
  layout->addWidget( lookInEdit, 0, 1 );

  fileNameEdit = new QLineEdit( w, "fileNameEdit" );
  layout->addWidget( fileNameEdit, 1, 1 );

  // The line edit fields have a col spacing of 100 and are stretchable.
  layout->addColSpacing( 1, 100 );
  layout->setColStretch( 1, 5 );

  upButton = new QPushButton( w, "upButton" );
  upButton->setText( tr( ".." ) );
  upButton->setPixmap( *cdUpIcon );
  layout->addWidget( upButton, 0, 3, Qt::AlignRight );

  okButton = new QPushButton( w, "okButton" );
  okButton->setText( tr( "ok" ) );
  okButton->setDefault( true );
  layout->addWidget( okButton, 1, 2, Qt::AlignCenter );

  cancelButton = new QPushButton( w, "cancelButton" );
  cancelButton->setText( tr( "cancel" ) );
  layout->addWidget( cancelButton, 1, 3, Qt::AlignRight );

  // Create the file list.
  fileList = new QListBox( w, "fileList" );

  layout->addMultiCellWidget( fileList, 2, 2, 0, 3 );
  layout->addRowSpacing( 2, 120 );
  
  // Connect slots so that we'll be notified when the dialog goes
  // away, whether or not something is selected.
  QObject::connect( upButton, SIGNAL( clicked() ),
                    this, SLOT( upSelected() ) );
  QObject::connect( okButton, SIGNAL( clicked() ),
                    this, SLOT( okSelected() ) );
  QObject::connect( cancelButton, SIGNAL( clicked() ),
                    this, SLOT( cancelSelected() ) );
  QObject::connect( fileList, SIGNAL( selected( QListBoxItem * ) ),
                    this, SLOT( listSelection( QListBoxItem * ) ) );
  QObject::connect( fileList, SIGNAL( clicked( QListBoxItem * ) ),
                    this, SLOT( listClick( QListBoxItem * ) ) );

  // Set parameters for what we want to see in the directory
  currentDir.setPath( lastDirPath.isEmpty()
                      ? QDir::currentDirPath()
                      : lastDirPath );
  currentDir.setFilter( QDir::Files | QDir::Dirs | QDir::Hidden );
  currentDir.setSorting( QDir::Name );
  currentDir.setMatchAllDirs( TRUE );

  // Build our list of files to display
  //populateFileList();

  w->adjustSize();
}

QxFileDialog::~QxFileDialog()
{
  // trivial destructor
}

/*
 * 6393054
 */
void
QxFileDialog::setWarningLabelHeight(int height)
{
  if(height == 0 || warningWidgetSpace != NULL)
    return;

  warningWidgetSpace = new QWidget( this, "warningWidgetSpace" );
  warningWidgetSpace->setMinimumHeight( height );
  warningWidgetSpace->setMaximumWidth( 0 );
  warningWidgetSpace->lower();
  layout->addWidget( warningWidgetSpace, 3, 0 );
  adjustSize();
}

/*
 * A method called to construct a list of all of the files
 * in the current directory, as recorded by currentDir, and
 * then display the list.
 *
 * An optional parameter, which defaults to false, specifies whether to clear
 * the fileNameEdit field.
 */
void
QxFileDialog::populateFileList(bool emptySelection)
{
  // Generate a new list of files in the current directory.
  QString fileL;
  const QFileInfoList *list = currentDir.entryInfoList();

  if (list == 0) {
    return;
  }

  // Zero out the list of files
  fileList->clear();

  // Add each entry in the list to our visible display
  QFileInfoListIterator it( *list );
  QFileInfo *fi;
  while ( ( fi = it.current() ) ) {
    fileL.sprintf( "%s", fi->fileName().data() );
    if ( fi->isDir() ) {
      fileL += "/";
    }
    fileList->insertItem( fileL );
    ++it;
  }

  lookInEdit->setText( currentDir.canonicalPath() );

  if (emptySelection) {
    fileNameEdit->clear();
  }

  fileList->setFocus();

  return;
}
/*
 * JNI setDirectoryNative calls this method for FileDialog's setDirectory() method calls.
 */
void
QxFileDialog::setDirectoryName(QString dir)
{
  QFileInfo fi(dir);
  if ( fi.isDir() ) {
    // It's a dir.  Populates the file list.

    if ( currentDir.cd( fi.filePath(), TRUE ) ) {
      //populateFileList();
    }
    return;
  }
  return;
}
void
QxFileDialog::setFileName(QString name)
{
  fileNameEdit->setText( name );
}
/*
 * A method called when the file dialog is dismissed with a selection.
 * Retrieve the pathname and set the selected file or directory on the
 * target FileDialog for this peer.
 */
void
QxFileDialog::okSelected()
{
  /* Get the directory. */
  QString dirName = lookInEdit->text();
  
  /* Get the file name. */
  QString fileName = fileNameEdit->text(); 
  
  QFileInfo fi(dirName, fileName);

  if ( fi.isDir() ) {
    /* If it's a dir, cd to that dir and populate the file list. */
    if ( currentDir.cd( fi.filePath(), TRUE ) ) {
      populateFileList(true);
    }
    return;
  } else {
    lastDirPath = currentDir.canonicalPath();
  }

  /* Make sure dirName ends with '/' if not empty. */
  if (dirName.isEmpty()) {
    dirName = ".";
  } else {
    if (dirName.at(dirName.length() - 1) != '/') {
      dirName.append('/');
    }
  }

  emit fileSelected(dirName, fileName);

  return;
}


/*
 * If the user has dismissed the dialog, reset the file and
 * dir values to nothing.
 */
void
QxFileDialog::cancelSelected()
{
  emit disposed();
  return;
}


/*
 * If we got a doubleclick, or a return with a selected
 * item, we're done if it's a file. Otherwise cd into the
 * new directory.
 *
 * Now delegates to okSelected().
 */
void
QxFileDialog::listSelection( QListBoxItem *selectedItem )
{
  listClick(selectedItem);
  okSelected();
  return;
}

/*
 * If we get a "clicked" signal, set the context of
 * fileNameEdit.
 */
void
QxFileDialog::listClick( QListBoxItem *selectedItem )
{
  if ( selectedItem != 0 ) {
    setFileName( selectedItem->text() );
  }
  return;
}


/*
 * If the user selects the "up one directory" button.
 */
void
QxFileDialog::upSelected()
{
  if ( currentDir.cdUp() ) {
    populateFileList(true);
  }
  return;
}

