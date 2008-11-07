package com.sun.midp.appmanager;

import java.util.Vector;

/**
 *  Interfase represente the set of pairs of ID and label.
 *  One entity id market as selected.
 *  It can be used to represent the data for exclusive
 *  option buttons where each option has an ID.
 */
interface ValueChoice {

    /**
     * Returns the ID of the selected item.
     *
     * @return ID of selected element
     */
    int getSelectedID();

    /**
     * Returns ID of specified item.
     * @param index item index
     * @return item ID
     */
    int getID(int index);

    /**
     * Returns label of specified choice items.
     * @param index item index
     * @return label
     */
    String getLabel(int index);

    /**
     * Returns count of items
     * @return count
     */
    int getCount();

    /**
     * Returns choice title.
     * @return title
     */
    String getTitle();
}
