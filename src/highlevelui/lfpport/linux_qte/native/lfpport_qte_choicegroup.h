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

#ifndef _LFPPORT_QTE_CHOICEGROUP_H_
#define _LFPPORT_QTE_CHOICEGROUP_H_

#include <lfpport_choicegroup.h>
#include "lfpport_qte_item.h"

#include <qvbuttongroup.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qlist.h>
#include "lfpport_qte_patched.h"

/**
 * Constant for an exclusive-choice list or choice group. Its value is
 * defined in the <i>MIDP Specification</i>.
 */
#define EXCLUSIVE 1

/**
 * Constant for a multiple-choice list or choice group. Its value is
 * defined in the <i>MIDP Specification</i>.
 */
#define MULTIPLE  2

/**
 * Constant for an implicit list. Its value is defined in the <i>MIDP
 * 2.0 Specification</i>.
 */
#define IMPLICIT  3

/**
 * Constant for a pop-up choice group. Its value is defined in the
 * <i>MIDP Specification</i>.
 */
#define POPUP     4

/**
 * Constant for requesting the device's default text-wrapping policy
 * for the elements of the list or choice group.  Its value is defined
 * in the <i>MIDP Specification</i>.
 */
#define TEXT_WRAP_DEFAULT 0

/**
 * Constant for requesting text wrapping for for the elements of the
 * list or choice group with content longer than one line.  Its value
 * is defined in the <i>MIDP Specification</i>.
 */
#define TEXT_WRAP_ON      1

/**
 * Constant for requesting that text wrapping not be used for for the
 * elements of the list or choice group with content longer than one
 * line.  Its value is defined in the <i>MIDP Specification</i>.
 */
#define TEXT_WRAP_OFF     2

/**
 * Maximum width to be used for choicegroup image.
 * If image is wider it will be clipped to this width
 */
#define PREF_IMG_WIDTH 12

/**
 * Maximum height to be used for choicegroup image.
 * If image is taller and text occupies less height 
 * image will be clipped to this height.
 */
#define PREF_IMG_HEIGHT 12

/**
 * Shared class for all types of ChoiceGroup's widget.
 */
class Choice : public Item {

  Q_OBJECT 

  public :

    /**
     * Construct a ChoiceGroup widget.
     *
     * @param parent owner form's widget
     * @param label label text
     * @param fitPolicy text wrapping policy
     * @param alwaysVertLayout true if label should always be on
     *		a separated line from the body
     */
    Choice(QWidget *parent, const QString &label, int layout = 0,
	   int fitPolicy = 0, bool alwaysVertLayout = true);

    /**
     * Destruct this widget.
     */
    virtual ~Choice();

    /**
     * Handle user events.
     * Since Qt uses SIGNAL/SLOT to deliver events directly, 
     * this function is not used.
     *
     * @param eventPtr event to handle
     * @return true if no further handling is needed
     */
    virtual jboolean  handleEvent(QEvent *eventPtr);

    /**
     * Insert an element.
     *
     * @param elementNum index of the inserted element
     * @param str text portion
     * @param img image portion
     * @param selected selection state
     * @return status of this call
     */
    virtual MidpError insert(int elementNum, const QString &str, QPixmap* img, 
			     jboolean selected) = 0;

    /**
     * Delete an element.
     *
     * @param elementNum index of the to-be deleted element
     * @param selectedIndex element to select after deletion
     * @return status of this call
     */
    virtual MidpError deleteElement(int elementNum, int selectedIndex) = 0;

    /**
     * Remove all elements.
     * @return status of this call
     */
    virtual MidpError deleteAll() = 0;

    /**
     * Update an element with new text, image and selection state.
     *
     * @param elementNum index of the element
     * @param str new text
     * @param img new image
     * @param selected new selection state
     * @return status of this call
     */
    virtual MidpError set(int elementNum, const QString &str, QPixmap* img, 
			  jboolean selected) = 0;

    /**
     * Set selection state for an element.
     *
     * @param elementNum index of the element
     * @param selected new selection state
     * @return status of this call
     */
    virtual MidpError setSelectedIndex(int elementNum, jboolean selected) = 0;

    /**
     * Get current selected element.
     *
     * @param elementNum return the index of the currently selected element
     * @return status of this call
     */
    virtual MidpError getSelectedIndex(int *elementNum) = 0;

    /**
     * Set selection state for all elements.
     *
     * @param selectedArray array with selection states
     * @param arrayLength size of the array
     * @return status of this call
     */
    virtual MidpError setSelectedFlags(jboolean* selectedArray,
				       int arrayLength) = 0;

    /**
     * Get the selection state of all the elements.
     *
     * @param numSelected return the number of selected elements
     * @param selectedArray array to store returned selection states
     * @param arrayLength size of the array
     * @return status of this call
     */
    virtual MidpError getSelectedFlags(int *numSelected,
				       jboolean* selectedArray,
				       int arrayLength) = 0;

    /**
     * Get the selection state of an element.
     *
     * @param selected return state
     * @param elementNum index of the element
     * @return status of this call
     */
    virtual MidpError isSelected(jboolean *selected, int elementNum) = 0;

    /**
     * Set the text wrapping policy.
     *
     * @param fitPolicy TEXT_WRAP_ON, TEXT_WRAP_OFF or TEXT_WRAP_DEFAULT
     * @return status of this call
     */
    virtual MidpError setFitPolicy(int fitPolicy) = 0;

    /**
     * Set font of an element
     *
     * @param elementNum index of the element
     * @param new font
     * @return status of this call
     */
    virtual MidpError setFont(int elementNum, QFont *font) = 0;

    /**
     * Returns currently set fit policy.
     * @return fitPolicy currently set
     */
    int getFitPolicy();

 protected:
    /**
     * Text wrapping policy.
     * TEXT_WRAP_ON or TEXT_WRAP_OFF.
     */
    int fitPolicy;
};

/**
 * Returns currently set fit policy in Choice
 * @return fitpolicy currently set in Choice
 */
inline int Choice::getFitPolicy() { return fitPolicy; }

//*******************************************************************

/**
 * Widget for choice elements of EXCLUSIVE and MULTIPLE ChoiceGroups.
 * Extend QButton to support text wrapping and image.
 */
class ChoiceButton : public QButton {
  Q_OBJECT

 private:
  /**
   * Whether it is part of an exclusive group.
   */
  bool isExclusive;

  /**
   * Image to show.
   */
  QPixmap *img; 

  /**
   * Size of the image.
   */
  QSize imgSize;

  /**
   * Text wrapping policy.
   */
  int fitPolicy;

 protected:
  /**
   * Override QButton to notify Java peer of new selection.
   *
   * @param keyEvent key event to handle
   */

  void keyPressEvent(QKeyEvent *keyEvent);
  /**
   * Override QButton to notify Java peer of traversal out.
   *
   * @param keyEvent key event to handle
   */
  void keyReleaseEvent(QKeyEvent *keyEvent);

  /**
   * Override QButton to notify Java peer of new selection.
   *
   * @param mouseEvent key event to handle
   */
  void mouseReleaseEvent(QMouseEvent *mouseEvent);

  /**
   * Makes this item have focus, enabling any item-specific commands; in
   * addition, if the given event was caused by a user action, notifies the
   * Java platform of the change in focus.
   *
   * @param event pointer to the device event corresponding to the change in
   *        focus.
   */
  void focusInEvent(QFocusEvent *event);

  /**
   * Override QButton to custom paint wrapped text and image.
   *
   * @param painter painter to use
   */
  void drawButton(QPainter * painter);


 public:
  /**
   * Construct a choice button.
   *
   * @param str text portion
   * @param img image portion
   * @param parent containing choicegroup body widget
   * @param isExclusive whether it's part of an exclusive group
   * @param fitPolicy text wrapping policy
   */
  ChoiceButton(const QString &str, QPixmap* img, QWidget *parent, 
               bool isExclusive, int fitPolicy);

  /**
   * Destruct a Choice Button widget.
   */
  virtual ~ChoiceButton();


  /**
   * A public method to set a button to on or off.
   *
   * @param check whether it should be shown as selected
   */
  void setChecked( bool check ) { setOn( check ); }

  /**
   * Override QButton to calculate size based on its contents.
   *
   * @return size needed to show this button
   */
  QSize sizeHint() const;

  /**
   * Size calculation based on the width available 
   *
   * @param w the width available for this ChoiceButton
   * @return size needed to show this button in the passed in width
   */
  QSize sizeHint(int w) const;

  /**
   * Update this button with new image.
   *
   * @param pixmap new image
   */
  void setImage(QPixmap *pixmap);

 public slots:

  /**
   * Notify Java peer that its selection state has changed.
   *
   */
  void notifyStateChanged();

  /**
   * Change text wrapping policy.
   *
   * @param fitPolicy new policy
   */
  void setFitPolicy(int fitPolicy);

};

//***************************************************************

/**
 * Body widget for MULTIPLE and EXCLUSIVE ChoiceGroup.
 */
class ChoiceButtonBoxBody : public QVButtonGroup {

 public:
  /**
   * Create a body widget.
   *
   * @param parent ChoiceGroup widget
   */
  ChoiceButtonBoxBody(QWidget *parent);

  /**
   * ChoiceButtonBoxBody destructor
   */
  virtual ~ChoiceButtonBoxBody();

  /**
   * Override QButtonGroup to traverse between choice elements.
   *
   * @param key key user pressed
   */
  void moveFocus(int key);

  /** 
   * Override QButtonGroup to return the first choice that is selected.
   *
   * @return first choice selected
   */
  ChoiceButton *selected();

  /**
   * Size calculation based on the width available.
   * 
   * @param w the width available for this ChoiceButtonBoxBody
   * @return the size of the button base on the fixed width
   */
  QSize sizeHint(int w) const;

  /**
   * Override QVButtonGroup to fix width for ChoiceButtons before
   * internal layout happens.
   *
   * @param w the new width of this ChoiceButtonBoxBody
   * @param h the new height of this ChoiceButtonBoxBody
   */
  void resize(int w, int h);

 protected:

  /**
   * Makes this item have focus, enabling any item-specific commands; in
   * addition, if the given event was caused by a user action, notifies the
   * Java platform of the change in focus.
   *
   * @param event pointer to the device event corresponding to the change in
   *        focus.
   */
  void focusInEvent(QFocusEvent *event);
};

/**
 * Body widget for IMPLICIT List.
 */
class ListBody : public QListBox {
 public:
    
  /**
   * Override QListBox to notify Java peer of traversal out.
   *
   * @param keyEvent key event to handle
   */
  void keyPressEvent(QKeyEvent *keyEvent);

  /**
   * Override QListBox to notify Java peer of traversal out.
   *
   * @param keyEvent key event to handle
   */
  void keyReleaseEvent(QKeyEvent *keyEvent);

  /**
   * Construct.
   * @param parent List widget.
   */
  ListBody(QWidget *parent);

  /**
   * ListBody destructor.
   */
  virtual ~ListBody();

 protected:

  /**
   * Makes this item have focus, enabling any item-specific commands; in
   * addition, if the given event was caused by a user action, notifies the
   * Java platform of the change in focus.
   *
   * @param event pointer to the device event corresponding to the change in
   *        focus.
   */
  void focusInEvent(QFocusEvent *event);

};


/**
 * Body widget for POPUP ChoiceGroup.
 * It's a extended QPushButton with a QPopupMenu.
 */
class PopupBody : public QPushButton {
 protected:

  /** button text before truncation */
  QString longText;

  /** button text, truncated */
  QString shortText;

  /** button width to track resizing and redo truncation when necessary */
  int oldWidth;

  /** popup list */
  QListBox *qPopup;

  /** if list is popped up */
  bool poppedUp;

 public:
  /**
   * PopupBody constructor.
   *
   * @param parent POPUP choicegroup widget
   */
  PopupBody(QWidget *parent);

  /**
   * PopupBody destructor.
   */
  virtual ~PopupBody();

  /**
   * Overrides default implementation to return size based
   * on the width of the largest element in the Popup.
   * @return the size of the popup
   */
  QSize sizeHint() const;

 /**
  * Override setText to support text truncation
  */
  void setText(const QString & newText);

 /**
  * Set popup list variable
  */
  void setList(QListBox *list);
 
 /**
  * Get popup list variable
  */
  QListBox* getList();
 
 /**
  * Override drawButton to support text truncation
  */
 virtual void drawButton( QPainter * p );

    /**
     * Pops down (removes) the combo box popup list box.
     */
    void popDownList();

 protected:
  /**
   * Makes this item have focus, enabling any item-specific commands; in
   * addition, if the given event was caused by a user action, notifies the
   * Java platform of the change in focus.
   *
   * @param event pointer to the device event corresponding to the change in
   *        focus.
   */
  void focusInEvent(QFocusEvent *event);

  /**
   * Override to show popup list 
   *
   * @param keyEvent key event to handle
   */
  void keyPressEvent(QKeyEvent *keyEvent);

  /**
   * Override to show popup list
   *
   * @param e pointer event to handle
   */
  void mousePressEvent( QMouseEvent *e);

  /**
   * Override QPushButton to notify Java peer of traversal out.
   *
   * @param keyEvent key event to handle
   */
  void keyReleaseEvent(QKeyEvent *keyEvent);

  /**
   * The event filter receives events from the listbox when it is
   * popped up. 
   */
  bool eventFilter(QObject *object, QEvent *event);

  /**
   * Popups the popup list.
   * If the list is empty, no selections appear.
   */
  void popupList();

};

// *********************************************************


/**
 * Native widget for MULTIPLE and EXCLUSIVE ChoiceGroup.
 */
class ChoiceButtonBox : public Choice {

  Q_OBJECT

  /** Body widget */
  ChoiceButtonBoxBody *qGroup;

  /** Current selected element. Only used for EXCLUSIVE CG. */
  int selectedIndex;

 protected:
  /**
   * Moves this body widget so that its upper left corner is at the given x
   * and y coordinates.
   *
   * @param x the horizontal coordinate of the upper-left corner of this
   *        choice group.
   * @param y the vertical coordinate of the upper-left corner of this choice
   *        group.
   */
    void bodyRelocate(int x, int y);

    /**
     * Resize this body widget.
     * 
     * @param w new width
     * @param h new height
     */
    void bodyResize(int w, int h);

    /**
     * Calculate body widget height when width is limited to a given value.
     *
     * @param takenWidth return value of the real width used
     * @param w maximum width
     * @return body widget height
     */
    int bodyHeightForWidth(int *takenWidth, int w);

    /**
     * Calculate body widget width when height is limited to a given value.
     *
     * @param takenHeight return value of the real height used
     * @param h maximum height
     * @return body widget width
     */
    int  bodyWidthForHeight(int *takenHeight, int h);

  public :
    /**
     * Constructor.
     *
     * @param parent owner screen's widget
     * @param label label text
     * @param layout layout directive associated with this choicegroup
     * @param exclusive true if EXCLUSIVE CG, otherwise MULTIPLE CG
     * @param fitPolicy wrap text
     */
    ChoiceButtonBox(QWidget *parent, const QString &label, 
		    int layout, bool exclusive, int fitPolicy);

    /**
     * ChoiceButtonBox destructor.
     */
    ~ChoiceButtonBox();

    /**
     * Insert an element.
     *
     * @param elementNum insert position
     * @param str text portion
     * @param img image portion
     * @param selected selection state
     * @return status of this call
     */
    MidpError insert(int elementNum, const QString &str, QPixmap* img, 
		     jboolean selected);

    /**
     * Delete an element from this body widget.
     *
     * @param elementNum index of the element to be deleted
     * @param selIndex index of the element that should be selected
     *			after deletion
     * @return status of this call
     */
    MidpError deleteElement(int elementNum, int selectedIndex);

    /**
     * Delete all elements.
     *
     * @return status of this call
     */
    MidpError deleteAll();

    /**
     * Update an element with new text, image and selection state.
     *
     * @param elementNum index of the element
     * @param str new text
     * @param img new image
     * @param selected new selection state
     * @return status of this call
     */
    MidpError set(int elementNum, const QString &str, QPixmap* img,
		  jboolean selected);

    /**
     * Set selection state of an element.
     *
     * @param elementNum index of the element
     * @param selected new selection state
     * @return status of this call
     */
    MidpError setSelectedIndex(int elementNum, jboolean selected);

    /**
     * Get current selected element.
     *
     * @param elementNum return value for index of currently selected element
     * @return status of this call
     */
    MidpError getSelectedIndex(int *elementNum);

    /**
     * Set selection state for all elements.
     *
     * @param selectedArray array of selection state
     * @param arrayLength size of the array
     * @return status of the call
     */
    MidpError setSelectedFlags(jboolean* selectedArray, int arrayLength);

    /**
     * Get selection state of all elements.
     *
     * @param numSelected return the number of elements currently selected
     * @param selectedArray array for store the states
     * @param arrayLength size of the array
     * @return status of this call
     */
    MidpError getSelectedFlags(int *numSelected,
			       jboolean* selectedArray, int arrayLength);

    /**
     * Check whether an element is selected.
     *
     * @param selected return whether selected
     * @param elementNum index of the element
     * @return status of this call
     */
    MidpError isSelected(jboolean *selected, int elementNum);

    /**
     * Set the text wrapping policy.
     *
     * @param fitPolicy TEXT_WRAP_ON if wrapping is enabled
     * @return status of this call
     */
    MidpError setFitPolicy(int fitPolicy);

    /**
     * Set font for an element.
     *
     * @param elementNum index of the element
     * @param font new font
     * @return status of this call
     */
    MidpError setFont(int elementNum, QFont *font);
};

/**
 * Widget of IMPLICIT List.
 */
class List : public Choice {
  Q_OBJECT

  /**
   * Body widget.
   */
  ListBody *qList;

 protected:
  /**
   * Moves this choice group so that its upper left corner is at the given x
   * and y coordinates.
   *
   * @param x the horizontal coordinate of the upper-left corner of this
   *        choice group.
   * @param y the vertical coordinate of the upper-left corner of this choice
   *        group.
   */
    void bodyRelocate(int x, int y);

    /**
     * Resize the body widget.
     *
     * @param w new widget
     * @param h new height
     */
    void bodyResize(int w, int h);

    /**
     * Calculate the height of the body widget when its width is limited to a given
     * value.
     *
     * @param takenWidth pointer to the real width used, to be set on return
     * @param w maximum width allowed
     * @return height
     */
    int  bodyHeightForWidth(int *takenWidth, int w);

    /**
     * Calculate the width of the body widget when its height is limited to a given
     * value.
     *
     * @param takenHeight pointer to the real height used, to be set on return
     * @param h maximum height allowed
     * @return width
     */
    int  bodyWidthForHeight(int *takenHeight, int h);

  public :
    /**
     * List constructor.
     *
     * @param parent owner screen's widget
     * @param label label text
     * @param fitPolicy text wrapping policy
     */
    List(QWidget *parent, const QString &label, int fitPolicy);

    /**
     * List destructor.
     */
    ~List();

    /**
     * Insert a new element into an implicit list.
     *
     * @param elementNum insert position
     * @param str text portion of the new element
     * @param img image portion of the new element
     * @param selected selection state of the new element
     * @return status of this call
     */
    MidpError insert(int elementNum, const QString &str, QPixmap* img, 
		     jboolean selected);

    /**
     * Delete an element from an implicit List.
     *
     * @param elementNum index of the element to be deleted
     * @param selectedIndex new element to be selected after the deletion
     * @return status of this call
     */
    MidpError deleteElement(int elementNum, int selectedIndex);

    /**
     * Delete all elements from an implicit list.
     *
     * @return status of this call
     */
    MidpError deleteAll();

    /**
     * Update the text and image of an element.
     *
     * @param elementNum index of the element
     * @param str new text
     * @param img new image
     * @param selected new selection state
     * @return status of this call
     */
    MidpError set(int elementNum, const QString &str, QPixmap* img,
		  jboolean selected);

    /**
     * Set the selected index of this List.
     *
     * @param elementNum index of the element to be set
     * @param selected true if the element should be made selected
     * @return status of this call
     */
    MidpError setSelectedIndex(int elementNum, jboolean selected);

    /**
     * Get current selection.
     *
     * @param elementNum pointer to the current selection, to be set on return
     * @return status of this call
     */
    MidpError getSelectedIndex(int *elementNum);

    /**
     * Set selection state for all elements in this List.
     * Only one element can be selected at a time.
     *
     * @param selectedArray array of selection state
     * @param arrayLength size of the array
     * @return status of this all
     */
    MidpError setSelectedFlags(jboolean* selectedArray, int arrayLength);

    /**
     * Get selection state of all elements.
     *
     * @param numSelected pointer to the number of selected elements,
     *			to be set on return
     * @param selectedArray array to store the selection state
     * @param arrayLength size of the array
     * @return status of this call
     */
    MidpError getSelectedFlags(int *numSelected,
			       jboolean* selectedArray, int arrayLength);

    /**
     * Test whether an element is selected.
     *
     * @param selected pointer to true/false value, to be set on return
     * @param elementNum element index
     * @return status of this call
     */
    MidpError isSelected(jboolean *selected, int elementNum);

    /**
     *  Set text wrapping policy of this list.
     *
     * @param fitPolicy text wrapping policy
     * @return status of this call
     */
    MidpError setFitPolicy(int fitPolicy);

    /**
     * Set font for an element of this List.
     *
     * @param elementNum index of the element
     * @param font new font
     * @return status of this call
     */
    MidpError setFont(int elementNum, QFont *font);

  /**
   * Gets the maximum width available for this item without padding
   * 
   * @param withPad if true available width including any padding should
   *                be returned; otherwise padding has to be excluded
   *                from the returned value
   * @return width available for this item without item padding
   */
  int getMaxWidth(bool withPad);

  /**
   * Gets the maximum height available for this item without padding
   * 
   * @param withPad if true available height including any padding should
   *                be returned; otherwise padding has to be excluded
   *                from the returned value
   * @return height available for this item without item padding
   */
  int getMaxHeight(bool withPad);

public slots:
    /**
     * Notifies the Java platform that the given element has changed its
     * selected state, when the change was caused by user action. This
     * function does nothing if this is a multiple-choice list, or if it is a
     * single-choice list and the user selected the element that was already
     * selected.
     *
     * @param elementNum index (using zero-based counting) of this list's
     *        element that has changed its state.
     */
    void elementSelected(int elementNum);
};

/**
 * Widget of POPUP ChoiceGroup.
 */
class Popup : public Choice {
  Q_OBJECT

 private:

  /**
   * Body widget is a custom button with popup menu.
   */
  PopupBody *qButton;

  /**
   * The popup menu with all choices.
   */
  QListBox *qPopup;

  /**
   * Index of the currently selected element, using zero-based
   * counting.
   */
  int selectedIndex;

 protected:

  /**
   * Calculate width of the widest element in the popup.
   *
   * @param width is the maximum width available
   * @return the width of the widest element in the popup
   */
  int calculateMaxElementWidth(int w);

 protected:

  /**
   * Moves this choice group so that its upper left corner is at the given x
   * and y coordinates.
   *
   * @param x the horizontal coordinate of the upper-left corner of this
   *        choice group.
   * @param y the vertical coordinate of the upper-left corner of this choice
   *        group.
   */
  void bodyRelocate(int x, int y);

  /**
   * Resize the body widget of a Popup.
   *
   * @param w new width
   * @param h new height
   */
  void bodyResize(int w, int h);

  /**
   * Calculate body widget height when width is limited to a given value.
   *
   * @param takenWidth return value of the real width used
   * @param w maximum width
   * @return body widget height
   */
  int  bodyHeightForWidth(int *takenWidth, int w);

  /**
   * Calculate body widget width when height is limited to a given value.
   *
   * @param takenHeight return value of the real height used
   * @param h maximum height
   * @return body widget width
   */
  int  bodyWidthForHeight(int *takenHeight, int h);

  public :
    /**
     * Popup constructor.
     *
     * @param parent owner screen's widget
     * @param label label text
     * @param layout layout directive associated with this item
     * @param fitPolicy text wrapping policy
     */
    Popup(QWidget *parent, const QString &label, int layout, int fitPolicy);

    /**
     * Popup destructor.
     */
    ~Popup();
   
    /**
     * Insert an element into this popup choicegroup widget.
     *
     * @param elementNum insert position
     * @param str text portion
     * @param img image portion
     * @param selected selection state
     * @return status of this call
     */
    MidpError insert(int elementNum, const QString &str, QPixmap* img, 
		     jboolean selected);

    /**
     * Delete an element from this popup choice widget.
     *
     * @param elementNum index of the element to be deleted
     * @param selIndex index of the element that should be selected
     *			after deletion
     * @return status of this call
     */
    MidpError deleteElement(int elementNum, int selectedIndex);

    /**
     * Remove all elements.
     *
     * @return status of this call
     */
    MidpError deleteAll();

    /**
     * Update an element with new text, image and selection state.
     *
     * @param elementNum index of the element
     * @param str new text
     * @param img new image
     * @param selected new selection state
     * @return status of this call
     */
    MidpError set(int elementNum, const QString &str, QPixmap* img,
		  jboolean selected);

    /**
     * Set selection state of an element.
     *
     * @param elementNum index of the element
     * @param selected new selection state
     * @return status of this call
     */
    MidpError setSelectedIndex(int elementNum, jboolean selected);

    /**
     * Get current selected element.
     *
     * @param elementNum return value for index of currently selected element
     * @return status of this call
     */
    MidpError getSelectedIndex(int *elementNum);

    /**
     * Set selection state for all elements.
     *
     * @param selectedArray array of selection state
     * @param arrayLength size of the array
     * @return status of the call
     */
    MidpError setSelectedFlags(jboolean* selectedArray, int arrayLength);

    /**
     * Get selection state of all elements.
     *
     * @param numSelected return the number of elements currently selected
     * @param selectedArray array for store the states
     * @param arrayLength size of the array
     * @return status of this call
     */
    MidpError getSelectedFlags(int *numSelected,
			       jboolean* selectedArray, int arrayLength);

    /**
     * Check whether an element is selected.
     *
     * @param selected return whether selected
     * @param elementNum index of the element
     * @return status of this call
     */
    MidpError isSelected(jboolean *selected, int elementNum);

    /**
     * Set the text wrapping policy.
     *
     * @param fitPolicy TEXT_WRAP_ON if wrapping is enabled
     * @return status of this call
     */
    MidpError setFitPolicy(int fitPolicy);

    /**
     * Set font for an element.
     *
     * @param elementNum index of the element
     * @param font new font
     * @return status of this call
     */
    MidpError setFont(int elementNum, QFont *font);

    /**
     * Calculate width of the widest element in the popup.
     *
     * @param w is the width available
     * @param withImage true if pixmap should be included in width
     *        calculations
     * @return the width of the widest element in the popup
     */
    int calculateMaxElementWidth(int w, bool withImage);

public slots:
    /**
     * Sets the selected element to the given element, and renders the pop-up
     * using the given element's text.
     *
     * @param elementNum index (using zero-based counting) of the pop-up
     *        element that the user has selected.
     */
    void elementSelected(int id);
};

//*************************************************************************

/**
 * A custom QListBoxItem that supports text wrapping and image.
 */
class ListElement : public QListBoxItem {

 public:
    /**
     * Constructs a list element with given string and image
     * @param str text
     * @param img image
     * @param font font to use when drawing the text
     */
    ListElement(const QString &str, QPixmap *img, QFont *font);

    /**
     * popup element destructor
     */
    virtual ~ListElement();

    /**
     * Override to custom paint a List element
     *
     * @param p painter to use
     */
    void paint(QPainter *p);

    /**
     * Override to return list element width.
     *
     * @param lb containing listbox widget
     * @return width
     */
    int width(const QListBox *lb) const;

    /**
     * Override to return list element height.
     *
     * @param lb containing listbox widget
     * @return height
     */
    int height(const QListBox *lb) const;

    /**
     * Set font for this ListElement.
     *
     * @param f new font
     */
    void setFont(QFont *f);    

    /**
    * Returns the pixmap that corresponds to this popup element.
    *
    * @return pixmap of this element
    */
    QPixmap* pixmap();

    /**
     * Returns the font that corresponds to this popup element.
     *
     * @return font of this element
     */
    QFont* getFont();


 private:
    /** image */
    QPixmap *pix;
    
    /** text font */
    QFont *font;
};

/**
 * Custom menu item that supports text wrapping and image.
 */
class PopupElement : public QCustomMenuItem {
  public :
    /**
     * Constructs a popup element with given string and image
     * @param popup owner popup widget
     * @param str text
     * @param img image
     * @param f text font
     */
    PopupElement(Popup *popup, const QString &str, QPixmap* img, QFont *f);

    /**
     * popup element destructor
     */
    virtual ~PopupElement();

    /**
     * Override to always use full span.
     *
     * @return always true
     */
    bool fullSpan () const;
  
    /**
     * Override to custom paint string and image content in the space
     * provided.
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
    void paint(QPainter *p, const QColorGroup &cg, bool act, bool enabled,
               int x, int y, int w, int h);

    /**
     * Override to calculate the size needed
     * to draw the contents of this popup element.
     *
     * @return size needed
     */
    QSize sizeHint();

    /**
     * Sets the font to be used to draw this element
     *
     * @param font new font
     */
    void setFont(QFont *font);

    /**
     * Sets content of this popup element.
     *
     * @param str new text
     * @param pixmap new image
     */
    void set(const QString &str, QPixmap *pixmap);

    /**
     * Returns the text that corresponds to this popup element.
     *
     * @return text of this element
     */
    QString text() const;

    /**
     * Returns the pixmap that corresponds to this popup element.
     *
     * @return pixmap of this element
     */
    QPixmap *pixmap();

    /**
     * Returns the font that corresponds to this popup element.
     *
     * @return font of this element
     */
    QFont *font();

 private:

    /** Text */
    QString string;

    /** Image */
    QPixmap *pix;

    /** Text font */
    QFont *f;

    /** Owner menu widget */
    Popup *popup;
};

#endif /* _LFPPORT_QTE_CHOICEGROUP_H_ */
