/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

package javax.microedition.lcdui;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;
import com.sun.midp.configurator.Constants;

/**
 * Look and feel class for <code>Form</code>.
 */
class FormLFImpl extends DisplayableLFImpl implements FormLF {
    /**
     * Creates <code>FormLF</code> associated with passed in form.
     * <code>FormLFImpl</code> maintains an array of views associated
     * with its items.
     *
     * @param form the <code>Form</code> object associated with this
     *             <code>FormLF</code>
     * @param items the array of Items using which the passed in
     *              <code>Form</code> was created
     * @param numOfItems current number of elements
     */
    FormLFImpl(Form form, Item items[], int numOfItems) {

        super(form);

        width -= Constants.VERT_SCROLLBAR_WIDTH;

        if (items == null) {
            itemLFs = new ItemLFImpl[GROW_SIZE];
            // numOfLFs was initialized to 0
            // so there is no need to update it
        } else {
            this.itemLFs = new ItemLFImpl[items.length > GROW_SIZE ?
                                         items.length : GROW_SIZE];

            for (int i = 0; i < numOfItems; i++) {
                itemLFs[i] = (ItemLFImpl)items[i].getLF();
            }

            // right now we have the same number of views as items
            numOfLFs = numOfItems;
        }
    }

    /**
     * Creates <code>FormLF</code> for the passed in screen.
     * Passed in <code>ItemLF</code> is added as the only itemLF present.
     * This constructor is used by <code>List</code> and <code>TextBox</code>.
     *
     * @param screen the <code>Screen</code> object associated with this
     *               <code>FormLFImpl</code>
     * @param item the <code>Item</code> to be added to this screen
     */
    FormLFImpl(Screen screen, Item item) {
        super(screen);

        itemLFs = new ItemLFImpl[1];
        itemLFs[0] = (ItemLFImpl)item.getLF();
        numOfLFs = 1;
    }

    // ************************************************************
    //  public methods - FormLF interface implementation
    // ************************************************************

    /**
     * Returns the width in pixels of the displayable area available for
     * items.
     * The value may depend on how the device uses the screen and may be
     * affected by the presence or absence of the ticker, title,
     * or commands.
     * The <code>Item</code>s of the <code>Form</code> are
     * laid out to fit within this width.
     *
     * @return the width of the <code>Form</code> in pixels
     */
    public int lGetWidth() {
	return width;
    }

    /**
     * Returns the height in pixels of the displayable area available
     * for items.
     * This value is the height of the form that can be displayed without
     * scrolling.
     * The value may depend on how the device uses the screen and may be
     * affected by the presence or absence of the ticker, title,
     * or commands.
     *
     * @return the height of the displayable area of the
     *         <code>Form</code> in pixels
     */
    public int lGetHeight() {
	return height;
    }

    /**
     * Set the current traversal location to the given <code>Item</code>.
     * This call has no effect if the given <code>Item</code> is the
     * current traversal item, or if the given <code>Item</code> is not
     * part of this <code>Form</code>. Note that null can be passed in
     * clear the previously set current item.
     *
     * @param item the <code>Item</code> to make the current traversal item
     */
    public void uItemMakeVisible(Item item) {

	// Focus needs to be transferred to new pending current item
	// Request invalidate if there isn't one scheduled already
	if (pendingCurrentItemLF == null) {
	    lRequestInvalidate();
	}

    pendingCurrentItemLF = (item == null ? null : (ItemLFImpl)item.itemLF);
    }

    /**
     * Notifies look&amp;feel object of an item set in the corresponding
     * <code>Form</code>.
     *
     * @param itemNum the index of the item set
     * @param item the item set in the corresponding <code>Form</code>
     */
    public void lSet(int itemNum, Item item) {

        itemLFs[itemNum] = (ItemLFImpl)item.getLF();

	// Focus index remains at the same location

        lRequestInvalidate();
    }

    /**
     * Notifies look&amp;feel object of an item inserted in the corresponding
     * <code>Form</code>.
     *
     * @param itemNum the index of the inserted item
     * @param item the item inserted in the corresponding <code>Form</code>
     */
    public void lInsert(int itemNum, Item item) {
        if (itemLFs.length == numOfLFs) {
            ItemLFImpl newItemLFs[] =
                new ItemLFImpl[numOfLFs + GROW_SIZE];
            System.arraycopy(itemLFs, 0, newItemLFs, 0, itemNum);
            System.arraycopy(itemLFs, itemNum, newItemLFs, itemNum + 1,
                             numOfLFs - itemNum);
            itemLFs = newItemLFs;
        } else {
            // if we're not appending
            if (itemNum != numOfLFs) {
                System.arraycopy(itemLFs, itemNum, itemLFs, itemNum + 1,
                                 numOfLFs - itemNum);
            }
        }

        itemLFs[itemNum]  = (ItemLFImpl)item.getLF();

	numOfLFs++;

	// Focus remains on the same item
	if (itemNum <= focusIndex) {
	    focusIndex++;
	}

        lRequestInvalidate();
    }

    /**
     * Notifies look&amp;feel object of an item deleted in the corresponding
     * <code>Form</code>.
     *
     * @param itemNum the index of the deleted item
     * @param deleteditem the item deleted in the corresponding form
     */
    public void lDelete(int itemNum, Item deleteditem) {

        if (pendingCurrentItemLF != null && pendingCurrentItemLF.item == deleteditem) {
	    pendingCurrentItemLF = null;
	}

        // if the previous item has new line after, or the next item has
        // new line before, and it's not the last item,
        // than we could just mark the next item as actualBoundsInvalid[Y]
        if (itemNum < (numOfLFs-1)) {
            if (((itemNum > 0) && (itemLFs[itemNum-1].equateNLA()) ||
		 itemLFs[itemNum+1].equateNLB()) &&
                itemLFs[itemNum+1].isNewLine) {

		if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
		    Logging.report(Logging.INFORMATION,
				   LogChannels.LC_HIGHUI_FORM_LAYOUT,
				   " setting actualBoundsInvalid[Y] #" +
				   (itemNum + 1));
		    if (itemNum > 0) {
			Logging.report(Logging.INFORMATION,
				       LogChannels.LC_HIGHUI_FORM_LAYOUT,
				       " | itemLFs[itemNum-1] = "+
				       itemLFs[itemNum - 1]);
		    }
		    Logging.report(Logging.INFORMATION,
				   LogChannels.LC_HIGHUI_FORM_LAYOUT,
				   " | itemLFs[itemNum] = " +
				   itemLFs[itemNum]);
                    if (itemNum < numOfLFs - 1) {
			Logging.report(Logging.INFORMATION,
				       LogChannels.LC_HIGHUI_FORM_LAYOUT,
				       " | itemLFs[itemNum+1] = " +
				       itemLFs[itemNum+1]);
		    }
		}
                itemLFs[itemNum+1].actualBoundsInvalid[Y] = true;
            } else {
                itemLFs[itemNum+1].actualBoundsInvalid[X] = true;
            }
        }

        numOfLFs--;

        if (itemNum < numOfLFs) {
            System.arraycopy(itemLFs, itemNum + 1, itemLFs, itemNum,
                             numOfLFs - itemNum);
        }

        // Delete reference to the last item view
        // that was left after array copy
        itemLFs[numOfLFs] = null;

	// Focus remains on the same item if not deleted
	// Otherwise remains at the same location
	// Otherwise shift to the last item
	if (itemNum < focusIndex ||
	    (itemNum == focusIndex && focusIndex == numOfLFs)) {
	    focusIndex--;
	}

        lRequestInvalidate();
    }

    /**
     * Notifies look&amp;feel object that all items are deleted in
     * the corresponding <code>Form</code>.
     */
    public void lDeleteAll() {
	// Dereference all itemLFImpls so they can be GC'ed
	for (int i = 0; i < numOfLFs; i++) {
	    itemLFs[i] = null;
	}
        numOfLFs = 0;
	focusIndex = -1;
	pendingCurrentItemLF = null;

        lRequestInvalidate();
    }

    /**
     * This method is responsible for:
     * (1) Re-validate the contents of this <code>Form</code>, possibly due
     * to an individual item
     * (2) setup the viewable/scroll position
     * (3) repaint the currently visible <code>Item</code>s
     */
    public void uCallInvalidate() {
        // this call set up the location of the viewable and the scroll.

	// SYNC NOTE:
	// called without LCDUILock, since it may end up calling into a MIDlet
	uShowContents();

	// SYNC NOTE:
	// 1. We are on event dispatch thread, currentDisplay won't change.
	// 2. We are on event dispatch thread, call paint synchronously.
	// 3. Since we could call into app's functions, like traverse(),
	//    showNotify() and paint(), do this outside LCDUILock block.
        currentDisplay.callPaint(0, 0, width, height, null);
    }

    /**
     * Paint the contents of this <code>Form</code>.
     *
     * @param g the <code>Graphics</code> object to paint on
     * @param target the target Object of this repaint
     */
    public void uCallPaint(Graphics g, Object target) {
	int count;

        synchronized (Display.LCDUILock) {
            // super.lCallPaint(g, target); -- obsolete

            if (numOfLFs == 0) {
		return;
            }

	    // SYNC NOTE: since we may call into CustomItem.paint(),
	    // we have to do it outside LCDUILock. So make a copy of the
	    // itemLFs array.
	    if (target instanceof Item) {
		if (((Item)target).owner == this.owner) {
		    ensureDispatchItemArray(1);
		    dispatchItemLFs[0] = (ItemLFImpl)((Item)target).itemLF;
		    count = 1;
		} else {
		    count = 0;
		}
	    } else {
		ensureDispatchItemArray(numOfLFs);
		System.arraycopy(itemLFs, 0, dispatchItemLFs, 0, numOfLFs);
		count = numOfLFs;
	    }
	}

	// Call paint on the copied itemLFs array
	for (int i = 0; i < count; i++) {
	    uPaintItem(dispatchItemLFs[i], g);
	}

	// Dereference ItemLFImpl objects in dispatchItemLFs
	// But leave the shrinking to uCallHide
	resetDispatchItemArray(false);
    }

    /**
     * Notify this <code>Form</code> that it is being shown.
     */
    public void uCallShow() {
	// Create native resources with title and ticker
	super.uCallShow();

	// Setup items and show form native resource
	// SYNC NOTE: May call into app code to collect sizes.
	// Call it outside LCDUILock
	uShowContents();

	// Scroll native window to show current item.
	// traverse() is called only in lSetCurrentItem, not here.
	synchronized (Display.LCDUILock) {
	    // It's possible that the current item is set right before
	    // this locking block, we need to check its native resource id.
	    ItemLFImpl itemLFInFocus = getItemInFocus();
	    if (itemLFInFocus != null &&
		itemLFInFocus.nativeId != INVALID_NATIVE_ID) {
		setCurrentItem0(nativeId, itemLFInFocus.nativeId, 0);
	    }
	}
    }

    /**
     * Notify this <code>Form</code> that it is being hidden.
     */
    public void uCallHide() {

	// No more than one custom item can be in focus at a time
	ItemLFImpl customItemToTraverseOut = null;
	int count = 0;

	synchronized (Display.LCDUILock) {

            pendingCurrentItemLF = null;

	    // We need to loop through our Items to identify those
	    // that traverseOut and hideNotify need to be called.
	    //
	    // SYNC NOTE:
	    // We cannot call into app code while holding LCDUILock.
	    // For CustomItems, we postpone calls to outside this
	    // sync. block.

	    ensureDispatchItemArray(numOfLFs);

	    for (int x = 0; x < numOfLFs; x++) {
		try {
		    // callTraverseOut needs to happen on the item in focus
		    if (itemLFs[x].hasFocus) {
			if (itemLFs[x] instanceof CustomItemLFImpl) {
			    customItemToTraverseOut = itemLFs[x];
			} else {
			    // SYNC NOTE: Items other than CustomItem do not
			    // call into app code in their traverseOut.
			    // We can call it while holding the LCDUILock.
			    itemLFs[x].uCallTraverseOut();
			}
		    }

		    itemLFs[x].lHideNativeResource();

		    // Free native resource of each ItemLF
		    itemLFs[x].deleteNativeResource();

		    // Items that are visible in the viewport
		    // should set their visibleInViewport flag to false and
		    // CustomItems should call app's hideNotify() as well
		    if (itemLFs[x].visibleInViewport) {
			if (itemLFs[x] instanceof CustomItemLFImpl) {
			    // Remember it in temporary array
			    dispatchItemLFs[count++] = itemLFs[x];
			} else {
			    itemLFs[x].lCallHideNotify();
			}
		    }

		} catch (Throwable t) {
		    // do nothing... move on
		}
	    }

	} // synchronized

	// Call CustomItem traverseOut outside LCDUILock
	if (customItemToTraverseOut != null) {
	    customItemToTraverseOut.uCallTraverseOut();
	}

	// Call CustomItem hideNotify outside LCDUILock
	for (count--; count >= 0; count--) {
	    dispatchItemLFs[count].uCallHideNotify();
	}

	// Reset temp array to default size
	resetDispatchItemArray(true);

	// Delete Form's native resource including title and ticker
	super.uCallHide();
    }

    /**
     * Called by <code>Display</code> to notify an <code>ItemLF</code>
     * in current <code>FormLF</code> of a change in its peer state.
     * If the the peerId matches the nativeId of this <code>FormLF</code>,
     * uViewportChanged() will be called to process the scroll
     * notification.
     * Otherwise, if there is an <code>ItemLF</code> that matches the peerId,
     * the <code>ItemLF</code> will be called to process this notification.
     * Otherwise, this is treated as a special notification to this
     * <code>FormLF</code> upon focus changed between items, and
     * parameter 'hint' will contain the index of the new current
     * <code>ItemLF</code>.
     *
     * @param modelVersion the version of the peer's data model
     * @param peerId one of the following:
     *  <ul> <li> the id of this <code>FormLF</code> if viewport
     *            has changed in the corresponding native resource
     *            of this <code>FormLF</code>
     *            (current scroll position is passed as hint)
     *       <li> the id of the <code>ItemLF</code> whose peer state
     *            has changed
     *	     <li> <code>INVALID_NATIVE_ID</code> if a focus
     *            changed notification.
     * @param hint some value that is interpreted only between the peers
     */
    public void uCallPeerStateChanged(int modelVersion,
				      int peerId,
				      int hint) {
	if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
	    Logging.report(Logging.INFORMATION,
			   LogChannels.LC_HIGHUI_FORM_LAYOUT,
			   "-=-=- FormLF: dsPeerStateChanged " +
			   peerId + "/" + hint);
	}

	int notifyType;
	ItemLFImpl oldFocus = null, itemLFToNotify = null;

	synchronized (Display.LCDUILock) {
	    if (modelVersion != super.modelVersion) {
		return; // model version out of sync, ignore the event
	    }

	    // If not matching ItemLF, this is a focus changed notification
	    // 'hint' is the id of the new focused itemLF
	    if (peerId == INVALID_NATIVE_ID) {
		notifyType = 1; // focus changed
		oldFocus = getItemInFocus();
		itemLFToNotify = id2Item(hint);
	    } else if (peerId == nativeId) {
		// there is a scroll event from the native peer,
		// we call show/hide Notify outside of the synchronized block
		notifyType = 2; // viewport changed
	    } else {
		// peerId identified the ItemLF, notify it
		notifyType = 3; // item peer state changed
		itemLFToNotify = id2Item(peerId);
	    }
	}

	// SYNC NOTE: Following calls may end in app code.
	// 	      So do it outside LCDUILock
	switch (notifyType) {

	case 1: // Focus notification
        pendingCurrentItemLF = itemLFToNotify;
        uCallSetCurrentItem(true, CustomItem.NONE);
	    break;
            
	case 2: // Scrolling notification
	    // 'hint' is the new viewport position
	    uViewportChanged(hint, hint + viewportHeight);

	    // Spec requires CustomItem's paint() to be called after
	    // its showNotify() is called and before hideNotify()
	    // it is safe to pass null as both parameters
	    // because only CustomItems will be repainted and they
	    // use their own Graphics
	    uCallPaint(null, null);
	    break;

	case 3: // Item peer notification
	    if (itemLFToNotify != null &&
		itemLFToNotify.uCallPeerStateChanged(hint)) {
		// Notify the itemStateListener
		owner.uCallItemStateChanged(itemLFToNotify.item);
	    }
	    break;

	default:
	    // for safety/completeness.
            Logging.report(Logging.ERROR, LogChannels.LC_HIGHUI_FORM_LAYOUT,
                "FormLFImpl: notifyType=" + notifyType);
	    break;
	}
    }

    /**
     * This method is used in repaint, in order to determine the translation
     * of the draw coordinates.
     *
     * @return <code>true</code>, if the scroll responsibility is on the
     *         native platform.
     *         <code>false</code>, if the scroll is done at Java level.
     */
    public boolean uIsScrollNative() {
	// only native form overrides this and returns true
	return true;
    }

    // *****************************************************
    //  Package private methods
    // *****************************************************

    /**
     * Check the key and return true if it's navigation key
     * @param key key code
     * @return true if the key is navigation key false otherwise 
     */
    private boolean isNavigationKey(int key) {
        return key == Canvas.UP ||
            key == Canvas.LEFT ||
            key == Canvas.DOWN ||
            key == Canvas.RIGHT;
    }
    
    /**
     * Handle a key press.
     *
     * @param keyCode the key code of the key which was pressed
     */
    void uCallKeyPressed(int keyCode) {
        if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
            Logging.report(Logging.INFORMATION,
                           LogChannels.LC_HIGHUI_FORM_LAYOUT,
                           "got callKeyPressed: " + keyCode);
        }
        int dir = KeyConverter.getGameAction(keyCode);
        if (isNavigationKey(dir)) {
            boolean forward = true;
            switch (dir) {
            case Canvas.UP:
            case Canvas.LEFT:
                forward = false;
                break;
            case Canvas.DOWN:
            case Canvas.RIGHT:
            default:
                forward = true;
                break;
            }
            uCallSetCurrentItem(forward, dir);
        } else {
            ItemLFImpl v = null;
            synchronized (Display.LCDUILock) {
                v = getItemInFocus();
            }
            
            // pass the keypress onto the current item
            if (v != null && v instanceof CustomItemLFImpl) {

                // NOTE: customItem.getInteractionModes() determines
                // the supported events. The Zaurus platform implementation
                // does not support traversal in any direction.
                // if it is desired to support horizontal and/or vertical
                // traversal, than the proper flags must be set accordingly.
            
                // pass all key events to the CustomItem, including arrows
                v.uCallKeyPressed(keyCode);
            }
        }
    }

    /**
     * Handle a key release event.
     *
     * @param keyCode the key which was released
     */
    void uCallKeyReleased(int keyCode) {
	if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
	    Logging.report(Logging.INFORMATION,
			   LogChannels.LC_HIGHUI_FORM_LAYOUT,
			   "got callKeyReleased: " + keyCode);
	}
        
        if (!isNavigationKey(KeyConverter.getGameAction(keyCode))) {
            ItemLFImpl v = null;
            synchronized (Display.LCDUILock) {
                v = getItemInFocus();
            } // synchronized
            
            // SYNC NOTE: formMode can only change as a result of a
            // traversal, which can only occur serially on the event
            // thread, so its safe to use it outside of the lock
            
            if (v != null && v instanceof CustomItemLFImpl) {
                v.uCallKeyReleased(keyCode);
            }
        }
    }

    /**
     * Handle a key repeat.
     *
     * @param keyCode the key code of the key which was repeated
     */
    void uCallKeyRepeated(int keyCode) {
	if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
	    Logging.report(Logging.INFORMATION,
			   LogChannels.LC_HIGHUI_FORM_LAYOUT,
			   "got callKeyRepeated: " + keyCode);
	}
        if (isNavigationKey(KeyConverter.getGameAction(keyCode))) {
            uCallKeyPressed(keyCode);
        } else {
            ItemLFImpl v = null;
            synchronized (Display.LCDUILock) {
                v = getItemInFocus();
            } // synchronized

            // SYNC NOTE: formMode can only change as a result of a
            // traversal, which can only occur serially on the event
            // thread, so its safe to use it outside of the lock

            if (v != null && v instanceof CustomItemLFImpl) {
                v.uCallKeyRepeated(keyCode);
            }
        }
    }

    /**
     * Handle a pointer pressed event.
     *
     * @param x The x coordinate of the press
     * @param y The y coordinate of the press
     */
    void uCallPointerPressed(int x, int y) {
	if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
	    Logging.report(Logging.INFORMATION,
			   LogChannels.LC_HIGHUI_FORM_LAYOUT,
			   "got callPointerPressed: " + x + "," + y);
	}

        ItemLFImpl v = null;

        synchronized (Display.LCDUILock) {

	    v = getItemInFocus();

	    // stop here if no current item to handle the key
            if (v == null) {
		return;
	    }

        } // synchronized

        // SYNC NOTE: formMode can only change as a result of a
        // traversal, which can only occur serially on the event
        // thread, so its safe to use it outside of the lock

	if (v instanceof CustomItemLFImpl)
	    v.uCallPointerPressed(x, y);
    }

    /**
     * Handle a pointer released event.
     *
     * @param x The x coordinate of the release
     * @param y The y coordinate of the release
     */
    void uCallPointerReleased(int x, int y) {
	if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
	    Logging.report(Logging.INFORMATION,
			   LogChannels.LC_HIGHUI_FORM_LAYOUT,
			   "got callPointerReleased: " + x + "," + y);
	}

        ItemLFImpl v = null;

        synchronized (Display.LCDUILock) {

	    v = getItemInFocus();

	    // stop here if no current item to handle the key
            if (v == null) {
		return;
	    }

        } // synchronized

        // SYNC NOTE: formMode can only change as a result of a
        // traversal, which can only occur serially on the event
        // thread, so its safe to use it outside of the lock

	if (v instanceof CustomItemLFImpl)
	    v.uCallPointerReleased(x, y);
    }

    /**
     * Handle a pointer dragged event.
     *
     * @param x The x coordinate of the drag
     * @param y The y coordinate of the drag
     */
    void uCallPointerDragged(int x, int y) {
	if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
	    Logging.report(Logging.INFORMATION,
			   LogChannels.LC_HIGHUI_FORM_LAYOUT,
			   "got callPointerDragged: " + x + "," + y);
	}

        ItemLFImpl v = null;

        synchronized (Display.LCDUILock) {

	    v = getItemInFocus();

	    // stop here if no current item to handle the key
            if (v == null) {
		return;
	    }

        } // synchronized

        // SYNC NOTE: formMode can only change as a result of a
        // traversal, which can only occur serially on the event
        // thread, so its safe to use it outside of the lock

	if (v instanceof CustomItemLFImpl)
	    v.uCallPointerDragged(x, y);
    }

    /**
     * Gets item currently in focus.
     *
     * @return the item currently in focus in this form;
     *         if there are no items in focus, <code>null</code> is returned
     */
    public Item lGetCurrentItem() {
	ItemLFImpl v = getItemInFocus();

	if (v == null) {
	    return null;
	}

	return v.item;
    }

    /**
     * Paint an item.
     *
     * @param itemLF the <code>ItemLFImpl</code> to paint
     * @param g the <code>Graphics</code> object to paint to
     */
    void uPaintItem(ItemLFImpl itemLF, Graphics g) {
	synchronized (Display.LCDUILock) {
	    // NOTE: Its possible, that an Item is in an invalid state
	    // during a requested repaint. Its ok to simply return,
	    // because it means there is a validation event coming on
	    // the event thread. When the form re-validates, the Item
	    // will be given a proper bounds and will be repainted
	    if (itemLF.actualBoundsInvalid[X]
	    || itemLF.actualBoundsInvalid[Y]
	    || itemLF.actualBoundsInvalid[WIDTH]
	    || itemLF.actualBoundsInvalid[HEIGHT]
	    || itemLF.nativeId == INVALID_NATIVE_ID) {
		return;
	    }
	}

	if (itemLF.sizeChanged) {
	    itemLF.uCallSizeChanged(itemLF.bounds[WIDTH],
				    itemLF.bounds[HEIGHT]);
	    itemLF.sizeChanged = false;
	}

	// repaint only visible in viewport items
	if (itemLF.visibleInViewport) {
	    // CustomItem uses its own off screen graphics for painting
	    // and the rest of the items do not need to repaint
	    itemLF.uCallPaint(null,
			      itemLF.bounds[WIDTH], itemLF.bounds[HEIGHT]);
	}
    }


    /**
     * Paint an <code>Item</code> contained in this <code>Screen</code>.
     * The <code>Item</code> requests a paint in its own coordinate space.
     * <code>Screen</code> translates those coordinates into the overall
     * coordinate space and schedules the repaint
     *
     * @param item the <code>Item</code> requesting the repaint
     * @param x the x-coordinate of the origin of the dirty region
     * @param y the y-coordinate of the origin of the dirty region
     * @param w the width of the dirty region
     * @param h the height of the dirty region
     */
    void lRequestPaintItem(Item item, int x, int y, int w, int h) {

        ItemLFImpl iLF = (ItemLFImpl)item.getLF();

        lRequestPaint(iLF.bounds[X] + x, iLF.bounds[Y] + y, w, h, item);
    }

    /**
     * Create native resource for this <code>Form</code>.
     * <code>Item</code>s' resources are not created here.
     */
    void createNativeResource() {
	nativeId = createNativeResource0(owner.title,
					 owner.ticker == null ?  null
					    : owner.ticker.getString());
    }

    /**
     * Service method - returns the <code>ItemLFImpl</code> that has focus.
     *
     * @return the current <code>ItemLFImpl</code>, or <code>null</code>
     *         if there is no current.
     */
    ItemLFImpl getItemInFocus() {

	if (focusIndex < 0) {
	    return null;
	} else {
	    return itemLFs[focusIndex];
	}
    }

    // ***************************************************************

    /**
     * Scroll to show an <code>Item</code> and give focus to it if possible.
     *
     * @param nativeId native resource id of the <code>Form</code>
     * @param itemId native resource id for the focused <code>Item</code>
     * @param yOffset offset for the y co-ordinate of the
     *                focused <code>Item</code>
     */
    native void setCurrentItem0(int nativeId, int itemId, int yOffset);

    /**
     * Current Y position in a scrollable form.
     *
     * @return current scroll Y position
     */
    native int getScrollPosition0();

    /**
     * Create the native resource of this <code>Form</code>.
     *
     * @param title the title text of the <code>Form</code>
     * @param tickerText the text of the <code>Ticker</code>,
     *                   <code>Null</code> if no ticker.
     *
     * @return native resource id
     *
     * @exception OutOfMemoryException - if out of native resource
     */
    private native int createNativeResource0(String title, String tickerText);

    /**
     * Populate the native <code>Form</code> with visible <code>ItemLF</code>s
     * and then show.
     *
     * @param nativeId native resource id
     * @param modelVersion initial model version number for this visible period
     * @param w width of the virtual Form without scrolling
     * @param h height of the virtual Form without scrolling
     *
     * @exception OutOfMemoryException - if out of native resource
     */
    private native void showNativeResource0(int nativeId,
					    int modelVersion,
					    int w, int h);

    /**
     * Current viewport height in the native resource
     *
     * @return current viewport height
     */
    private native int getViewportHeight0();

    /**
     * Make sure all items have native resource and
     * all <code>CustomItem</code>s have their minimum and preferred sizes
     * cached.
     */
    private void uEnsureResourceAndRequestedSizes() {
	int i, count = 0;

	synchronized (Display.LCDUILock) {

	    // Make a temporary copy of ItemLFs we need to collect sizes from
	    ensureDispatchItemArray(numOfLFs);

	    // Make sure each Item has native resource
	    // and remember all the CustomItemLFImpls
	    for (i = 0; i < numOfLFs; i++) {
		if (itemLFs[i].nativeId == INVALID_NATIVE_ID) {
		    itemLFs[i].createNativeResource(super.nativeId);
		    // layout(UPDATE_LAYOUT) later will not call
		    // setSize/setLocation on an ItemLF that has valid bounds
		    // already. But the native resource is recreated
		    // above, we make up these two calls here.
		    itemLFs[i].initNativeResource();
		    // Every native resource is default to be visible in
		    // viewport. It's up to the native container to maintain
		    // viewport.
		    itemLFs[i].lShowNativeResource();
		}

		if (itemLFs[i] instanceof CustomItemLFImpl) {
		    // Remember this in temporary array
		    dispatchItemLFs[count++] = itemLFs[i];
		}
	    }
	} // synchronized

	// Collect min and preferred sizes from CustomItems
	// SYNC NOTE: This may call into app code like
	// CustomItem.getPrefContentWidth(). So do it outside LCDUILock
	for (i = 0; i < count; i++) {
	    ((CustomItemLFImpl)dispatchItemLFs[i]).uCallSizeRefresh();
	}

	// Dereference ItemLFImpl objects in dispatchItemLFs
	// But leave the shrinking to uCallHide
	resetDispatchItemArray(false);
    }

    /**
     * Show all items and give focus to current item.
     * SYNC NOTE: caller must NOT hold LCDUILock since this function may
     * call into app code like getPrefContentWidth(), sizeChanged or paint()
     * of <code>CustomItem</code>.
     */
    private void uShowContents() {
	if (Logging.REPORT_LEVEL <= Logging.INFORMATION) {
	    Logging.report(Logging.INFORMATION,
			   LogChannels.LC_HIGHUI_FORM_LAYOUT,
			   "\nFormLFImpl: showContents()");
	}

	// Ensure resources for all items and requested sizes for CustomItems
	uEnsureResourceAndRequestedSizes();

	// Layout
	synchronized (Display.LCDUILock) {

	    if (firstShown) {
		LayoutManager.instance().lLayout(LayoutManager.FULL_LAYOUT,
						 itemLFs,
						 numOfLFs,
						 width,
						 height,
						 viewable);
		firstShown = false;

	    } else {
		LayoutManager.instance().lLayout(LayoutManager.UPDATE_LAYOUT,
						 itemLFs,
						 numOfLFs,
						 width,
						 height,
						 viewable);
	    }

	    // Set native Form's window viewable size (logical Form size)
	    // and make it shown if not yet
	    showNativeResource0(nativeId, modelVersion, width,
				viewable[HEIGHT]);

	    // update viewport height
            viewportHeight = getViewportHeight0();

	} // synchronized

	// Set up current focused item and viewport
	uCallSetCurrentItem(true, CustomItem.NONE);

	// Set visibleInViewport flag for CustomItems in the
	// viewport and call showNotify()
	int pos = getScrollPosition0();
	uViewportChanged(pos, pos + viewportHeight);
    }

    /**
     * Setup focus and scroll viewport to show focused item.
     * @param forward true if forward direction is required,
     * false in case of backward direction
     * @param dir the traversal direction
     */
    private void uCallSetCurrentItem(boolean forward, int dir) {
        ItemLFImpl oldFocus, newFocus = null;
        
        synchronized (Display.LCDUILock) {
            oldFocus = getItemInFocus();
        } // synchronized
        
        int start = 0, count = 0;
        // Make a temporary copy of ItemLFs
        ensureDispatchItemArray(numOfLFs);
        System.arraycopy(itemLFs, 0, dispatchItemLFs, 0, numOfLFs);
        count = numOfLFs;
        
        if (focusIndex >= 0) {
            start = focusIndex;
        }
        for (int j, i = 0; i < count; ) {
            j = forward ?
                (start + i) % count :
                (start - i + count) % count;
            
            if (j != focusIndex && pendingCurrentItemLF != null) {
                newFocus = pendingCurrentItemLF;
                pendingCurrentItemLF = null;
                // break anyway, even if new item does not accept the focus
                i = count;
            } else {
                newFocus = dispatchItemLFs[j];
                i++;
            }
            
            if (newFocus != null &&
                (newFocus.uCallTraverse(dir, width, height, 
                                        getVisRect(newFocus)) || 
                 newFocus != oldFocus)) {
                break;
            }
            newFocus = null;
        }
        // Dereference ItemLFImpl objects in dispatchItemLFs
        // But leave the shrinking to uCallHide
        resetDispatchItemArray(false);
        
        // Update focus index and command set
        uFocusChanged(dir, newFocus, oldFocus);
        
        // IMPL_NOTE: we need to examine the returned visRect and
        // see if extra scrolling is needed
        
        if (newFocus != null && newFocus != oldFocus) {
            // Set up focus and scroll viewport to show new focus item
            synchronized (Display.LCDUILock) {
                // It's possible that this newFocus item has
                // been just removed from this Form since we
                // are outside LCDUILock. Check again.
                if (newFocus.nativeId != INVALID_NATIVE_ID) {
                    setCurrentItem0(nativeId, newFocus.nativeId, 0);
                }
            }
        }
    }
        
    /**
     * Update current item index and notify related item of the change.
     * Item specific abstract commands will also be shown.
     * This call will not setup scrolling.
     *
     * @param direction traversal direction
     * @param newFocus the new item in focus.
     * @param oldFocus the fallback item if new item refuses to take focus.
     * @return true if the internal traversal has been done false otherwise 
     */
    private boolean uFocusChanged(int direction,
                  ItemLFImpl newFocus, ItemLFImpl oldFocus) {
        boolean ret = false;
        if (newFocus != null && newFocus != oldFocus) {
            // itemLFs array may have changed. So re-map the focus index
            synchronized (Display.LCDUILock) {
                // Update item specific commands
                focusIndex = item2Index(newFocus); // Could be -1
                if (oldFocus != null) {
                    oldFocus.uCallTraverseOut();
                }
                updateCommandSet();
                ret = true;
            }
        }
        return ret;
    }

    /**
     * Calculate the rectangle representing the region of the item that is
     * currently visible. This region might have zero area if no part of the
     * item is visible, for example, if it is scrolled offscreen.
     * @param item item
     * @return  It must be an int[4] array. The information in this array is
     * a rectangle of the form [x,y,w,h]  where (x,y) is the location of the
     * upper-left corner of the rectangle relative to the item's origin, and
     * (w,h) are the width and height of the rectangle.
     */
    private int[] getVisRect(ItemLFImpl item) {
        int visRect[] = new int[4];
        synchronized (Display.LCDUILock) {
            // Initialize the in-out rect for traversal
            visRect[X] = 0;
            visRect[WIDTH] = width;

            // take the coordinates from the overall
            // coordinate space 

            int itemY1 = item.bounds[Y];
            int itemY2 = item.bounds[Y] + item.bounds[HEIGHT];

            // vpY1 the y coordinate of the top left visible pixel
            // current scroll position
            int vpY1 = getScrollPosition0();
            // vpY2 the y coordinate of bottom left pixel
            int vpY2 = vpY1 + viewportHeight;
                        
            // return only the visible region of item

            // item completely visible in viewport
            visRect[Y] = 0;
            visRect[HEIGHT] = item.bounds[HEIGHT];
                        
            if ((itemY1 >= vpY2) || (itemY2 <= vpY1)) { 
                // no part of the item is visible
                // so this region has zero area
                visRect[WIDTH] = 0;
                visRect[HEIGHT] = 0;
            } else {
                if (itemY1 < vpY1) {
                    // upper overlap
                    visRect[Y] =  vpY1 - itemY1;
                    visRect[HEIGHT] -= (vpY1 - itemY1);
                }
                if (itemY2 >= vpY2) {
                    // lower overlap
                    visRect[HEIGHT] -= (itemY2 - vpY2);
                }
            } 
        }
        return visRect;
    }
    
    /**
     * Called by the system to notify that viewport scroll location
     * or height has been changed.
     *
     * @param vpY1 the y coordinate of the top left visible pixel
     * @param vpY2 the y coordinate of bottom left pixel 
     *             immediately below the viewport
     */
    private void uViewportChanged(int vpY1, int vpY2) {
	
	int i, showCount, hideCount, size;
	
	synchronized (Display.LCDUILock) {

	    ensureDispatchItemArray(numOfLFs);
	    size = numOfLFs;
	       
	    showCount = 0;
	    hideCount = numOfLFs;
	       
	    for (i = 0; i < numOfLFs; i++) {
		if (itemLFs[i].bounds[Y] + 
		    itemLFs[i].bounds[HEIGHT]-1 > vpY1 &&
		    itemLFs[i].bounds[Y] < vpY2) { 
		    // should become visible
		    if (itemLFs[i].visibleInViewport == false) {
			dispatchItemLFs[showCount++] = itemLFs[i];
		    }
		} else { 
		    // should not be visible
		    if (itemLFs[i].visibleInViewport) {
			dispatchItemLFs[--hideCount] = itemLFs[i];
		    }
		}
	    }
	} // synchronized (LCDUILock)
	   
        for (i = 0; i < showCount; i++) {
	    dispatchItemLFs[i].uCallShowNotify();
        }
	   
	for (i = hideCount; i < size; i++) {
	    dispatchItemLFs[i].uCallHideNotify();
	}
	
	resetDispatchItemArray(true);
    }

    /**
     * Service method - find the <code>ItemLFImpl</code> from a given 
     * native id.
     *
     * @param nativeId native id to search
     *
     * @return the <code>ItemLFImpl</code>, or <code>null</code> not found
     */
    private ItemLFImpl id2Item(int nativeId) {

	ItemLFImpl focus = getItemInFocus();

	if (focus != null && focus.nativeId == nativeId) {
	    
	    return focus;
	    
	} else {

	    for (int i = 0; i < numOfLFs; i++) {
		if (itemLFs[i].nativeId == nativeId) {
		    return itemLFs[i];
		}
	    }

	    // there is no matching ItemLFImpl
	    return null;
	}
    }

    /**
     * Service method - find the <code>ItemLFImpl</code> index.
     *
     * @param itemLF itemLF to map
     *
     * @return index of the item. -1 if not found.
     */
    private int item2Index(ItemLFImpl itemLF) {

	for (int i = 0; i < numOfLFs; i++) {
	    if (itemLFs[i] == itemLF) {
		return i;
	    }
	}

	return -1;
    }


    /**
     * Ensure that dispatchItemLFs array has enough space for use.
     * SYNC NOTE: This function must only be used in event dispatch thread.
     *
     * @param size maximum number of itemLFs needed
     */
    private static void ensureDispatchItemArray(int size) {
	if (size > dispatchItemLFs.length) {
	    dispatchItemLFs = new ItemLFImpl[size];
	}
    }

    /**
     * Clear contents of dispatchItemLFs array after use.
     * SYNC NOTE: This function must only be used in event dispatch thread.
     *
     * @param alsoShrink true if the array size should be minimized
     */
    private static void resetDispatchItemArray(boolean alsoShrink) {

	if (alsoShrink && dispatchItemLFs.length > DISPATCH_ITEM_ARRAY_BLOCK) {
	    dispatchItemLFs = new ItemLFImpl[DISPATCH_ITEM_ARRAY_BLOCK];
	} else {
	    // Only clean up existing array contents
	    for (int i = 0; i < dispatchItemLFs.length; i++) {
		dispatchItemLFs[i] = null;
	    }
	}
    }


    /** 
     * A bit mask to capture the horizontal layout directive of an item.
     */
    final static int LAYOUT_HMASK = 0x03;

    /** 
     * A bit mask to capture the vertical layout directive of an item.
     */
    final static int LAYOUT_VMASK = 0x30;

    /** 
     * Do a full layout.
     */
    final static int FULL_LAYOUT = -1;

    /** 
     * Only update layout.
     */
    final static int UPDATE_LAYOUT = -2;

    /**
     * This is the rate at which the internal array of Items grows if
     * it gets filled up.
     */
    private static final int GROW_SIZE = 4;

    /**
     * Array of <code>ItemLF</code>s that correspond to the array of items 
     * in <code>Form</code>.
     */
    private ItemLFImpl[] itemLFs;

    /**
     * Block size of the temporary array of <code>ItemLF</code>s used 
     * in dispatch.
     */
    private final static int DISPATCH_ITEM_ARRAY_BLOCK = 10;

    /**
     * Temporary array of <code>ItemLF</code>s that is ONLY used in 
     * dispatch thread during show, hide and re-layout this <code>Form</code>.
     *
     * ensureDispatchItemArray() should be called before use and
     * resetDispatchItemArray() should be called when it is no longer needed,
     * to allow <code>ItemLFImpl</code> objects been GC'ed.
     */
    private static ItemLFImpl[] dispatchItemLFs =
		new ItemLFImpl[DISPATCH_ITEM_ARRAY_BLOCK];

    /**
     * The number of views present in this <code>FormLF</code>.
     */
    private int numOfLFs;
    
    /**
     * This helps an optimization.
     */
    private boolean firstShown = true;


    /**
     * Viewport height in the native resource
     */
    private int viewportHeight; // = 0;

    /** 
     * Overall dimensions of the view. It is an array so it could be passed 
     * as a reference to <code>LayoutManager</code>.
     */
    int viewable[] = new int[4];

    /**
     * This holds the index of the current item in focus.
     */
    int focusIndex = -1;

    /**
     * ItemLF pending to become the current focused item.
     * It is set in lSetCurrentItem() upon API call Display.setCurrentItem().
     * During show or invalidate, the focus will be transfer from current item
     * to this pending item and this variable will be cleared afterwards.
     */
    ItemLFImpl pendingCurrentItemLF;

    /**
     * Left to right layout is default.
     * Used by isImplicitLineBreak.
     */
    final static boolean ltr = true; 

} // class FormLFImpl
