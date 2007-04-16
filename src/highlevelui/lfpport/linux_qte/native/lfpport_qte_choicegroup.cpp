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

/**
 * @file
 * Qt port of ChoiceGroup.
 */

#include "lfpport_qte_mscreen.h"
#include <qnamespace.h>
#include "lfpport_qte_util.h"

#include <gxpportqt_image.h>

#include <lfpport_form.h>
#include "lfpport_qte_choicegroup.h"

#include <moc_lfpport_qte_choicegroup.cpp>

/**
 * Static pointer to hold active popup button that cannot be deleted.
 * This popup button is a child of mscreen. It will either be
 * reused by new popup, or deleted when mscreen is deleted.
 */
static PopupBody *cachedPopupBody;

/**
 * Draw an choice element in a list.
 * We draw choices ourselves to support text wrapping policy and images.
 *
 * @param p QPainter of the choice list
 * @param x upper left corner X of the drawable area
 * @param y upper left corner Y of the drawable area
 * @param w width of the drawable area
 * @param h height of the drawable area
 * @param hPad horizontal padding size
 * @param vPad vertical padding size
 * @param string the text portion of this choice element
 * @param pixmap the image portion of this choice element
 * @param font font to used for the text
 * @param fitPolicy wrapping or not wrapping
 */
void drawElement(QPainter *p,
         int x, int y, int w, int h, int hPad, int vPad,
         QString string, QPixmap *pixmap, QFont font, 
         int fitPolicy) {

  x += hPad/2;
  y += vPad/2;
  w -= hPad;
  h -= vPad;

  p->setFont(font);

  int imgWidth = 0;

  if (pixmap != NULL) { 

    if (string.isEmpty()) {
      p->drawPixmap(x, y, *pixmap);
    } else {
      imgWidth = pixmap->width();
      if (imgWidth > PREF_IMG_WIDTH) {
    imgWidth = PREF_IMG_WIDTH;
      }
      p->drawPixmap(x, y, *pixmap, 0, 0, imgWidth, pixmap->height());
      imgWidth += 3;
    }
  }

  p->drawText(x + imgWidth, y, w - imgWidth, h, 
          fitPolicy == TEXT_WRAP_OFF ?
              Qt::AlignLeft | Qt::AlignTop : 
                  Qt::AlignLeft | Qt::AlignTop | Qt::WordBreak, 
          string);

}

/**
 * Calculate size of an element.
 *
 * @param string the text portion of this choice element
 * @param pixmap the image portion of this choice element
 * @param font font to used for the text
 * @param fitPolicy wrapping or not wrapping
 * @param parentWidth maximum width allowed
 * @param hPad horizontal padding size
 * @param vPad vertical padding size
 * @return size of an element with the given contents
 */ 
QSize sizeElement(QString string, QPixmap *pixmap, QFont font, 
          int fitPolicy, int parentWidth, int hPad, int vPad) {

  int x = hPad/2;
  int y = vPad/2;
  int w = parentWidth - hPad;
  int h = qteapp_get_mscreen()->getScreenHeight() - vPad;

  int imgWidth = 0;
  if (pixmap != NULL) {
    imgWidth = pixmap->width();
    if (imgWidth > PREF_IMG_WIDTH) {
      imgWidth = PREF_IMG_WIDTH;
    }
    if (!string.isEmpty()) {
      imgWidth += 3; // pad between image and text
    }
  }

  QRect rect = QFontMetrics(font).boundingRect(x+imgWidth, y, w-imgWidth, h,
               fitPolicy == TEXT_WRAP_OFF ? 
               Qt::AlignLeft | Qt::AlignTop : 
                       Qt::AlignLeft | Qt::AlignTop | Qt::WordBreak,
               string);

  QSize s = rect.size();
  s.rwidth() += hPad;
  s.rheight() += vPad;

  if (pixmap != NULL) {
    if (string.isEmpty()) {
      s.setHeight(PREF_IMG_HEIGHT);
    }
    s.rwidth() += imgWidth;
  }

  return s;
}

/**
 * Construct a ChoiceGroup base object.
 *
 * @param parent parent widget pointer
 * @param label label string
 * @param layout layout directive associated with this item
 * @param fitPolicy wrapping or not wrapping
 * @param alwaysVertLayout whether label and body should always be on
 *      separated lines
 */
Choice::Choice(QWidget *parent, const QString &label, 
           int layout, int fitPolicy,
           bool alwaysVertLayout) : 
  Item(parent, label, layout, alwaysVertLayout) {
  this->fitPolicy = fitPolicy;
}

/**
 * Destruct a ChoiceGroup base object.
 */
Choice::~Choice() {
}

/**
 * Process events for this ChoiceGroup widget.
 * Since Qt uses SIGNAL/SLOT to deliver events, this function is not used.
 * Do nothing here.
 *
 * @param eventPtr event
 * @return true if the event is fully handled.
 */
jboolean Choice::handleEvent(QEvent *eventPtr) {
    /* Suppress unused-parameter warning */
    (void)eventPtr;

    return KNI_TRUE;
}

/**
 * Construct a customized button that supports text wrapping and image.
 *
 * @param str label text of this button
 * @param img image of this button
 * @param parent parent widget pointer
 * @param isExclusive whether it is part of an exclusive button group
 * @param fitPolicy wrapping or not wrapping the label text
 */
ChoiceButton::ChoiceButton(const QString &str, QPixmap* img, QWidget *parent,
               bool isExclusive, int fitPolicy) 
  : QButton( parent, 0, WRepaintNoErase | WResizeNoErase | WMouseNoMask ) {

  this->isExclusive = isExclusive;
  this->fitPolicy = fitPolicy;

  setToggleButton( TRUE );

// Creation of the QSizePolicy object causes a segmentation violation if
// running in debug mode on device. So comment out this code.
//  setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Fixed ) );

  setText(str);
  setImage(img);

  connect(this, SIGNAL(clicked()), this, SLOT(setFocus()));
}

/** Destruct a ChoiceButton. */
ChoiceButton::~ChoiceButton() {
}

/**
 * Override QButton to notify Form of focus change.
 *
 * @param event focus event to handle
 */
void ChoiceButton::focusInEvent(QFocusEvent *event) {

  // Notify Java if this is caused by user action
  MidpFormFocusChanged(parent()->parent());

  // Continue with focus activation
  QButton::focusInEvent(event);
}

/**
 * Override QButton to notify the button group of status change.
 *
 * @param mouseEvent key event
 */
void ChoiceButton::mouseReleaseEvent(QMouseEvent *mouseEvent) {
  bool curState = QButton::isOn();

  QButton::mouseReleaseEvent(mouseEvent);

  // Notify Java of a state change if the state changed 
  // after user interaction
  if (curState != QButton::isOn()) {
    notifyStateChanged();
  }
}

/**
 * Override QButton to notify the item has to be traversed out.
 *
 * @param keyEvent key event
 */
void ChoiceButton::keyPressEvent(QKeyEvent *keyEvent) {
    ChoiceButtonBoxBody *qGroup = (ChoiceButtonBoxBody *)QButton::parentWidget();

    switch(keyEvent->key()) {
#ifdef QT_KEYPAD_MODE
        case Qt::Key_Select:
#endif
    case Qt::Key_Return:
    {
        if (isExclusive) { // radio button
            if (qGroup->selected() != this) {
                int id = qGroup->id(this);
                if (id != -1) {
                    qGroup->setButton(id);
                    notifyStateChanged();
                }
            }
        } else { // check box
            QButton::setOn(!QButton::isOn());
            notifyStateChanged();
        }
    }
    break;
    case Qt::Key_Up:
    case Qt::Key_Left:
        if (qGroup->find(0)->hasFocus()) {
            PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
            mscreen->keyPressEvent(keyEvent);
        } else {
            QButton::keyPressEvent(keyEvent);
        }
        break;
    case Qt::Key_Down:
    case Qt::Key_Right:
        if (qGroup->find(qGroup->count() - 1)->hasFocus()) {
            PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
            mscreen->keyPressEvent(keyEvent);
        } else {
            QButton::keyPressEvent(keyEvent);
        }
        break;
    default:
        QButton::keyPressEvent(keyEvent);
        break;
    }   
}

/**
 * Override QButton to notify the item has been traversed out.
 *
 * @param keyEvent key event
 */
void ChoiceButton::keyReleaseEvent(QKeyEvent *keyEvent) {
    ChoiceButtonBoxBody *qGroup = (ChoiceButtonBoxBody *)QButton::parentWidget();
    
    switch(keyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Left:
        if (qGroup->find(0)->hasFocus()) {
            PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
            mscreen->keyReleaseEvent(keyEvent);
        } else {
            QButton::keyReleaseEvent(keyEvent);
        }
        break;
    case Qt::Key_Down:
    case Qt::Key_Right:
        if (qGroup->find(qGroup->count() - 1)->hasFocus()) {
            PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
            mscreen->keyReleaseEvent(keyEvent);
        } else {
            QButton::keyReleaseEvent(keyEvent);
        }
        break;
    default:
        QButton::keyReleaseEvent(keyEvent);
        break;
    }   
}

/**
 * Notify Form that selection in a Choice element has changed.
 * For a Choice button that is part of an exclusive group. 
 *
 */
void ChoiceButton::notifyStateChanged() {
  QButtonGroup *qGroup = (QButtonGroup *)parent();
  MidpFormItemPeerStateChanged(((ChoiceButtonBox *)qGroup)->parent(),
                                qGroup->id(this));
}

/**
 * Override QButton to do the calculation based on the contents.
 */
QSize ChoiceButton::sizeHint() const {
  // layout will be forced to happen after there was a resize() call
  // on ChoiceButton's parent (ChoiceButtonBoxBody)
  // Fixed width was set in ChoiceButtonBoxBody.resize()
  return sizeHint(width());
}

/**
 * Size calculations based on the width available.
 *
 * @param w the width available for this ChoiceButton
 */
QSize ChoiceButton::sizeHint(int w) const {

    constPolish();

    QSize mySize = isExclusive ? 
                   style().exclusiveIndicatorSize() : 
                   style().indicatorSize();


    if (img == NULL && text().isEmpty()) {
      mySize.rheight() += 4;
      return mySize;
    }

    mySize.rwidth() += 6; // pad between indicator and content

    if (img != NULL) {
      if (mySize.height() < imgSize.height()) {
    mySize.setHeight(imgSize.height());
      }
      mySize.rwidth() += imgSize.width(); 
      
      if (!text().isEmpty()) {
    // 2 pixel pad afterimage
    mySize.rwidth() += 2;
      }
    }

    if (!text().isEmpty()) {
      w -= mySize.width();

      QRect textRect = 
    QFontMetrics(font()).boundingRect(mySize.width(), 0, 
                      w, qteapp_get_mscreen()->getScreenHeight(),
                      fitPolicy == TEXT_WRAP_OFF ? 
                      AlignLeft | AlignTop :
                      AlignLeft | AlignTop | WordBreak,
                      text()); 
    
      if (textRect.height() > mySize.height()) {
    mySize.setHeight(textRect.height());
      }

      if (textRect.width() >= w) {
    mySize.rwidth() += w;
      } else {
    mySize.rwidth() += textRect.width();
      }
    }

    mySize.rheight() += 4;

    return mySize;
    //expandedTo( QApplication::globalStrut() );
}


/**
 * Override QButton to custom paint text and image.
 *
 * @param paint painter to use
 */
void ChoiceButton::drawButton( QPainter *paint )
{
    paint->setFont(font());

    QSize indSize = isExclusive ?
                    style().exclusiveIndicatorSize() : 
                    style().indicatorSize();

    int x = 0;
    int y = 0;

    if (isExclusive) {
      style().drawExclusiveIndicator(paint, x, y, 
                     indSize.width(), indSize.height(), 
                     colorGroup(), 
                     isOn(), isDown(), isEnabled() );
    } else {
      style().drawIndicator(paint, x, y, 
                indSize.width(), indSize.height(), colorGroup(),
                state(), isDown(), isEnabled());
    }

    x = indSize.width() + 6;
    y = 2;

    int h = height() - 4;
    // the following value should be better calculated
    // (padding is not included)

    int w = width() - x;

    // the rest is content
    QRect focusRect(x - 3, y - 1, w + 3, h + 2);

    if (img != NULL) {
      style().drawItem( paint, x, y, imgSize.width(), imgSize.height(),
            AlignLeft|AlignTop, colorGroup(), isEnabled(),
            img, NULL);
      x += imgSize.width() + 2;
      w -= imgSize.width() + 2;
    } 

    if (!text().isEmpty()) {

      int elWidth = 0;

      if (fitPolicy == TEXT_WRAP_OFF &&
      paint->fontMetrics().width(text()) > w) {
    elWidth = paint->fontMetrics().width("...");
      }
      style().drawItem( paint, x, y, w - elWidth, h,
            fitPolicy == TEXT_WRAP_OFF ? 
            AlignLeft | AlignTop : 
            AlignLeft | AlignTop | WordBreak, 
            colorGroup(), isEnabled(), NULL, text());
      if (elWidth > 0) {
    paint->drawText(x + w - elWidth, y, elWidth, h, 
            AlignLeft | AlignTop, "...");
      }      
    }

    if ( hasFocus() ) {
      if (text().isEmpty() && img == NULL) {
    // draw focus around indicator 1 pixel wider
    focusRect.setLeft(1);
        focusRect.setWidth(indSize.width() + 2);
      }

      style().drawFocusRect(paint, focusRect, colorGroup());
    }      
}

/**
 * Set image portion of this element.
 *
 * @param pixmap new image
 */
void ChoiceButton::setImage(QPixmap *pixmap) {
  img = pixmap;
  if (img != NULL) {
    // clip image to 16 x 16
    imgSize.setWidth(img->width() > 16 ? 16 : img->width() );
    imgSize.setHeight(img->height() > 16 ? 16 : img->height() );
  } else {
    imgSize.setWidth(0);
    imgSize.setHeight(0);
  }
}

/**
 * Set text wrapping policy.
 *
 * @param fitPolicy new policy
 */
void ChoiceButton::setFitPolicy(int fitPolicy) {
  this->fitPolicy = fitPolicy;
}

/**
 * Construct a button group as the body widget of a ChoiceGroup.
 * This is for ChoiceGroup type EXCLUSIVE and MULTIPLE.
 *
 * @param parent parent widget pointer
 */
ChoiceButtonBoxBody::ChoiceButtonBoxBody(QWidget *parent) : 
  QVButtonGroup(parent) {
  setFocusPolicy(QWidget::NoFocus);
}

/** Destruct a button group widget */
ChoiceButtonBoxBody::~ChoiceButtonBoxBody() {
}

/**
 * Override to delegate focus to current selected choice element.
 * 
 * @param event focus event to handle
 */
void ChoiceButtonBoxBody::focusInEvent(QFocusEvent *event) {

  /* Suppress unused-parameter warning */
  (void)event;

  QButton *button;
 
  if (QVButtonGroup::isRadioButtonExclusive()) {
    // there must be at least one button selected
    button = selected();
  } else {
    button = QVButtonGroup::find(0);
  }
  
  if (button != NULL) {
    button->setFocus();
  }
  
  // Continue with focus activation
  // QVButtonGroup::focusInEvent(event);
}


/**
 * Override QButtonGroup to traverse between choice elements.
 *
 * @param key key that user pressed. Only UP/DOWN keys are processed.
 */
void ChoiceButtonBoxBody::moveFocus(int key) {
  QButton *button = (QButton *)QWidget::focusWidget();
  if (button != NULL) {
    PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
    int id = QVButtonGroup::id(button);
    button = NULL;

    if (key == Qt::Key_Up && id > 0) {
      button = QVButtonGroup::find(--id);
      if (button != NULL) {
    button->setFocus();
        int visY = parentWidget()->y();
    if (id > 0) {
      visY += y() + button->y();
    }
    mscreen->ensureVisible(parentWidget()->x(), visY, 0, 0);
      }
    } else if (key == Qt::Key_Down && id < QVButtonGroup::count()-1) {
      button = QVButtonGroup::find(++id);
      if (button != NULL) {
    button->setFocus();
    int visY = parentWidget()->y();
    if (id < QVButtonGroup::count()-1) {
      visY += y() + button->y() + button->height();
    } else {
      visY += parentWidget()->height();
    }
    mscreen->ensureVisible(parentWidget()->x(), visY, 0, 0);
      }
    } else {
      // focus does not move
      return;
    }
  }
}

/**
 * Size calculations based on the width available.
 *
 * @param w the width available for ChoiceButtonBoxBody
 */
QSize ChoiceButtonBoxBody::sizeHint(int w) const {
  int maxWidth = 0;
  QSize buttonSize, size(0,0);

  // Qt uses a padding of 18 to layout QButtons in QButtonGroup
  w -= 2*frameWidth() + 18;

  for(int i = 0, num = count(); i < num; i++) {
    buttonSize = ((ChoiceButton *)find(i))->sizeHint(w);
    if (buttonSize.width() > maxWidth) {
      maxWidth = buttonSize.width();
    }
    size.rheight() += buttonSize.height();
  }
  if (maxWidth > 0) {
    size.setWidth(maxWidth + 2*frameWidth() + 18);
    size.rheight() += 2*frameWidth() + 5 * (count()-1) + 18;
  }

  return size;
}

/**
 * Overrides QVButtonGroup to set fixed width on ChoiceButtons 
 * before the internal layout is done.
 *
 * @param w the new width of this ChoiceButtonBoxBody
 * @param h the new height of this ChoiceButtonBoxBody
 */
void ChoiceButtonBoxBody::resize(int w, int h) {
  // Qt uses a padding of 18 to layout QButtons in QButtonGroup
  int buttonWidth = w - 2*frameWidth() - 18;

  for(int i = 0, num = count(); i < num; i++) {
    ((ChoiceButton *)find(i))->setFixedWidth(buttonWidth);
  }
  
  QVButtonGroup::resize(w, h);
}

/**
 * Override QButtonGroup to return the first choice that is selected.
 *
 * @return the first choice button that is selected. Null if none.
 */
ChoiceButton * ChoiceButtonBoxBody::selected()
{  
  ChoiceButton *button = NULL;

  for(int i= 0; i < count(); i++) {
    button = (ChoiceButton *)find(i);
    if (button != NULL && button->isOn() && button->isToggleButton()) {
      return button;
    }
  }

  return NULL;
}

/**
 * Construct a ChoiceGroup widget with label and ChoiceButtonBoxBody.
 *
 * @param parent parent widget pointer
 * @param label label text
   * @param layout layout directive associated with this choicegroup
 * @param exclusive flag for exclusive choice type
 * @param fitPolicy flag for text wrapping
 */
ChoiceButtonBox::ChoiceButtonBox(QWidget *parent, const QString &label, 
                 int layout, bool exclusive, int fitPolicy) : 
  Choice(parent, label, layout, fitPolicy) {

  qGroup = new ChoiceButtonBoxBody(this);
  
  qGroup->setExclusive(exclusive);
  qGroup->setRadioButtonExclusive(exclusive);

  // ChoiceButtonBox with no elements should not take focus
  setFocusPolicy(QWidget::NoFocus);

  // Delegate focus to ChoiceButtonBoxBody
  setFocusProxy(qGroup);
}

/** Destruct a ChoiceButton Box */
ChoiceButtonBox::~ChoiceButtonBox() {
  deleteAll();
  delete qGroup;
}

/**
 * Move the body widget.
 *
 * @param x new X coordinate
 * @param y new Y coordinate
 */
void ChoiceButtonBox::bodyRelocate(int x, int y) {
  qGroup->move(x, y);
}

/**
 * Resize the body widget.
 *
 * @param w new width
 * @param h new height
 */
void  ChoiceButtonBox::bodyResize(int w, int h) {
  qGroup->resize(w, h);
}

/**
 * Calculate body widget's width if its height is limited to a given value.
 * 
 * @param takenHeight pointer to a value that should be set to real used height
 * @param h maximum allowed height for the body widget
 * @return body widget's width
 */
int ChoiceButtonBox::bodyWidthForHeight(int *takenHeight, int h) {
  // This function is not used on Qt since we don't layout 
  // choices horizontally even when there is no space vertically.

  // Right now width cannot be height dependent 
  // NOTE: if fit policy is supported that should change

  QSize size = ((QVButtonGroup *)qGroup)->sizeHint();
  *takenHeight = size.height();
  if (*takenHeight > h) {
    *takenHeight = h;
  }
  return size.width();
}

/**
 * Calculate body widget's height if its width is limited to a given value.
 *
 * @param takenWidth pointer to a value that should be set to real used width
 * @param h maximum allowed width for the body widget
 * @return body widget's height
 */
int ChoiceButtonBox::bodyHeightForWidth(int *takenWidth, int w) {

  // Right now height is not dependent on width
  QSize size = qGroup->sizeHint(w);

  *takenWidth = size.width();
  if (*takenWidth > w) {
    *takenWidth = w;
  }
  return size.height();
}

/**
 * Insert a choice element with text and image.
 *
 * @param elementNum insert location
 * @param str text portion
 * @param img image portion
 * @param selected initial selection state
 * @return status of this call
 */
MidpError ChoiceButtonBox::insert(int elementNum, 
                  const QString &str, QPixmap* img, 
                  jboolean selected) {
  MidpError err = KNI_OK;
  ChoiceButton *newElement = NULL;

  // store initial group count (after an new buttons is created
  // with a qGroup as a parent count will be +1 )
  int num = qGroup->count();

  if (qGroup->isRadioButtonExclusive()) {
    newElement = new ChoiceButton(str, img, qGroup, true, fitPolicy);
    // images if any are not displayed
  } else {
    newElement = new ChoiceButton(str, img, qGroup, false, fitPolicy);
    newElement->setChecked(selected);
    // images if any are not displayed
  }
  newElement->setFocusPolicy(QWidget::StrongFocus);

  // if the last element is added we do not need
  // readjust the ids before it
  if (elementNum == num) {
    if (qGroup->insert(newElement, elementNum) < 0) {
      return KNI_EINVAL;
    }
  } else {
    QButton *button = NULL;
    for(int i = num-1; i >= elementNum; i--) {
      button = qGroup->find(i);
      if (button != NULL) {
    qGroup->remove(button);
    qGroup->insert(button, i+1);
      }
    }
  
    if (qGroup->insert(newElement, elementNum) < 0) {
      return KNI_EINVAL;
    }

    QPoint p;
    for(int i = elementNum+1; i < num+1; i++) {
      button = qGroup->find(i);
      if (button != NULL) {
    p = button->pos();
    qGroup->remove(button);
    button->reparent(0, p);
    p.setY(p.y() + button->height());
    button->reparent(qGroup, p, true);
    qGroup->insert(button, i);
      }
    }
  } 

  if (qGroup->isRadioButtonExclusive() && selected) {
    qGroup->setButton(elementNum);
  }

  newElement->show();

  setFocusPolicy(QWidget::StrongFocus);
  qGroup->setFocusPolicy(QWidget::StrongFocus);

  return err;
}

/**
 * Delete a choice element.
 *
 * @param elementNum index of the element to be deleted
 * @param selectedIndex new element that should be selected after deletion
 * @return status of this call
 */
MidpError ChoiceButtonBox::deleteElement(int elementNum, int selectedIndex) {
  QButton *button = qGroup->find(elementNum);

  if (button == NULL) {
    return KNI_EINVAL;
  }

  qGroup->remove(button);
  delete button;

  if (qGroup->count() == 0) {
    setFocusPolicy(QWidget::NoFocus);
    qGroup->setFocusPolicy(QWidget::NoFocus);
    return KNI_OK;
  } 

  // if the last element was removed we do not need
  // readjust the ids
  if (elementNum != qGroup->count()) {
    for(int i=elementNum+1; i < qGroup->count()+1; i++) {
      button = qGroup->find(i);
      if (button != NULL) {
    qGroup->remove(button);
    // button->reparent(0, QPoint(0,0));
    qGroup->insert(button, i-1);
      }
    }
  }

  if (selectedIndex >= 0) {
    setSelectedIndex(selectedIndex, true);
  }

  return KNI_OK;
}

/**
 * Delete all choice elements.
 *
 * @return status of this call
 */
MidpError ChoiceButtonBox::deleteAll() {
  int i;
  MidpError err = KNI_OK;

  for (i = qGroup->count()-1; i >= 0; i--) {
    if (deleteElement(i, -1) != KNI_OK) {
      err = KNI_EINVAL;
    }
  }

  return err;
}

/**
 * Update a choice element with new text and image.
 *
 * @param elementNum index of the element
 * @param str new text
 * @param img new image
 * @selected new selection state
 * @return status of this call
 */
MidpError ChoiceButtonBox::set(int elementNum, const QString &str, QPixmap* img,
               jboolean selected) {

  /* Suppress unused-parameter warning */
  (void)img;

  ChoiceButton *button = (ChoiceButton *)qGroup->find(elementNum);

  if (button == NULL) {
    return KNI_EINVAL;
  }

  button->setText(str);
  button->setImage(img);

  setSelectedIndex(elementNum, selected);

  repaint();

  return KNI_OK;
}

/**
 * Set an element's selection state.
 * If the choice group is exclusive, the old selected element will be
 * deselected.
 *
 * @param elementNum index of the element
 * @param selected whether it should be selected
 * @return status of this call
 */
MidpError ChoiceButtonBox::setSelectedIndex(int elementNum, jboolean selected) {
  if (qGroup->isRadioButtonExclusive()) {
    if (selected) {
      qGroup->setButton(elementNum);
    }
  } else {
    QButton *button = qGroup->find(elementNum);
    if (button == NULL) {
      return KNI_EINVAL;
    }
    ((ChoiceButton *)button)->setChecked(selected);
  }
  return KNI_OK;
}

/**
 * Get the index of current selected element.
 *
 * @param elementNum pointer to the element index to be filled in
 *      -1 if no selection.
 * @return status of this call
 */
MidpError ChoiceButtonBox::getSelectedIndex(int *elementNum) {
  QButton *button = qGroup->selected();

  if (button == NULL) {
    *elementNum = -1;
    return KNI_OK;
  }

  *elementNum = qGroup->id(button); // will return -1 if not a member

  return KNI_OK;
}

/**
 * Set all elements' selection state.
 *
 * @param selectedArray array of selection state
 * @param arrayLength size of the array
 * @return status of this call
 */
MidpError ChoiceButtonBox::setSelectedFlags(jboolean* selectedArray,
                    int arrayLength) {

  /* Suppress unused-parameter warning */
  (void)arrayLength;

  int i, n = qGroup->count();
  if (qGroup->isRadioButtonExclusive()) {
    for (int i = 0; i < n; i++) {
      if (selectedArray[i]) {
    setSelectedIndex(i, true);
    return KNI_OK;
      }
    }
    setSelectedIndex(0, true);
  } else {
    ChoiceButton *checkbox;
    for (i = 0; i < n; i++) {
      checkbox = (ChoiceButton *)qGroup->find(i);
      if (checkbox != NULL) {
    checkbox->setChecked(selectedArray[i]);
      }
    }
  }

  return KNI_OK;
}

/**
 * Set all elements' selection state.
 *
 * @param selectedArray array of selection state
 * @param arrayLength size of the array
 * @return status of this call
 */
MidpError ChoiceButtonBox::getSelectedFlags(int *numSelected,
                    jboolean* selectedArray, 
                    int arrayLength) {
  QButton *button;
  int i;
  *numSelected = 0;
  if (qGroup->isRadioButtonExclusive()) {
    for (i = 0; i < arrayLength; i++) {
      selectedArray[i] = false;
    }
    if ((button = qGroup->selected()) != NULL) {
      i = qGroup->id(button);
      if (i >= 0) {
    selectedArray[i] = true;
    *numSelected = 1;
      }
    }
  } else {
    int n = qGroup->count();
    for (i = 0; i < n; i++) {
      if ((button = qGroup->find(i)) != NULL) {
    selectedArray[i] = button->isOn();
    if (selectedArray[i]) {
      numSelected++;
    }
      }
    }
    
    for (i = n; i < arrayLength; i++) {
      selectedArray[i] = false;
    }
  }

  return KNI_OK;
}

/**
 * Test whether a choice element is selected.
 *
 * @param selected pointer to a flag that is to be set on return
 * @param elementNum element index
 * @return status of the call
 */
MidpError ChoiceButtonBox::isSelected(jboolean *selected, int elementNum) {
  QButton *button;
  *selected = false;

  if (qGroup->isRadioButtonExclusive()) {
    button = qGroup->selected();
    if (button != NULL) {
      *selected = (qGroup->id(button) == elementNum);
    }

  } else {
    button = qGroup->find(elementNum);
    if (button != NULL) {
      *selected = button->isOn();
    }
  }

  return KNI_OK;
}

/**
 * Set text wrapping policy.
 *
 * @param fitPolicy new policy
 * @return status of the call
 */
MidpError ChoiceButtonBox::setFitPolicy(int fitPolicy) {
  ChoiceButton *button;
  this->fitPolicy = fitPolicy;
  for (int i = 0; i < qGroup->count(); i++) {
    if ((button = (ChoiceButton *)qGroup->find(i)) != NULL) {
      button->setFitPolicy(fitPolicy);
    }
  }

  return KNI_OK;
}

/**
 * Set Font of an element.
 *
 * @param elementNum index of the element
 * @param f new font
 * @return status of the call
 */
MidpError ChoiceButtonBox::setFont(int elementNum, QFont *f) {
  QButton *button;
  if ((button = qGroup->find(elementNum)) != NULL) {
    button->setFont(*f);
    repaint();
  }
  return KNI_OK;
}

/**
 * Constructs an implicit list element with given string and image.
 *
 * @param str text portion
 * @param img image portion
 * @param f font to used for the text
 */
ListElement::ListElement(const QString &str, QPixmap *img, QFont *f)
 : QListBoxItem() { 

  setText(str);
  pix = img;
  font = f;
}

/**
 * popup element destructor
 */
ListElement::~ListElement() {
}

/**
 * Override QListBoxItem to custom paint a List element.
 *
 * @param p painter to use
 */
void ListElement::paint(QPainter *p) {
  
  QListBox *lb = listBox();
  QSize s = lb->itemRect(this).size();
  int w = qteapp_get_mscreen()->getScreenWidth() 
          - ITEM_BOUND_PAD - ITEM_BOUND_PAD 
          - 2*lb->frameWidth()
          - (lb->verticalScrollBar()->sizeHint()).width();

  int fitPolicy = ((Choice *)lb->parent())->getFitPolicy();
  if (s.width() < w || fitPolicy == TEXT_WRAP_OFF) {
    w = s.width();
  }
  drawElement(p, 0, 0, w, s.height(), 6, 2,
          text(), pix, font == NULL ? lb->font() : *font, fitPolicy);
}

/**
 * Override QListBoxItem to return list element width.
 *
 * @param lb the parent list box
 * @return element width
 */
int ListElement::width(const QListBox *lb) const {
  
  if (!lb) return 0;

  int w = qteapp_get_mscreen()->getScreenWidth() 
          - ITEM_BOUND_PAD - ITEM_BOUND_PAD 
          - 2*lb->frameWidth()
          - (lb->verticalScrollBar()->sizeHint()).width();

  return sizeElement(text(), pix, font == NULL ? lb->font() : *font, 
             ((Choice *)lb->parent())->getFitPolicy(), 
             w, 6, 2).width();
} 

/**
 * Override QListBoxItem to return list element height.
 *
 * @param lb the parent list box
 * @return element height
 */
int ListElement::height(const QListBox *lb) const {
 
  if (!lb) return 0;

  int w = qteapp_get_mscreen()->getScreenWidth() 
          - ITEM_BOUND_PAD - ITEM_BOUND_PAD 
          - 2*lb->frameWidth()
          - (lb->verticalScrollBar()->sizeHint()).width();

  return sizeElement(text(), pix, font == NULL ? lb->font() : *font, 
             ((Choice *)lb->parent())->getFitPolicy(), 
             w, 6, 2).height();
}

/**
 * Set current font for this ListElement.
 *
 * @param f new font of this element
 */
void ListElement::setFont(QFont *f) {
  font = f;
}

/**
 * Returns the pixmap that corresponds to this popup element.
 *
 * @return pixmap of this element
 */
QPixmap* ListElement::pixmap() {
    return pix;
}

/**
 * Returns the font that corresponds to this popup element.
 *
 * @return font of this element
 */
QFont* ListElement::getFont() {
    return font;
}

/**
 * Construct the body widget for an implicit List.
 *
 * @param parent parent widget pointer
 */
ListBody::ListBody(QWidget *parent) : QListBox(parent) {
    setFocusPolicy(QWidget::StrongFocus);
}

/** Destruct a body widget */
ListBody::~ListBody() {
}


/**
 * Override to notify Form focus change.
 *
 * @param event the focus event to handle
 */
void ListBody::focusInEvent(QFocusEvent *event) {
    // Notify Java if this is caused by user action
    if (event->reason() != QFocusEvent::Other) {
    MidpFormFocusChanged(parent());
    }

    // Continue with focus activation
    QListBox::focusInEvent(event);
}

/**
 * Override QListBox to notify the item has to be traversed out.
 *
 * @param keyEvent key event
 */
void ListBody::keyPressEvent(QKeyEvent *keyEvent) {
    // always handle select event because it switches 
    // between the modal and non-modal modes
#ifdef QT_KEYPAD_MODE
    if (keyEvent->key() == Qt::Key_Select) {
        QListBox::keyPressEvent(keyEvent);
    } else if (isModalEditing()) {
#endif
        switch(keyEvent->key()) {
        case Qt::Key_Up:
        case Qt::Key_Left:
            if (item(0)->current()) {
                PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
                mscreen->keyPressEvent(keyEvent);
            } else {
                QListBox::keyPressEvent(keyEvent);
            }
            break;
        case Qt::Key_Down:
        case Qt::Key_Right:
            if (item(count() - 1)->current()) {
                PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
                mscreen->keyPressEvent(keyEvent);
            } else {
                QListBox::keyPressEvent(keyEvent);
            }
            break;
        default:
            QListBox::keyPressEvent(keyEvent);
            break;
        }
#ifdef QT_KEYPAD_MODE
    } else {
        // not handle events while it's in not modal state
        keyEvent->ignore();
    }
#endif
}

/**
 * Override QListBox to notify the item has been traversed out.
 *
 * @param keyEvent key event
 */
void ListBody::keyReleaseEvent(QKeyEvent *keyEvent) { 
    // always handle select event because it switches 
    // between the modal and non-modal modes
#ifdef QT_KEYPAD_MODE
    if (keyEvent->key() == Qt::Key_Select) {
        QListBox::keyReleaseEvent(keyEvent);
    } else if (isModalEditing()) {
#endif
        switch(keyEvent->key()) {
        case Qt::Key_Up:
        case Qt::Key_Left:
            if (item(0)->current()) {
                PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
                mscreen->keyReleaseEvent(keyEvent);
            } else {
                QListBox::keyReleaseEvent(keyEvent);
            }
            break;
        case Qt::Key_Down:
        case Qt::Key_Right:
            if (item(count() - 1)->current()) {
                PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
                mscreen->keyReleaseEvent(keyEvent);
            } else {
                QListBox::keyReleaseEvent(keyEvent);
            }
            break;
        default:
            QListBox::keyReleaseEvent(keyEvent);
            break;
        }
#ifdef QT_KEYPAD_MODE
    } else {
        // not handle events while it's in not modal state
        keyEvent->ignore();
    }
#endif
}



/**
 * Construct a List (implicit type ChoiceGroup) widget.
 *
 * @param parent parent widget pointer
 * @param label label text
 * @param fitPolicy text wrapping policy
 */
List::List(QWidget *parent, const QString &label, int fitPolicy) : 
  Choice(parent, "", 0, fitPolicy) {

  /* Suppress unused-parameter warning */
  (void)label;

  qList = new ListBody(this);
  
  qList->setSelectionMode(QListBox::Single);

  setFocusPolicy(QWidget::StrongFocus);

  // Delegate focus to ListBody
  setFocusProxy(qList);

  connect(qList, SIGNAL(selected(int)), this, SLOT(elementSelected(int)));
}

/** Destruct a List widget */
List::~List() {
  deleteAll();
  delete qList;
}

/**
 * Resize the body widget.
 *
 * @param w new widget
 * @param h new height
 */
void List::bodyResize(int w, int h) {
  qList->resize(w, h); 
}

/**
 * Relocate the body widget.
 *
 * @param x new X coordinate of the left-top corner
 * @param y new Y coordinate of the left-top corner
 */
void List::bodyRelocate(int x, int y) {
  qList->move(x, y);
}

/**
 * Calculate the height of the body widget when its width is limited to a given
 * value.
 *
 * @param takenWidth pointer to the real width used, to be set on return
 * @param w maximum width allowed
 * @return height
 */
int List::bodyHeightForWidth(int *takenWidth, int w) {
  *takenWidth = w;
  return qteapp_get_mscreen()->getScreenHeight() 
    - ITEM_BOUND_PAD - ITEM_BOUND_PAD;
}

/**
 * Calculate the width of the body widget when its height is limited to a given
 * value.
 *
 * @param takenHeight pointer to the real height used, to be set on return
 * @param h maximum height allowed
 * @return width
 */
int List::bodyWidthForHeight(int *takenHeight, int h) {
  *takenHeight = h;
  return qteapp_get_mscreen()->getScreenWidth() - ITEM_BOUND_PAD - ITEM_BOUND_PAD;
}

/**
 * Insert a new element into an implicit list.
 *
 * @param elementNum insert position
 * @param str text portion of the new element
 * @param img image portion of the new element
 * @param selected selection state of the new element
 * @return status of this call
 */
MidpError List::insert(int elementNum, 
               const QString &str, QPixmap* img, 
               jboolean selected) {

  qList->insertItem(new ListElement(str, img, NULL), 
            elementNum);

  qList->setSelected(elementNum, selected);

  return KNI_OK;
}

/**
 * Delete an element from an implicit List.
 *
 * @param elementNum index of the element to be deleted
 * @param selectedIndex new element to be selected after the deletion
 * @return status of this call
 */
MidpError List::deleteElement(int elementNum, int selectedIndex) {
  qList->removeItem(elementNum);
  if (selectedIndex != -1) {
    setSelectedIndex(selectedIndex, true);
  }
  return KNI_OK;
}

/**
 * Delete all elements from an implicit list.
 *
 * @return status of this call
 */
MidpError List::deleteAll() {
  qList->clear();
  return KNI_OK;
}

/**
 * Update the text and image of an element.
 *
 * @param elementNum index of the element
 * @param str new text
 * @param img new image
 * @param selected new selection state
 * @return status of this call
 */
MidpError List::set(int elementNum, const QString &str, QPixmap* img, 
            jboolean selected) {

  qList->changeItem(new ListElement(str, img, NULL), 
            elementNum);
  if (selected) {
    qList->setSelected(elementNum, selected);
  }
  return KNI_OK;
}

/**
 * Set the selected index of this List.
 *
 * @param elementNum index of the element to be set
 * @param selected true if the element should be made selected
 * @return status of this call
 */
MidpError List::setSelectedIndex(int elementNum, jboolean selected) {
  qList->setSelected(elementNum, selected);
  return KNI_OK;
}

/**
 * Get current selection.
 *
 * @param elementNum pointer to the current selection, to be set on return
 * @return status of this call
 */
MidpError List::getSelectedIndex(int *elementNum) {
  *elementNum = qList->currentItem();
  return KNI_OK;
}

/**
 * Set selection state for all elements in this List.
 * Only one element can be selected at a time.
 *
 * @param selectedArray array of selection state
 * @param arrayLength size of the array
 * @return status of this all
 */
MidpError List::setSelectedFlags(jboolean* selectedArray, int arrayLength) {

  /* Suppress unused-parameter warning */
  (void)arrayLength;

  int n = qList->count();
  for (int i = 0; i < n; i++) {
    if (selectedArray[i]) {
      setSelectedIndex(i, true);
      return KNI_OK;
    }
  }
  setSelectedIndex(0, true);

  return KNI_OK;
}

/**
 * Get selection state of all elements.
 *
 * @param numSelected pointer to the number of selected elements,
 *          to be set on return
 * @param selectedArray array to store the selection state
 * @param arrayLength size of the array
 * @return status of this call
 */
MidpError List::getSelectedFlags(int *numSelected,
                 jboolean* selectedArray, int arrayLength) {
  int i;
  for (i = 0; i < arrayLength; i++) {
    selectedArray[i] = false;
  }

  i = qList->currentItem();
  if (i >= 0) {
    selectedArray[i] = true;
    *numSelected = 1;
  } else {
    *numSelected = 0;
  }

  return KNI_OK;
}

/**
 * Test whether an element is selected.
 *
 * @param selected pointer to true/false value, to be set on return
 * @param elementNum element index
 * @return status of this call
 */
MidpError List::isSelected(jboolean *selected, int elementNum) {
  *selected = (qList->currentItem() == elementNum);
  return KNI_OK;
}

/**
 * Set text wrapping policy of this list.
 *
 * @param fitPolicy text wrapping policy
 * @return status of this call
 */
MidpError List::setFitPolicy(int fitPolicy) {
  this->fitPolicy = fitPolicy;

  // repaint list to cover the case when fit policy is changed while list
  // is visible
  // paint code of ListElement will get the new fitpolicy value to be used 
  qList->repaint();

  return KNI_OK;
}

/**
 * Set font for an element of this List.
 *
 * @param elementNum index of the element
 * @param font new font
 * @return status of this call
 */
MidpError List::setFont(int elementNum, QFont *font) {

  ListElement *le = (ListElement *)qList->item(elementNum);
  if (le) {
    le->setFont(font);
    qList->repaint();
  }
  
  return KNI_OK;
}

/**
 * Notify Java peer of new selection.
 *
 * @param elementNum index of the new selected element
 */
void List::elementSelected(int elementNum) {
    MidpFormItemPeerStateChanged(this, elementNum);
}

/**
 * Constructs a popup element with given string and image.
 * 
 * @param popup owner popup menu
 * @param str text portion
 * @param img image portion
 * @param f font to used for this element
 */
PopupElement::PopupElement(Popup *popup, 
               const QString& str, QPixmap* img, 
               QFont *f)
  : string(str) {

  pix = img;
  this->popup = popup;
  this->f = f;
}

/**
 * popup element destructor
 */
PopupElement::~PopupElement() {
}

/**
 * Override to always use full width.
 *
 * @return always true
 */
bool PopupElement::fullSpan () const {
  return true;
}
  
/**
 * Paints string and image content in the space provided
 *
 * @param p painter to use
 * @param cg color group to use
 * @param act whether this element is active
 * @param enabled whether this element is enabled
 * @param x X coordinate of the top left corner
 * @param y Y coordinate of the top left corner
 * @param w maximum width granted
 * @param h maximum height granted
 */
void PopupElement::paint(QPainter *p, const QColorGroup &cg, 
             bool act, bool enabled,
             int x, int y, int w, int h) {

  /* Suppress unused-parameter warning */
  (void)cg;
  (void)act;
  (void)enabled;

  drawElement(p, x, y, w, h, 6, 2, 
          string, pix, f == NULL ? popup->font() : *f, 
          popup->getFitPolicy());
}

/**
 * Calculates the size needed to draw the content of this popup element
 */
QSize PopupElement::sizeHint() {

  return sizeElement(string, pix, f == NULL ? popup->font() : *f, 
             popup->getFitPolicy(), popup->width(), 6, 2);
}

/**
 * Sets the font to be used to draw this element.
 * 
 * @param f new font
 */
void PopupElement::setFont(QFont *f) {
  this->f = f;
}

/**
 * Sets content of this popup element.
 *
 * @param str new text
 * @param pixmap new image
 */
void PopupElement::set(const QString &str, QPixmap *pixmap) {
  string = str.copy();
  pix = pixmap;
}

/**
 * Returns the string that corresponds to this popup element.
 */
QString PopupElement::text() const {
  return string;
}

/**
 * Returns the pixmap that corresponds to this popup element.
 *
 * @return pixmap of this element
 */
QPixmap *PopupElement::pixmap() {
  return pix;
}

/**
 * Returns the font that corresponds to this popup element.
 *
 * @return font of this element
 */
QFont *PopupElement::font() {
  return f;
}

/**
 * Constructor for popup choicegroup body widget.
 *
 * @param parent parent widget pointer
 */
PopupBody::PopupBody(QWidget *parent) : QPushButton(parent) {
  setFocusPolicy(QWidget::StrongFocus);
  oldWidth = -1;
  poppedUp = FALSE;
}

/** Destruct a popup choicegroup body widget */
PopupBody::~PopupBody() {
}

/**
 * Override to notify Form of focus change.
 *
 * @param event focus event to handle
 */
void PopupBody::focusInEvent(QFocusEvent *event) {
    // Notify Java if this is caused by user action
    if (event->reason() != QFocusEvent::Other &&
    event->reason() != QFocusEvent::Popup) {
    MidpFormFocusChanged(parent());
    }

    // Continue with focus activation
    QPushButton::focusInEvent(event);
}

/**
 * Override to patch a feature in Qt that shows popup menu at 
 * (0, 0) if triggered by key press, instead of mouse click.
 *
 * @param keyEvent key event to handle
 */
void PopupBody::keyPressEvent(QKeyEvent *keyEvent) {
    switch(keyEvent->key()) {
        case Qt::Key_Space:
        case Qt::Key_Enter:
        case Qt::Key_Return:
#ifdef QT_KEYPAD_MODE
        case Qt::Key_Select:
#endif
            if (!poppedUp) {
                popupList();
            }
            break;
        case Qt::Key_Up:
        case Qt::Key_Left:
        case Qt::Key_Right:
        case Qt::Key_Down:
            if (!poppedUp) {
                PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
                mscreen->keyPressEvent(keyEvent);
                break;
            }
        default:
            QPushButton::keyPressEvent(keyEvent);
            break;
    }   
}

/**
 * Override to show popup list
 *
 * @param e pointer event to handle
 */
void PopupBody::mousePressEvent(QMouseEvent *e) {
    (void)e;
    if (!poppedUp) {
        popupList();
    } else {
        QPushButton::mousePressEvent(e);
    }
}

/**
 * Override to notify the item has been traversed out
 *
 * @param keyEvent key event to handle
 */
void PopupBody::keyReleaseEvent(QKeyEvent *keyEvent) {
    switch(keyEvent->key()) {
    case Qt::Key_Up:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Down:
        if (!poppedUp) {
            PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
            mscreen->keyReleaseEvent(keyEvent);
            break;
        }
    default:
        QPushButton::keyReleaseEvent(keyEvent);
        break;
    }   
}

/**
 * The event filter steals events from the listbox when it is
 * popped up. 
 */
bool PopupBody::eventFilter(QObject *object, QEvent *event) {
    if (!event) {
        return TRUE;
    } else if (object == qPopup ||
               object == qPopup->viewport()) {
        QMouseEvent *e = (QMouseEvent*)event;
        switch(event->type()) {
            case QEvent::MouseButtonRelease:
                if (qPopup->rect().contains(e->pos())) {
                    // emulate Double Click to get selected event
                    QMouseEvent tmp(QEvent::MouseButtonDblClick,
                            e->pos(), e->button(), e->state()) ;
                    QApplication::sendEvent(object, &tmp);
                    return TRUE;
                }
                break;
            case QEvent::MouseButtonPress:
                if (!qPopup->rect().contains(e->pos())) {
                    popDownList();
                    return TRUE;
                }
                break;
    
            case QEvent::KeyPress:
                switch(((QKeyEvent *)event)->key()) {
                    case Qt::Key_Escape:
            #ifdef QT_KEYPAD_MODE
                    case Qt::Key_Back:
                    case Qt::Key_No:
            #endif
                        popDownList();
                        return TRUE;
                    break;
                }
            default:
                break;
        }
    }
    return QWidget::eventFilter(object, event);
}


/**
 * Override QButton to the calculations based on other
 * elements in the popup
 */
QSize PopupBody::sizeHint() const {
  QSize size = QPushButton::sizeHint();

  int w = qPopup->sizeHint().width() +  style().buttonMargin() +
          style().menuButtonIndicatorWidth(size.height());
  if (w > qteapp_get_mscreen()->getScreenWidth() - ITEM_BOUND_PAD - ITEM_BOUND_PAD) {
    w = qteapp_get_mscreen()->getScreenWidth() - ITEM_BOUND_PAD - ITEM_BOUND_PAD;
  }
  size.setWidth(w);

  return size;
}

/**
 * Override setText to support text truncation
 */
void PopupBody::setText(const QString &newText) {
    QPushButton::setText(newText);
    longText = shortText = newText;
    oldWidth = -1;
}

/**
 * Set popup list variable
 */
void PopupBody::setList(QListBox *list) {
    qPopup = list;
}

/**
 * Get popup list variable
 */
QListBox* PopupBody::getList() {
    return qPopup;
}

/**
 * Popups the popup list.
 * If the list is empty, no selections appear.
 */
void PopupBody::popupList() {
    if (!qPopup->count()) {
        return;
    }
    // Send all listbox events to PopupBody eventFilter
    qPopup->installEventFilter(this);
    qPopup->viewport()->installEventFilter(this);
    PlatformMScreen* mscreen = PlatformMScreen::getMScreen();

    // define size and position for popup
    QPoint pos = mapToGlobal(QPoint(0, 0));
    int winH = mscreen->height();
    int bodyY = pos.y();
    int pX = pos.x();
    int pY = bodyY + height();
    QSize sizeP = qPopup->sizeHint();
    int pW = width();
    int pH = sizeP.height();
    pos = mscreen->mapToGlobal(QPoint(0, 0));
    int headerH = pos.y();

    int lowerPartH = winH + headerH - pY;
    int upperPartH = bodyY - headerH;
    if (pH > lowerPartH) {
        //select the highest part of the widget
        if (lowerPartH > upperPartH) {
            pH = lowerPartH;
        } else if (pH < upperPartH) {
            pY = bodyY - pH;
        } else {
            pY = headerH;
            pH = upperPartH;
        }
    }

    qPopup->resize(pW, pH);
    qPopup->move(pX, pY);
    qPopup->raise();
    qPopup->setVScrollBarMode(QScrollView::Auto);
    qPopup->show();

    poppedUp = TRUE;
}

/**
 * Pops down (removes) the combo box popup list box.
 */
void PopupBody::popDownList() {
    qPopup->removeEventFilter(this);
    qPopup->viewport()->removeEventFilter(this);
    qPopup->hide();
    poppedUp = FALSE;
}

/**
 * Override drawButton to support text truncation
*/
void PopupBody::drawButton( QPainter * p ) {    
    if(oldWidth != width()) {
        shortText = longText;
        truncateQString(shortText, font(), width()-PAD_CHOICE_POPUP_BUTTON);
        QPushButton::setText(shortText);
        oldWidth = width();
    }
    QPushButton::drawButton(p);
};

/**
 * Construct a popup choicegroup widget.
 *
 * @param parent parent widget
 * @param label label text
   * @param layout layout directive associated with this popup
 * @param fitPolicy text wrapping policy
 */
Popup::Popup(QWidget *parent, const QString &label, 
         int layout, int fitPolicy) : 
  Choice(parent, label, layout, fitPolicy, false) {

  // Try to reuse cached popup body
  if (cachedPopupBody != NULL) {
    QPoint newpos;

    cachedPopupBody->reparent(this, newpos, FALSE);
    qButton = cachedPopupBody;
    qPopup  = cachedPopupBody->getList();
    qPopup->reparent(this, newpos, FALSE);
    cachedPopupBody = NULL;

  } else {
    qButton = new PopupBody(this);
    qPopup = new QListBox(this, "", WType_Popup);
    qButton->setList(qPopup);
  }

  selectedIndex = -1;

  // Delegate focus to PopupBody
  setFocusProxy(qButton);

  connect(qPopup, SIGNAL(selected(int)), this, SLOT(elementSelected(int)));
}

/** Destruct a popup widget */
Popup::~Popup() {
    // Special handling for currently active popup menu
    // We cannot delete it because it is actively dispatching Qt event
    // Instead, we cache it for later use
    if (qteapp_get_application()->activePopupWidget() == qPopup) {
        PlatformMScreen * mscreen = PlatformMScreen::getMScreen();
        QPoint newpos;
    
        qPopup->hide(); // Hide popup menu
        qButton->hide(); // Hide the push button
        disconnect(qPopup, SIGNAL(selected(int)), this, SLOT(elementSelected(int)));
        qPopup->clear(); // Empty contents of the popup
        qButton->reparent(mscreen, newpos, FALSE); // Reparent to mscreen
        cachedPopupBody = qButton;
    }
}

/**
 * Resize the body widget of a Popup.
 *
 * @param w new width
 * @param h new height
 */
void Popup::bodyResize(int w, int h) {
  qButton->resize(w, h);
}

/**
 * Move body widget within a popup widget window.
 *
 * @param x X coordinate of top left corner of the body widget.
 *      Relative to popup widget window.
 * @param y Y coordinate of top left corner of the body widget.
 *      Relative to popup widget window.
 */ 
void Popup::bodyRelocate(int x, int y) {
  qButton->move(x, y);
}

/**
 * Calculate body widget height when width is limited to a given value.
 *
 * @param takenWidth return value of the real width used
 * @param w maximum width
 * @return body widget height
 */
int Popup::bodyHeightForWidth(int *takenWidth, int w) {
    int h = qButton->sizeHint().height();

  *takenWidth = calculateMaxElementWidth(w) +
                style().buttonMargin() + 
                style().menuButtonIndicatorWidth(h);

  return h;
}

/**
 * Calculate body widget width when height is limited to a given value.
 *
 * @param takenHeight return value of the real height used
 * @param h maximum height
 * @return body widget width
 */
int Popup::bodyWidthForHeight(int *takenHeight, int h) {

  /* Suppress unused-parameter warning */
  (void)h;

  *takenHeight = qButton->sizeHint().height();

  return calculateMaxElementWidth(qteapp_get_mscreen()->getScreenWidth() - 2*ITEM_BOUND_PAD) +
         style().buttonMargin() + style().menuButtonIndicatorWidth(h);
}

/**
 * Insert an element into this popup choicegroup widget.
 *
 * @param elementNum insert position
 * @param str text portion
 * @param img image portion
 * @param selected selection state
 * @return status of this call
 */
MidpError Popup::insert(int elementNum, 
            const QString &str, QPixmap* img, 
            jboolean selected) {
  qPopup->insertItem(new ListElement(str, img, NULL), 
            elementNum);

  setSelectedIndex(elementNum, selected);

  return KNI_OK;
}

/**
 * Delete an element from this popup choice widget.
 *
 * @param elementNum index of the element to be deleted
 * @param selIndex index of the element that should be selected
 *          after deletion
 * @return status of this call
 */
MidpError Popup::deleteElement(int elementNum, int selIndex) {

  qPopup->removeItem(elementNum);

  if (selIndex != -1) {
    setSelectedIndex(selIndex, true);
  } else {
    selectedIndex = -1;
  }

  return KNI_OK;
}

/**
 * Remove all elements.
 *
 * @return status of this call
 */
MidpError Popup::deleteAll() {
  qPopup->clear();

  selectedIndex = -1;
  return KNI_OK;
}

/**
 * Update an element with new text, image and selection state.
 *
 * @param elementNum index of the element
 * @param str new text
 * @param img new image
 * @param selected new selection state
 * @return status of this call
 */
MidpError Popup::set(int elementNum, const QString &str, QPixmap* img, 
             jboolean selected) {

    qPopup->removeItem(elementNum);
    qPopup->insertItem(new ListElement(str, img, NULL), elementNum);
    setSelectedIndex(elementNum, selected);
 
    return KNI_OK;
}

/**
 * Set selection state of an element.
 *
 * @param elementNum index of the element
 * @param selected new selection state
 * @return status of this call
 */
MidpError Popup::setSelectedIndex(int elementNum, jboolean selected) {
    const QPixmap* pixmap;
    qPopup->setSelected(elementNum, selected);

    if (selected) {
        qButton->setText(qPopup->text(elementNum));
        pixmap  = ((ListElement*)qPopup->item(elementNum))->pixmap();
        if (pixmap != NULL) {
            qButton->setIconSet(QIconSet(*pixmap,QIconSet::Automatic));
        } else {
            qButton->setIconSet(QIconSet());
        }
        qPopup->setCurrentItem(elementNum);
        selectedIndex = elementNum;
    }

    return KNI_OK;
}

/**
 * Get current selected element.
 *
 * @param elementNum return value for index of currently selected element
 * @return status of this call
 */
MidpError Popup::getSelectedIndex(int *elementNum) {
  *elementNum = selectedIndex;
  return KNI_OK;
}

/**
 * Set selection state for all elements.
 *
 * @param selectedArray array of selection state
 * @param arrayLength size of the array
 * @return status of the call
 */
MidpError Popup::setSelectedFlags(jboolean* selectedArray,
                  int arrayLength) {

  /* Suppress unused-parameter warning */
  (void)arrayLength;

  int n = qPopup->count();
  for (int i = 0; i < n; i++) {
    if (selectedArray[i]) {
      setSelectedIndex(i, true);
      return KNI_OK;
    }
  }
  setSelectedIndex(0, true);

  return KNI_OK;
}

/**
 * Get selection state of all elements.
 *
 * @param numSelected return the number of elements currently selected
 * @param selectedArray array for store the states
 * @param arrayLength size of the array
 * @return status of this call
 */
MidpError Popup::getSelectedFlags(int *numSelected,
                  jboolean* selectedArray, int arrayLength) {
  int i;

  for (i = 0; i < arrayLength; i++) {
    selectedArray[i] = false;
  }
  if (selectedIndex >= 0) {
    selectedArray[selectedIndex] = true;
    *numSelected = 1;
  } else {
    *numSelected = 0;
  }

  return KNI_OK;
}

/**
 * Check whether an element is selected.
 *
 * @param selected return whether selected
 * @param elementNum index of the element
 * @return status of this call
 */
MidpError Popup::isSelected(jboolean *selected, int elementNum) {
  *selected = (selectedIndex == elementNum);
  return KNI_OK;
}

/**
 * Set the text wrapping policy.
 *
 * @param fitPolicy TEXT_WRAP_ON if wrapping is enabled
 * @return status of this call
 */
MidpError Popup::setFitPolicy(int fitPolicy) {
    this->fitPolicy = fitPolicy;
    qPopup->repaint();
     
    return KNI_OK;
}

/**
 * Set font for an element.
 *
 * @param elementNum index of the element
 * @param font new font
 * @return status of this call
 */
MidpError Popup::setFont(int elementNum, QFont *font) {
  ListElement *le = (ListElement *)qPopup->item(elementNum);
  if (le) {
    le->setFont(font);
    qPopup->repaint();
  }

    return KNI_OK;
}

/**
 * Notify Java peer that an element is selected.
 *
 * @param id index of the element
 */ 
void Popup::elementSelected(int id) {
    const QPixmap* pixmap;
    qButton->popDownList();
    selectedIndex = id;
    MidpFormItemPeerStateChanged(this, selectedIndex);
    qButton->setText(qPopup->text(selectedIndex));

    pixmap  = ((ListElement*)qPopup->item(selectedIndex))->pixmap();
    if (pixmap != NULL) {
        qButton->setIconSet(QIconSet(*pixmap,QIconSet::Automatic));
    } else {
        qButton->setIconSet(QIconSet());
    }
}

/**
 * Calculate width of the widest element in the popup.
 *
 * @param w is the width available
 * @param withImage true if pixmap should be included in width
 *        calculations
 * @return the width of the widest element in the popup
 */
int Popup::calculateMaxElementWidth(int width) {
    ListElement *popupElement;
    int maxWidth =0;

  // In calculating the maximum width we use TEXT_WRAP_OFF policy.
  // Once maxWidth is larger or equal than the passed in
  // width no more calculations are needed
  for (int w = 0, i = 0, n = qPopup->count(); i < n; i++) {
    popupElement = (ListElement *)qPopup->item(i);
    w = sizeElement(popupElement->text(), 
            popupElement->pixmap(),
            popupElement->getFont() == NULL ? 
             qPopup->font() : *(popupElement->getFont()),
            TEXT_WRAP_OFF, width,
            6, 2).width();

    if (w >= width) {
      return width;
    }

    if (w > maxWidth) {
      maxWidth = w;
    }
  }
  return maxWidth;
}

// **************************************************************************

/**
 * Creates a ChoiceGroup native peer without showing it yet.
 * Upon successful return, fields in *cgPtr should be set properly.
 *
 * @param cgPtr pointer to the choice group's <tt>MidpItem</tt> structure.
 * @param ownerPtr owner screen pointer
 * @param label label text
 * @param choiceType type of the choicegroup
 * @param choices array of choice elements
 * @param numOfChoices size of the array
 * @param selectedIndex index of the element that is selected
 * @param fitPolicy text wrapping policy, TEXT_WRAP_ON if wrapping is enabled
 * @return an indication of success or the reason for failure
 */
extern "C" MidpError
lfpport_choicegroup_create(MidpItem* cgPtr, MidpDisplayable* formPtr,
               const pcsl_string* label, int layout,
               MidpComponentType choiceType,
               MidpChoiceGroupElement* choices, int numOfChoices,
               int selectedIndex, int fitPolicy) {
  QString qlabel, qstring;
  Choice *cg = NULL;
  int i;
  QWidget *parentWidgetPtr = (formPtr == INVALID_NATIVE_ID ? 
                  0 : (QWidget *)formPtr->frame.widgetPtr);

  pcsl_string2QString(*label, qlabel);
  switch(choiceType) {
  case EXCLUSIVE:
  default:
    cg = new ChoiceButtonBox(parentWidgetPtr, qlabel, layout, 1, fitPolicy);
    break;
  case MULTIPLE:
    cg = new ChoiceButtonBox(parentWidgetPtr, qlabel, layout, 0, fitPolicy);
    break;
  case IMPLICIT:
    cg = new List(parentWidgetPtr, "", fitPolicy);
    break;
  case POPUP:
    cg = new Popup(parentWidgetPtr, qlabel, layout, fitPolicy);
  }

  cgPtr->widgetPtr = cg;

  if (cg == NULL) {
    return KNI_ENOMEM;
  }

  initItemPtr(cgPtr, formPtr);

  for(i = 0; i < numOfChoices; i++) {
    pcsl_string2QString(choices[i].string, qstring);

    if (cg->insert(i, qstring, 
           gxpportqt_get_immutableimage_pixmap(choices[i].image), 
           choices[i].selected) < 0) {
        return KNI_ERR;
    }
   
    if (choices[i].font != NULL) {
      cg->setFont(i, (QFont *)choices[i].font);
    }
  }

  if (selectedIndex >= 0) {
    cg->setSelectedIndex(selectedIndex, true);
  }

  return KNI_OK;
}

/**
 * Notifies native peer that an element was inserted into the 
 * <code>ChoiceGroup</code> at the the elementNum specified.
 * 
 * @param cgPtr pointer to the ChoiceGroup native peer
 * @param elementNum the index of the element where insertion occurred
 * @param element the element to be inserted
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_insert(MidpItem* cgPtr,
               int elementNum, MidpChoiceGroupElement element) {
  QString qStr;

  pcsl_string2QString(element.string, qStr);

  return ((Choice *)cgPtr->widgetPtr)->insert(
               elementNum,
               qStr, 
               gxpportqt_get_immutableimage_pixmap(element.image),
               element.selected);
}

/**
 * Notifies native peer that an element referenced by <code>elementNum</code>
 * was deleted in the corresponding ChoiceGroup.
 *
 * @param cgPtr pointer to the ChoiceGroup native peer
 * @param elementNum the index of the deleted element
 * @param selectedIndex index of new selected element after deletion
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_delete(MidpItem* cgPtr, int elementNum, 
               int selectedIndex) {

  return ((Choice *)cgPtr->widgetPtr)->deleteElement(elementNum, selectedIndex);
}

/**
 * Notifies native peer that all elements 
 * were deleted in the corresponding ChoiceGroup.
 *
 * @param cgPtr pointer to the ChoiceGroup native peer
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_delete_all(MidpItem* cgPtr) {
  return ((Choice *)cgPtr->widgetPtr)->deleteAll(); 
}

/**
 * Notifies native peer that the <code>String</code> and 
 * <code>Image</code> parts of the
 * element referenced by <code>elementNum</code> were set in
 * the corresponding ChoiceGroup,
 * replacing the previous contents of the element.
 *
 * @param cgPtr pointer to the ChoiceGroup native peer
 * @param elementNum the index of the element set
 * @param element the new element
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_set(MidpItem* cgPtr,
             int elementNum, MidpChoiceGroupElement element) {
  QString qStr;

  pcsl_string2QString(element.string, qStr);

  return ((Choice *)cgPtr->widgetPtr)->set(
              elementNum, 
              qStr,
              gxpportqt_get_immutableimage_pixmap(element.image),
              element.selected); 
}

/**
 * Notifies native peer that an element was selected/deselected in the 
 * corresponding ChoiceGroup.
 *
 * @param cgPtr pointer to the ChoiceGroup native peer
 * @param elementNum the number of the element. Indexing of the
 * elements is zero-based
 * @param selected the new state of the element <code>true=selected</code>,
 * <code>false=not</code> selected
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_set_selected_index(MidpItem* cgPtr,
        int elementNum, jboolean selected) {
  return ((Choice *)cgPtr->widgetPtr)->setSelectedIndex(elementNum, 
                            selected); 
}

/**
 * Query current selection of native peer.
 *
 * @param elementNum return the index of the selected element.
 * @param cgPtr pointer to the ChoiceGroup native peer
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_get_selected_index(int* elementNum, MidpItem* cgPtr) {
  return ((Choice *)cgPtr->widgetPtr)->getSelectedIndex(elementNum); 
}

/**
 * Notifies native peer that selected state was changed on several elements 
 * in the corresponding MULTIPLE ChoiceGroup.
 *
 * @param cgPtr pointer to the ChoiceGroup native peer
 * @param selectedArray an array in which the method collect the
 * selection status
 * @param arrayLength size of the array
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_set_selected_flags(MidpItem* cgPtr, 
                       jboolean* selectedArray,
                       int arrayLength) {
  return ((Choice *)cgPtr->widgetPtr)->setSelectedFlags(selectedArray,
                            arrayLength); 
}

/**
 * Query all selections on native peer.
 *
 * @param numSelected should be set  to the number of elements selected
 *  in t selectedArray_return
 * @param cgPtr pointer to the ChoiceGroup native peer
 * @param selectedArray_return an pre-allocated array to collect selection
 * @param arrayLength size of the array
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_get_selected_flags(int *numSelected,
                       MidpItem* cgPtr,
                       jboolean* selectedArray_return,
                       int arrayLength) {
  return ((Choice *)cgPtr->widgetPtr)->getSelectedFlags(numSelected,
                            selectedArray_return,
                            arrayLength);
                                 
}

/**
 * Test whether an element is selected.
 *
 * @param selected return value of selection state
 * @param cgPtr pointer to ChoiceGroup native peer
 * @param elementNum index of the element
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_is_selected(jboolean *selected, MidpItem* cgPtr, 
                int elementNum) {
  return ((Choice *)cgPtr->widgetPtr)->isSelected(selected, elementNum);
}

/**
 * Notifies native peer that a new text fit policy was set in the corresponding
 * ChoiceGroup. Same definition as MIDP Spec.
 *
 * @param cgPtr pointer to choicegroup native peer
 * @param fitPolicy preferred content fit policy for choice elements
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_set_fit_policy(MidpItem* cgPtr, int fitPolicy) {
    return ((Choice *)cgPtr->widgetPtr)->setFitPolicy(fitPolicy);
}

/**
 * Notifies native peer that a new font was set for an element with the 
 * specified elementNum in the corresponding ChoiceGroup.
 *
 * @param cgPtr pointer to choicegroup native peer
 * @param elementNum the index of the element, starting from zero
 * @param fontPtr the preferred font to use to render the element
 * @return status of this call
 */
extern "C" MidpError
lfpport_choicegroup_set_font(MidpItem* cgPtr, 
                 int elementNum, PlatformFontPtr fontPtr) {
  return ((Choice *)cgPtr->widgetPtr)->setFont(elementNum,
                           (QFont *)fontPtr);
}

/**
 * Dismisses choicegroup popup
 * 
 * @return an indication of success or the reason for failure
 */
extern "C" MidpError 
lfpport_choicegroup_dismiss_popup() {

  QApplication* qapp = qteapp_get_application();
  if (qapp != NULL) {
    QWidget *popup = qapp->activePopupWidget();
    if (popup != NULL) {
      popup->hide();
    }
  }
  return KNI_TRUE;
}
 
