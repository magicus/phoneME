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

#include <lfpport_form.h>
#include "lfpport_qte_gauge.h"
#include <moc_lfpport_qte_gauge.cpp>

#include "lfpport_qte_mscreen.h"
#include "lfpport_qte_util.h"

#include <qpalette.h>
#include <qcolor.h>
#include <qpainter.h>


/*******************  Gauge : begin *******************/
/**
 * Construct a Gauge native peer with label and body.
 */
Gauge::Gauge(QWidget *parent, const QString &label, int layout)
  : Item(parent, label, layout)
{
}

/**
 * Destructor.
 */
Gauge::~Gauge() {}


/*******************  Gauge : end *******************/


/*******************  NoninteractiveGauge : begin *******************/
/** Constructor */
NoninteractiveGauge::NoninteractiveGauge(QWidget *parent, 
					 const QString &label, 
					 int layout,
					 int initialMaxValue, 
					 int initialValue)
  : Gauge(parent, label, layout) { 

    value = initialValue;
    maxValue = initialMaxValue;
    
    if (initialMaxValue == INDEFINITE) {      
        // indefinite gauge
        qNoninteractive = new MyProgressBar(100, this);
        qNoninteractive->setProgress(0);
      
        switch (initialValue) {

        case CONTINUOUS_IDLE:
            qNoninteractive->showLabel(FALSE);
            qNoninteractive->setDisabled(TRUE);
            break;
        case INCREMENTAL_IDLE:
            qNoninteractive->showLabel(TRUE);
            qNoninteractive->setDisabled(TRUE);
            break;
        case CONTINUOUS_RUNNING:
            qNoninteractive->showLabel(FALSE);
            // connect the timeout() SIGNAL of the timer to the corresponding SLOT
            connect( &timer, SIGNAL( timeout() ), this, SLOT( progress() ) );

            // start the timer with an interval of 1 s...
            if (!timer.isActive()) {
                timer.start( 1000 );   
            }
            break;
        }
    } else {
      // definite range gauge
      qNoninteractive = new MyProgressBar(initialMaxValue, this);
      qNoninteractive->showLabel(TRUE);
      qNoninteractive->setProgress(initialValue);
    }

    qNoninteractive->setFocusPolicy(QWidget::StrongFocus);
    setFocusPolicy(QWidget::StrongFocus);

    // Delegate focus to the ProgressBar
    qNoninteractive->setFocusProxy(this);
}

NoninteractiveGauge::~NoninteractiveGauge() {
  delete qNoninteractive;
}

void 
NoninteractiveGauge::progress()
{
  setValue(CONTINUOUS_RUNNING, -1);
}

/**
 * Set the value of current gauge.
 */
MidpError
NoninteractiveGauge::setValue(int value, int maxValue) {
  if (maxValue == INDEFINITE) {
    int progress;
        switch (value) {

        case INCREMENTAL_IDLE:
	        // qIncrement->setPalette( QPalette( QColor("Grey") ) );
            qNoninteractive->showLabel(TRUE);
	        qNoninteractive->setDisabled(TRUE);
	        break;
        case INCREMENTAL_UPDATING:
            qNoninteractive->showLabel(TRUE);
	        if (this->value == INCREMENTAL_IDLE) {
	            // was an idle gauge, now being activated again
	            qNoninteractive->setDisabled(FALSE);
	        }
	        progress = qNoninteractive->progress();
	
	        if (progress >= qNoninteractive->totalSteps() )  {
	            qNoninteractive->reset();
	            // forcibly setting this to 0, since reset() is setting it to -1
	            // not suitable for this logic
	            if (qNoninteractive->progress() != 0) {
	                qNoninteractive->setProgress(0);
	            }
	        } else {
	            // increment gauge
	            progress += 10;
	            qNoninteractive->setProgress(progress);
	        }
	        break;
        case CONTINUOUS_IDLE:
            qNoninteractive->showLabel(FALSE);
	        qNoninteractive->setDisabled(TRUE);
        	timer.stop();
	        break;
        case CONTINUOUS_RUNNING:
            qNoninteractive->showLabel(FALSE);
            if (this->value == CONTINUOUS_IDLE) {
	            // change from idle to continuous running state
	            qNoninteractive->setDisabled( FALSE );
	            if (!timer.isActive()) {
	                timer.start( 1000 );   
	            }
            }
            // progress the gauge
            progress = qNoninteractive->progress();
      
            if (progress >= qNoninteractive->totalSteps() )  {
	            qNoninteractive->reset();
	            // forcibly setting this to 0, since reset() is setting it to -1
	            // not suitable for further use
	            if (qNoninteractive->progress() != 0) {
	                qNoninteractive->setProgress(0);
	            } 
            } else {
	            progress += 10;
	            qNoninteractive->setProgress(progress);
            }
            break;
        } // switch (value)
  } else {
        qNoninteractive->showLabel(TRUE);
        if (qNoninteractive->totalSteps() != maxValue) {
            // if this was a max value update call
            qNoninteractive->setTotalSteps(maxValue);
        }
        qNoninteractive->setProgress(value);
  }
  
  this->value = value;
  this->maxValue = maxValue;
  return KNI_OK;
}

/**
 * Get the value of current gauge.
 */
MidpError
NoninteractiveGauge::getValue(int *value) {
  *value = qNoninteractive->progress();
  return KNI_OK;
}

void NoninteractiveGauge::bodyResize(int w, int h){
  qNoninteractive->resize(w, h);
}
void NoninteractiveGauge::bodyRelocate(int x, int y) {
  qNoninteractive->move(x, y);
}

/** 
 * Makes non-interactive gauge accept focus and notify the java 
 * peer of the focus change, to enable item specific commands.
 */
void NoninteractiveGauge::focusInEvent(QFocusEvent *event) {
  
  // Notify Java if this is caused by user action
  if (event->reason() != QFocusEvent::Other) {
    MidpFormFocusChanged(this);
  }

  // Continue with focus activation
  QWidget::focusInEvent(event);
}

/** 
 * Overriding, to draw the focus highlight.
 */
void NoninteractiveGauge::paintEvent(QPaintEvent *e)
{
    Item::paintEvent(e);
    
    if (qNoninteractive->hasFocus()) {
      QPainter painter(this);
      QRect focusRect = qNoninteractive->geometry();
      
      focusRect.setLeft(focusRect.x() - 1);
      focusRect.setTop(focusRect.y() - 1);
      focusRect.setWidth(focusRect.width() + 2);
      focusRect.setHeight(focusRect.height() + 2);
      
      style().drawFocusRect(&painter, focusRect, colorGroup());
    }
}

int NoninteractiveGauge::bodyHeightForWidth(int *takenWidth, int w) {

  /* Suppress unused-parameter warning */
  (void)w;

  //QSize minSize = qNoninteractive->minimumSizeHint();
  //*takenWidth = minSize.width();
  //return minSize.height();

  *takenWidth = 300;
  return 15;
}

int NoninteractiveGauge::bodyWidthForHeight(int *takenHeight, int h) {

  /* Suppress unused-parameter warning */
  (void)h;

  //QSize minSize = qNoninteractive->minimumSizeHint();
  //*takenHeight = minSize.height();
  //return minSize.width();

  *takenHeight = 15;
  return 300;
}


/*******************  NoninteractiveGauge : end *******************/



/*******************  InteractiveGauge : start *******************/
/** Constructor */
InteractiveGauge::InteractiveGauge(QWidget *parent, const QString &label, 
				   int layout,
				   int maxValue, int initialValue)
  : Gauge(parent, label, layout) { 
    
    value = initialValue;
    qSlider = new MySlider(0,maxValue,10,initialValue, this);

    // to emit valueChanged() only when pointing tool is released
    // avoiding too many events in between
    qSlider->setTracking(false);
    qSlider->setRange(0, maxValue);
    
    connect(qSlider, SIGNAL(valueChanged(int)), this, SLOT(setValue(int)));
    
    qSlider->setFocusPolicy(QWidget::StrongFocus);
    setFocusPolicy(QWidget::StrongFocus);
    
    // Delegate focus to the Slider
    setFocusProxy(qSlider);
}

InteractiveGauge::~InteractiveGauge() { 
  delete qSlider; 
} 

void 
InteractiveGauge::setValue(int newValue) {
  if (value != newValue) {
    value = newValue;
    MidpFormItemPeerStateChanged(this, newValue);
  }
}


/**
 * Set the value of current gauge.
 */
MidpError
InteractiveGauge::setValue(int newValue, int maxValue) {

  /* Suppress unused-parameter warning */
  (void)maxValue;

  if (value != newValue) {
    emit valueChanged(newValue);
  }

  return KNI_OK;
}

/**
 * Get the value of current gauge.
 */
MidpError
InteractiveGauge::getValue(int *value) {
  *value = qSlider->value();
  return KNI_OK;
}

void InteractiveGauge::bodyResize(int w, int h){
  qSlider->resize(w, h);
}

void InteractiveGauge::bodyRelocate(int x, int y) {
  qSlider->move(x, y);
}

int InteractiveGauge::bodyHeightForWidth(int *takenWidth, int w) {

  /* Suppress unused-parameter warning */
  (void)w;

  //QSize minSize = qSlider->minimumSizeHint();
  //*takenWidth = minSize.width();
  //return minSize.height();

  *takenWidth = 300;
  return 20;
}

int InteractiveGauge::bodyWidthForHeight(int *takenHeight, int h) {

  /* Suppress unused-parameter warning */
  (void)h;

  //QSize minSize = qSlider->minimumSizeHint();
  //*takenHeight = minSize.height();
  //return minSize.width();

  *takenHeight = 20;
  return 300;
}

/*******************  InteractiveGauge : end *******************/


/******************* MyProgressBar : start *******************/
MyProgressBar::MyProgressBar(int maxValue, QWidget *parent, const char* name)
  : QProgressBar(maxValue, parent, name)
{
    labelOn = TRUE;
    labelChanged = FALSE;
}

MyProgressBar::~MyProgressBar()
{
}

/**
 * Shows/hides percentage indicator
 *
 * @param on if label should be switched on or off
 */ 
void MyProgressBar::showLabel(bool on) 
{
    if (labelOn != on) {
        labelChanged = TRUE;
        labelOn = on;
    }
}

/**
 * Called to generate the text displayed. 
 * Overrided to hide text when gauge is in continuous mode.
 */
 bool MyProgressBar::setIndicator(QString & progress_str, 
                                    int progress, int totalSteps)
{
    if (!labelOn) {
        if (labelChanged) {
            labelChanged = FALSE;
            progress_str = QString::fromLatin1("");
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        return QProgressBar::setIndicator(progress_str, progress, totalSteps);
    }
}


/******************* MyProgressBar : end *******************/

/******************* MySlider : start *******************/
MySlider::MySlider(int minValue, int maxValue, int steps, int initialValue,
		   QWidget *parent, const char* name)
  : QSlider(minValue, maxValue, steps, initialValue, 
	    QSlider::Horizontal, parent, name)
{
}

MySlider::~MySlider()
{
}

/** 
 * Makes the interactive gauge accept focus and notify the java 
 * peer of the focus change, to enable item specific commands.
 */
void MySlider::focusInEvent(QFocusEvent *event) {
  
  // Notify Java if this is caused by user action
  if (event->reason() != QFocusEvent::Other) {
    MidpFormFocusChanged(parent());
  }
  
  // Continue with focus activation
  QSlider::focusInEvent(event);
}

/**
 * Override QSlider to notify Java peer of traversal out.
 *
 * @param keyEvent key event to handle
 */
void MySlider::keyPressEvent(QKeyEvent *key)
{
    int k = key->key();
    if (k == Qt::Key_Up || k == Qt::Key_Down)  {
        PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
        mscreen->keyPressEvent(key);
    } else {
        QSlider::keyPressEvent(key);
    }
}

/**
 * Override QSlider to notify Java peer of traversal out.
 *
 * @param keyEvent key event to handle
 */
void MySlider::keyReleaseEvent(QKeyEvent *key)
{
    int k = key->key();
    if (k == Qt::Key_Up || k == Qt::Key_Down)  {
        PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
        mscreen->keyReleaseEvent(key);
    } else {
        QSlider::keyReleaseEvent(key);
    }
}


/******************* MySlider : end *******************/


/*******************  pd* Methods : start *******************/

/**
 * Create a Gauge Qt peer without showing it yet.
 * Upon successful return, fields in *itemPtr should be set properly.
 */
extern "C" MidpError
lfpport_gauge_create(MidpItem* itemPtr, MidpDisplayable* ownerPtr,
		     const pcsl_string* label, int layout, jboolean interactive,
		     int maxValue, int initialValue) {
  QString qlabel;
  
  pcsl_string2QString(*label, qlabel);

  // Fill in MidpItem structure
  QWidget *parent;
  Gauge *gauge;

  parent = (ownerPtr == NULL)? NULL : (QWidget *)ownerPtr->frame.widgetPtr;
  
  if (interactive) {
    gauge = new InteractiveGauge(parent,
				 qlabel,
				 layout,
				 maxValue,
				 initialValue);
  } else {
    gauge = new NoninteractiveGauge(parent,
				    qlabel,
				    layout,
				    maxValue,
				    initialValue);
  }
  
  itemPtr->widgetPtr = gauge;
  initItemPtr(itemPtr, ownerPtr);
  
  return KNI_OK;
}

/**
 * Notifies native peer of a content change in the corresponding Gauge.
 * @param value - the new current value
 * @param maxValue - the new max value
 */
extern "C" MidpError
lfpport_gauge_set_value(MidpItem* itemPtr, int value, int maxValue) {
  return ((Gauge *)itemPtr->widgetPtr)->setValue(value, maxValue);
}

/**
 * Query native peer for current value.
 * @return the current value
 */
extern "C" MidpError
lfpport_gauge_get_value(int *value, MidpItem* itemPtr) {
  return ((Gauge *)itemPtr->widgetPtr)->getValue(value);
}

/*******************  pd* Methods : end *******************/
