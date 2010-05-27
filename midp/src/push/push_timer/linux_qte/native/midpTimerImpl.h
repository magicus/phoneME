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

#ifndef QT_TIMER_H
#define QT_TIMER_H

#include <qtimer.h>

/**
 * @file
 *
 * Platform definition of a Timer.
 */

class QTimer;

struct AlarmEntry;

//VMTimer class
class VMTimer : public QObject
{
  Q_OBJECT
public:
    VMTimer(int alarmHandle, QObject *parent = 0, const char* name = 0);

	void startTimer(int time);

private slots:
   	void timerDone();

private:

	QTimer* timer;
	AlarmEntry* alarm;

};

#endif





