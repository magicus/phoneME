/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * 
 * This source file is specific for Qt-based configurations.
 */

#include <qlabel.h>
#include <qwidget.h>
#include <qpushbutton.h>

#include <qteapp_export.h>
#include <midpPauseResume.h>

/**
 * @file
 *
 * Platform dependent native code to handle incoming call.
 */

#define button_text "Resume"
#define screen_width 130
#define screen_height 80
#define button_width 60
#define button_height 20
#define label_height 40


class SuspendDialog : public QWidget {
    Q_OBJECT
    private:
        QLabel *label;
        QPushButton *button;
    public slots:
        void buttonActivate();
    public:
        SuspendDialog(QWidget *parent);
        bool close(bool );
        bool eventFilter(QObject *obj, QEvent *e);
};
