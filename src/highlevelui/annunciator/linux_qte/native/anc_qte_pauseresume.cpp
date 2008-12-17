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

#include <qteapp_export.h>
#include <midpPauseResume.h>
#include <midpEventUtil.h>

#if ENABLE_MULTIPLE_DISPLAYS
#include <lcdlf_export.h>
#endif /* ENABLE_MULTIPLE_DISPLAYS */


#include "anc_qte_pauseresume.h"

#include <moc_anc_qte_pauseresume.cpp>
/**
 * @file
 *
 * Platform dependent native code to handle incoming call.
 */

SuspendDialog::SuspendDialog(QWidget *parent)
    : QWidget(parent, NULL, WType_TopLevel | WType_Modal){
    setGeometry(40, 100, screen_width, screen_height);

    label = new QLabel(this,"Application is paused.\nClose this dialog to resume.");
    label->setText("Application is paused.\nClose this dialog to resume.");
    label->setGeometry(0, 10, screen_width, label_height);
    label->show();

    button = new QPushButton(this, NULL);
    button->setGeometry((screen_width - button_width)  >> 1, label_height + 10, button_width, button_height);
    button->setText(button_text);
    connect(button, SIGNAL(clicked()), this, SLOT(buttonActivate()));
    button->show();
    this->installEventFilter(this);
}


bool SuspendDialog::close(bool ) {
    qteapp_get_mscreen()->activateAll();
    return FALSE; // Wait for dismiss callback
}

void SuspendDialog::buttonActivate() {
    pdMidpNotifyResumeAll();
}


// Returns TRUE if this event has been handled
bool SuspendDialog::eventFilter(QObject *obj, QEvent *e) {
    if (((e->type() == QEvent::KeyPress || e->type() == QEvent::Accel) &&
#ifdef QT_KEYPAD_MODE
         ((QKeyEvent *)e)->key() == Qt::Key_Hangup)) {  
#else
         ((QKeyEvent *)e)->key() == Qt::Key_End)) {
#endif
    // Pressing the (x) button means to destroy the
        // foreground MIDlet.
        MidpEvent evt;

        MIDP_EVENT_INITIALIZE(evt);

#if ENABLE_MULTIPLE_ISOLATES
        evt.type = MIDLET_DESTROY_REQUEST_EVENT;
#if ENABLE_MULTIPLE_DISPLAYS  
            evt.DISPLAY = gForegroundDisplayIds[lcdlf_get_current_hardwareId()];  
#else  
            evt.DISPLAY = gForegroundDisplayId;  
#endif /* ENABLE_MULTIPLE_DISPLAYS */  
        evt.intParam1 = gForegroundIsolateId;
        midpStoreEventAndSignalAms(evt);
#else
        evt.type = DESTROY_MIDLET_EVENT;
        midpStoreEventAndSignalForeground(evt);
#endif
        return TRUE;
    }
    return QWidget::eventFilter(obj, e );
}


static SuspendDialog *suspendDialog;

/**
 * Platform handling code for VM pause notification call.
 */
extern "C"
void pdMidpNotifySuspendAll() {
    if (suspendDialog == NULL) {
        QWidget * mscreen = qteapp_get_mscreen()->asWidget();

        suspendDialog = new SuspendDialog(mscreen);
        suspendDialog->show();
    }
}

/**
 * Platform handling code for VM resume notification call.
 */
extern "C"
void pdMidpNotifyResumeAll() {

    if (suspendDialog != NULL) {
        qteapp_get_mscreen()->activateAll();
        // Hide and delete
        delete suspendDialog;
        suspendDialog = NULL;
    }
}
