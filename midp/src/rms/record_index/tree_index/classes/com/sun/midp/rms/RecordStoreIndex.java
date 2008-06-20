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
 */

package com.sun.midp.rms;

import java.io.IOException;

import javax.microedition.rms.*;

import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

/**
 * A class implementing a index of the record store.
 *
 *  Methods used by the RecordStoreImpl
 *      close()
 *      deleteIndex()
 *      getRecordIDs()
 *      getRecordHeader()
 *      getFreeBlock()
 *      updateBlock()
 *      deleteRecordIndex()
 *      removeBlock()
 *
 */

class RecordStoreIndex {
    /*
     * The layout of the database file is as follows:
     *
     * Bytes - Usage
     * 00-03 - Size of index file (big endian)
     * 04-07 - Offset to recordId tree root (big endian)
     * 08-11 - Offset to free block tree root (big endian)
     * 12-15 - Offset to the list of free tree blocks (big endian)
     * 16-xx - Tree Blocks
     */

    /** IDX_SIZE offset */
    static final int IDX0_SIZE = 0;

    /** IDX_ID_ROOT offset */
    static final int IDX1_ID_ROOT = 4;

    /** IDX_FREE_ROOT offset */
    static final int IDX2_FREE_BLOCK_ROOT = 8;

    /** IDX_FREE_NODES offset */
    static final int IDX3_FREE_NODE_HEAD = 12;

    /** Size of the index header */
    static final int IDX_HEADER_SIZE = 16;

    /** The maximum number of data elements in each  node */
    static final int NODE_ELEMENTS = 8;

    /** The size of the tree blocks */
    static final int NODE_SIZE = 4 + (NODE_ELEMENTS * (4 + 4 + 4));

    /** The Record Store that this object indexes */
    private AbstractRecordStoreImpl recordStore;

    /** The Record Store database file */
    private AbstractRecordStoreFile dbFile;

    /** The Record Store database index file */
    private AbstractRecordStoreFile idxFile;

    /** The header of the index file */
    private byte[] idxHeader = new byte[IDX_HEADER_SIZE];

    /** The node buffer for initializing nodes */
    private byte[] nodeBuf = new byte[NODE_SIZE];

    /**
     * Constructor for creating an index object for the given Record Store.
     *
     * @param rs record store that this object indexes
     * @param suiteId unique ID of the suite that owns the store
     * @param recordStoreName a string to name the record store
     *
     * @exception IOException if there are any file errors
     */
    RecordStoreIndex(AbstractRecordStoreImpl rs, int suiteId,
                     String recordStoreName) throws IOException {
        recordStore = rs;
        if (rs != null) {
            dbFile = rs.getDbFile();
        }

        boolean exist =
          RecordStoreUtil.exists(suiteId, recordStoreName,
                                 AbstractRecordStoreFile.IDX_EXTENSION);

        idxFile = rs.createIndexFile(suiteId, recordStoreName);

        if (exist) {
            // load header
            if (idxFile.read(idxHeader) != IDX_HEADER_SIZE) {
               throw new IOException("Index file corrupted");
            }
        } else {
            RecordStoreUtil.putInt(IDX_HEADER_SIZE + NODE_SIZE * 2,
                                   idxHeader, IDX0_SIZE);
            RecordStoreUtil.putInt(IDX_HEADER_SIZE, idxHeader, IDX1_ID_ROOT);
            RecordStoreUtil.putInt(IDX_HEADER_SIZE + NODE_SIZE,
                                   idxHeader, IDX2_FREE_BLOCK_ROOT);
            idxFile.write(idxHeader);
            idxFile.write(nodeBuf);
            idxFile.write(nodeBuf);
            idxFile.commitWrite();
        }
    }

    /**
     * Closes the index file.
     *
     * @exception IOException if there are any file errors
     */
    void close() throws IOException {
        idxFile.close();
    }

    /**
     * Deletes index files of the named record store. MIDlet suites are
     * only allowed to delete their own record stores.
     *
     * @param suiteId ID of the MIDlet suite that owns the record store
     * @param recordStoreName the MIDlet suite unique record store to
     *          delete
     * @return <code>true</code> if file was found and deleted successfully,
     *         <code>false</code> otherwise.
     */
    static boolean deleteIndex(int suiteId, String recordStoreName) {
        return RecordStoreUtil.quietDeleteFile(suiteId,
                                   recordStoreName,
                                   AbstractRecordStoreFile.IDX_EXTENSION);
    }

    /**
     * Returns all of the recordId's currently in the record store index.
     *
     * @return an array of the recordId's currently in the index.
     */
    int[] getRecordIDs() {
        int count = recordStore.getNumRecords();

        int[] recordIdList = new int[count];
        getRecordIds(recordIdList);

        return recordIdList;
    }

    /**
     * Returns places all of the recordId's in the index.
     * If the array is not big enough, the recordId list will be
     * limited to the size of the given array.
     *
     * @param recordIdList array to place the recordId's
     *
     * @return the number of recordId's placed in the array.
     */
    int getRecordIds(int[] recordIdList) {
        int count = 0;

        try {
            Node node = new Node(idxFile);
            node.load(getRecordIdRootOffset());

            count = walk(node, recordIdList, 0);
        } catch (IOException e) {
            if (Logging.REPORT_LEVEL <= Logging.ERROR) {
                Logging.report(Logging.ERROR, LogChannels.LC_RMS,
                               "Could not walk the tree");
            }
        }

        return count;
    }

    /**
     *  Finds the record header for the given record and returns the
     *  offset to the header.
     *
     * @param recordId the ID of the record to use in this operation
     * @param header the header of the block to free
     *
     * @exception IOException if there is an error accessing the db file
     * @exception InvalidRecordIDException if the recordId is invalid
     *
     * @return the offset in the db file of the block added
     */
    int getRecordHeader(int recordId, byte[] header)
        throws IOException, InvalidRecordIDException {

        if (recordId <= 0) {
            throw new InvalidRecordIDException("error finding record data");
        }

        int loc_offset = getBlockOffsetOfRecord(recordId);

        if (loc_offset == 0) {
            // did not find the recordId
            throw new InvalidRecordIDException();
        }

        // read the header
        dbFile.seek(loc_offset);

        // read the block header
        if (dbFile.read(header) != AbstractRecordStoreImpl.BLOCK_HEADER_SIZE) {
            // did not find the recordId
            throw new InvalidRecordIDException();
        }

        return loc_offset;
    }

    /**
     *  Returns the offset to the header for the given recordId
     *
     * @param recordId the ID of the record to use in this operation
     *
     * @exception IOException if there is an error accessing the db file
     * @exception InvalidRecordIDException if the recordId is invalid
     *
     * @return the offset in the db file of the record block
     */
    int getBlockOffsetOfRecord(int recordId)
        throws IOException, InvalidRecordIDException {

        Node node = new Node(idxFile);
        node.load(getRecordIdRootOffset());

        int loc_offset = getKeyValue(node, recordId);

        if (loc_offset == 0) {
            // did not find the recordId
            throw new InvalidRecordIDException();
        }

        return loc_offset;
    }

    /**
     * Updates the index of the given block and its offset.
     *
     * @param blockOffset the offset in db file to the block to update
     * @param header the header of the block to update
     *
     * @exception IOException if there is an error accessing the index file
     */
    void updateBlock(int blockOffset, byte[] header) throws IOException {
        int recordId = RecordStoreUtil.getInt(header, 0);

        if (recordId > 0) {
            updateRecordId(recordId, blockOffset);
        }
    }

    /**
     * Updates the given recordId with the given offset. Adds the
     * recordId if it did not already exist.
     *
     * @param recordId the id of the record
     * @param blockOffset the offset in db file to the block to update
     *
     * @exception IOException if there is an error accessing the index file
     */
    void updateRecordId(int recordId, int blockOffset) throws IOException {
        Node node = new Node(idxFile);
        node.load(getRecordIdRootOffset());

        // update the key
        int newOffset = updateKey(node, recordId, blockOffset);

        // check if a new root node was added
        if (newOffset > 0) {
            // save the new root node offset
            setRecordIdRootOffset(newOffset);
        }
    }

    /**
     * The record is deleted from the record store index.
     *
     * @param recordId the ID of the record index to delete
     *
     * @exception IOException if there is an error accessing the db index
     */
    void deleteRecordIndex(int recordId) throws IOException {
        int rootOffset = getRecordIdRootOffset();
        Node node = new Node(idxFile);
        node.load(rootOffset);

        // find the key's node
        int loc_offset = deleteKey(node, recordId);

        // check if the offset of a new root was returned
        if (loc_offset > 0) {
            // new root, free old one
            freeNode(rootOffset);

            // save the new root
            setRecordIdRootOffset(loc_offset);
        }
    }

    /**
     * Searches for a free block large enough for the record.
     *
     * @param header a block header with the size set to the record data size
     *
     * @exception IOException if there is an error accessing the db file
     *
     * @return the offset in the db file of the block added
     */
    int getFreeBlock(byte[] header) throws IOException {
        int targetSize = RecordStoreUtil.
            calculateBlockSize(RecordStoreUtil.getInt(header, 4));
        int currentId = 0;
        int currentOffset = AbstractRecordStoreImpl.DB_HEADER_SIZE;
        int currentSize = 0;

        // search through the data blocks for a free block that is large enough
        while (currentOffset < recordStore.getSize()) {
            // seek to the next offset
            dbFile.seek(currentOffset);

            // read the block header
            if (dbFile.read(header) != AbstractRecordStoreImpl.BLOCK_HEADER_SIZE) {
                // did not find the recordId
                throw new IOException();
            }

            currentId = RecordStoreUtil.getInt(header, 0);
            currentSize = RecordStoreUtil.
                calculateBlockSize(RecordStoreUtil.getInt(header, 4));

            // check for a free block big enough to hold the data
            if (currentId < 0 && currentSize >= targetSize) {
                // a free block
                return currentOffset;
            }

            // added the block size to the currentOffset
            currentOffset += currentSize;
        }

        return 0;
    }

    /**
     * Removes the given block from the list of free blocks.
     *
     * @param blockOffset the offset in db file to the block to remove
     * @param header the header of the block to remove
     *
     * @exception IOException if there is an error accessing the db file
     */
    void removeBlock(int blockOffset, byte[] header) throws IOException {
    }


    /**
     *  Getter/Setter for header info
     */

    /**
     * Gets the offset to the root of the recordId tree.
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the offset of the recordId tree root
     */
    int getRecordIdRootOffset() throws IOException {
        return RecordStoreUtil.getInt(idxHeader, IDX1_ID_ROOT);
    }

    /**
     * Sets the offset to the root of the recordId tree.
     *
     * @param newOffset the new root offset
     *
     * @exception IOException if there is an error accessing the index file
     */
    void setRecordIdRootOffset(int newOffset) throws IOException {
        RecordStoreUtil.putInt(newOffset, idxHeader, IDX1_ID_ROOT);
        idxFile.seek(0);
        idxFile.write(idxHeader);
        idxFile.commitWrite();
    }

    /**
     * Gets the offset to the root of the free block tree.
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the offset of the free block tree
     */
    int getFreeBlockRootOffset() throws IOException {
        return RecordStoreUtil.getInt(idxHeader, IDX2_FREE_BLOCK_ROOT);
    }

    /**
     * Sets the offset to the root of the free block tree.
     *
     * @param newOffset the new root offset
     *
     * @exception IOException if there is an error accessing the index file
     */
    void setFreeBlockRootOffset(int newOffset) throws IOException {
        RecordStoreUtil.putInt(newOffset, idxHeader, IDX2_FREE_BLOCK_ROOT);
        idxFile.seek(0);
        idxFile.write(idxHeader);
        idxFile.commitWrite();
    }

    /**
     *  node allocation and free management
     */

    /**
     * Returns the offset to a free node if one exists. Otherwise,
     * returns the offset of a newly create node.
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the offset the new node in the index file
     */
    private int allocateNode() throws IOException {
        // check if there is a free node to use
        int loc_offset = RecordStoreUtil.getInt(idxHeader, IDX3_FREE_NODE_HEAD);

        if (loc_offset == 0) {
            // no free blocks, add one to the end of the index file
            loc_offset = RecordStoreUtil.getInt(idxHeader, IDX0_SIZE);
            RecordStoreUtil.putInt(loc_offset + NODE_SIZE,
	                           idxHeader, IDX0_SIZE);
        } else {
            idxFile.seek(loc_offset);
            idxFile.read(idxHeader, IDX3_FREE_NODE_HEAD, 4);
        }
        idxFile.seek(0);
        idxFile.write(idxHeader);
        idxFile.commitWrite();

        return loc_offset;
    }

    /**
     * Adds the node to the list of free nodes.
     *
     * @param inp_offset the offset of the node to free
     *
     * @exception IOException if there is an error accessing the index file
     */
    private void freeNode(int inp_offset) throws IOException {
        idxFile.seek(inp_offset);
        idxFile.write(idxHeader, IDX3_FREE_NODE_HEAD, 4);

        RecordStoreUtil.putInt(inp_offset, idxHeader, IDX3_FREE_NODE_HEAD);
        idxFile.seek(0);
        idxFile.write(idxHeader);
        idxFile.commitWrite();
    }


    /**
     *  Tree management
     */

    /**
     * Walks the tree starting at the given node and loads all of the tree's
     * keys into the given array.
     *
     * @param node the root node of the tree to walk
     * @param keyList array to fill with the tree's keys
     * @param count must be 0 for user call, other value for recursive call
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the number of keys placed into the array.
     */
    int walk(Node node, int[] keyList, int count) throws IOException {
        // number of keys in the key list
	int i;
	if (count > keyList.length - 1) { // array is filled
	    return keyList.length;
	}
	for (i = 0; i < NODE_ELEMENTS + 1 &&
                    count < keyList.length; i++) {
	    if (node.child[i] > 0) { // subtree
                // load the node
	        int parent_offset = node.offset; // save the parent's offset
                node.load(node.child[i]);
	        count = walk(node, keyList, count); // walk along subtree
                node.load(parent_offset);     // the parent node
	    }
	    if (node.key[i] > 0 && count < keyList.length) {
	        // add the current key
                keyList[count++] = node.key[i];
	    } else { // no more keys - stop walking
	        break;
	    }
	}
        return count;
    }

    /**
     * Searches the tree starting at the given node for the given key and
     * returns the value associated with the key
     *
     * @param node the root node of the tree to search for the key
     * @param key the search key
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the value for the key or 0 if the key was not found
     */
    int getKeyValue(Node node, int key) throws IOException {
        // find the node that contains the key
        int index = findNodeWithKey(node, key);

        if (node.key[index] == key) {
            return node.value[index];
        }

        return 0;
    }

    /**
     * Updates the tree starting with the given node with the key value pair.
     * If the key is already in the tree, the value is updated.  If the key
     * is not in the tree, it is inserted.  If the insertion causes the root
     * node to be split, the offset to the new root is returned, otherwise 0
     * is returned.
     *
     * @param node the root node of the tree to update the key with
     * @param key the key to update
     * @param value the new value
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the offset of the new tree root if one was added, 0 otherwise
     */
    int updateKey(Node node,
                  int key, int value) throws IOException {
        // find the node that contains the key
        int index = findNodeWithKey(node, key);

        if (node.key[index] == key) {
            // key is already in the tree, update it
            node.value[index] = value;
            node.save();

            return 0;
        }

        // add the key to the node
        Node newNode = new Node(idxFile);
        int rightChild = 0;

        while (key > 0) {
            // add new triplet at index
            node.addKey(key, value, rightChild, index);

            // check if node has overflowed
            if (node.key[NODE_ELEMENTS] == 0) {
                node.save();
                break;
            }

            // node overflowed, split node
            newNode.init(allocateNode());

            // save the midpoint value
            key = node.key[NODE_ELEMENTS/2];
            value = node.value[NODE_ELEMENTS/2];
            rightChild = node.child[NODE_ELEMENTS/2 + 1];

            // clear midpoint
            node.key[NODE_ELEMENTS/2] = 0;
            node.value[NODE_ELEMENTS/2] = 0;
            node.child[NODE_ELEMENTS/2 + 1] = 0;

            // copy the top half of original node to new node
            int j = 0;
            newNode.child[0] = rightChild;
            for (int i = NODE_ELEMENTS/2 + 1; i < NODE_ELEMENTS+1; i++, j++) {
                newNode.key[j] = node.key[i];
                newNode.value[j] = node.value[i];
                newNode.child[j + 1] = node.child[i + 1];

                node.key[i] = 0;
                node.value[i] = 0;
                node.child[i + 1] = 0;
            }
            node.numKeys = NODE_ELEMENTS/2;
            newNode.numKeys = j;
            rightChild = newNode.offset;
            int leftChild = node.offset;

            // save both nodes
            node.save();
            newNode.save();

            // load the parent of the node
            int parentOffset = node.popParent();
            if (parentOffset == 0) {
                // need to add a new root to tree
                int newRoot = allocateNode();

                node.init(newRoot);
                node.key[0] = key;
                node.value[0] = value;
                node.child[0] = leftChild;
                node.child[1] = rightChild;
                node.save();

                // done with split
                return newRoot;
            }

            // load the parent
            node.load(parentOffset);

            // find position to insert the midpoint of split
            for (index = 0; index < NODE_ELEMENTS &&
                 node.child[index] != leftChild; index++);
        }

        return 0;
    }

    /**
     * Searches the tree starting with the given node for the given key. If
     * the key is in the tree, the key value pair is deleted.  If the key
     * is not in the tree, nothing happens.  If the deletion causes the root
     * node to be merged, the offset to the new root is returned, otherwise 0
     * is returned.
     *
     * @param node the root node of the tree to remove key from
     * @param key the key to remove
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the offset of the new tree root if the old one was removed,
     *         otherwise 0
     */
    int deleteKey(Node node, int key) throws IOException {
        // find the key's node
        int index = findNodeWithKey(node, key);

        // check if key was found
        if (node.key[index] == key) {
            return deleteKeyFromNode(node, index);
        }

        return 0;
    }

    /**
     * Deleted the key value pair at the given index from the given node. If
     * the deletion causes the root node to be merged, the offset to the new
     * root is returned, otherwise 0 is returned.
     *
     * @param node the root node of the tree to remove key from
     * @param index the index to the key to remove
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the offset of the new tree root if the old one was removed,
     *         otherwise 0
     */
    private int deleteKeyFromNode(Node node,
                                  int index) throws IOException {
        // check if this node has right child
        if (node.child[index+1] > 0) {
            // find the least key in the subtree
            Node rootNode = node;
            node = new Node(idxFile);
            node.load(rootNode.child[index+1]);
            node.copyStack(rootNode);
            node.pushParent(rootNode.offset);

            while (node.child[0] > 0) {
                node.pushParent(node.offset);
                node.load(node.child[0]);
            }

            // replace delete key with discovered key
            rootNode.key[index] = node.key[0];
            rootNode.value[index] = node.value[0];
            rootNode.save();

            // now delete discovered key
            index = 0;

            // preserve right subtree from deleting
            node.child[0] = node.child[1];
        }

        // the node has no right child for the key, remove the key
        node.deleteKey(index);
        node.save();

        // get the parent offset
        int parentOffset = node.popParent();

        // rebalance tree as needed
        while (parentOffset > 0) {
            // check if node needs to be merged with sibling
            if (node.numKeys >= NODE_ELEMENTS/2) {
                // this node has at least the minimum number of keys
                node.load(parentOffset);
                parentOffset = node.popParent();
                continue;
            }

            // load the parent of this node
            Node parentNode = new Node(idxFile);
            parentNode.load(parentOffset);

            // find the offset of the node in the parent
            int childIdx = 0;
            for (; parentNode.child[childIdx] != node.offset &&
                 childIdx < NODE_ELEMENTS+1; childIdx++);

            int midpointIdx = childIdx;
            Node siblingNode = null;

            // try loading the left sibling
            if (childIdx-1 >= 0) {
                // load the left sibling
                siblingNode = new Node(idxFile);
                siblingNode.load(parentNode.child[childIdx-1]);
                midpointIdx = childIdx-1;
            }

            // check if this is a merge candidate
            if (siblingNode == null ||
                node.numKeys + siblingNode.numKeys + 1 > NODE_ELEMENTS) {
                if (siblingNode == null) {
                    siblingNode = new Node(idxFile);
                }

                // not a merge candidate, check the right sibling
                if (childIdx+1 < NODE_ELEMENTS+1 &&
                    parentNode.child[childIdx+1] > 0) {
                    // load the right sibling
                    siblingNode.load(parentNode.child[childIdx+1]);
                    midpointIdx = childIdx;
                }
            }

            // check if a merge is needed
            if (node.numKeys + siblingNode.numKeys + 1 <= NODE_ELEMENTS) {
                // merge the siblings
                Node leftNode, rightNode;
                if (childIdx == midpointIdx) {
                    leftNode = node;
                    rightNode = siblingNode;
                } else {
                    leftNode = siblingNode;
                    rightNode = node;
                }
                leftNode.addKey(parentNode.key[midpointIdx],
                                parentNode.value[midpointIdx],
                                rightNode.child[0],
                                leftNode.numKeys);

                for (int i = 0; i < rightNode.numKeys; i++) {
                    leftNode.addKey(rightNode.key[i],
                                    rightNode.value[i],
                                    rightNode.child[i+1],
                                    leftNode.numKeys);
                }

                leftNode.save();
                freeNode(rightNode.offset);

                parentNode.deleteKey(midpointIdx);
                if (parentNode.numKeys > 0) {
                    parentNode.save();
                } else {
                    // parentNode is empty
                    parentOffset = node.popParent();
                    if (parentOffset == 0) {
                        // the parent node is the root
                        return leftNode.offset;
                    } else {
                        int tempOffset = parentNode.offset;
                        freeNode(parentNode.offset);
                        parentNode.load(parentOffset);

                        // find the parentOffset
                        for (int x = 0; x < NODE_ELEMENTS+1; x++) {
                             if (parentNode.child[x] == tempOffset) {
                                 parentNode.child[x] = leftNode.offset;
                                 break;
                             }
                        }
                    }
                }
            } else {
                // could not merge the siblings
                // make sure each siblings has a minimum number of nodes
                if (midpointIdx == childIdx) {
                    // node is on the left, sibling is on the right
                    node.addKey(parentNode.key[midpointIdx],
                                parentNode.value[midpointIdx],
                                siblingNode.child[0],
                                node.numKeys);

                    parentNode.key[midpointIdx] = siblingNode.key[0];
                    parentNode.value[midpointIdx] = siblingNode.value[0];

                    siblingNode.child[0] = siblingNode.child[1];
                    siblingNode.deleteKey(0);
                } else {
                    // sibling is on the left, node is on the right
                    node.addKey(parentNode.key[midpointIdx],
                                parentNode.value[midpointIdx],
                                node.child[0],
                                0);

                    int tempIdx = siblingNode.numKeys;
                    node.child[0] = siblingNode.child[tempIdx];
                    parentNode.key[midpointIdx] = siblingNode.key[tempIdx-1];
                    parentNode.value[midpointIdx] =
                        siblingNode.value[tempIdx-1];
                    siblingNode.deleteKey(tempIdx-1);
                }

                siblingNode.save();
                parentNode.save();
                node.save();

                // check if the sibling lost too many keys
                if (siblingNode.numKeys < NODE_ELEMENTS/2) {
                    // make the sibling the merge candidate
                    siblingNode.copyStack(node);
                    node = siblingNode;
                    continue;
                }
            }

            // done with this node, move up the tree
            parentNode.copyStack(node);
            node = parentNode;
            parentOffset = node.popParent();
        }

        return 0;
    }

    /**
     * Searches the tree starting with the given node for the given key.  The
     * node that contains the key or the node where the key belongs is loaded
     * into the given node object then the method returns.  If the key is in
     * the tree, the index of the key is returned.  If the key is not in the
     * tree, the index where the key should be inserted is returned.
     *
     * @param node the root node of the tree to search for the key
     * @param key the key to search for
     *
     * @exception IOException if there is an error accessing the index file
     *
     * @return the index of the key or where the key belongs in the node
     */
    private int findNodeWithKey(Node node,
                                int key) throws IOException {
        // find node with given key
        int i = 0;
        while (i < NODE_ELEMENTS+1) {
            if (node.key[i] <= 0) {
                // reached end of keys
                // check if right child of last element exist
                if (node.child[i] > 0) {
                    // load the node
                    node.pushParent(node.offset);
                    node.load(node.child[i]);

                    // restart the loop for this node
                    i = 0;
                } else {
                    // key is not in tree, but belongs in this node
                    return i;
                }
            } else if (key == node.key[i]) {
                // found the key
                return i;
            } else if (key < node.key[i]) {
                // check for child tree
                if (node.child[i] > 0) {
                    // load the node
                    node.pushParent(node.offset);
                    node.load(node.child[i]);

                    // restart the loop for this node
                    i = 0;
                } else {
                    // key is not in tree, but belongs in this node
                    return i;
                }
            } else {
                i++;
            }
        }

        // belongs at the end of the current node
        return i;
    }

    /** Maximum depth of the parent node stack */
    private int maxStackDepth = 3;

    /**
     * Abstraction of a tree node
     */
    class Node {
        /** file that contains the tree, the index file */
        AbstractRecordStoreFile treeFile;

        /** number of keys in this node */
        int numKeys;

        /** offset of this node in the tree file */
        int offset;

        /** keys in this node */
        int[] key = new int[NODE_ELEMENTS+1];

        /** values in this node */
        int[] value = new int[NODE_ELEMENTS+1];

        /** children in this node */
        int[] child = new int[NODE_ELEMENTS+2];

        /** stack of parents of this node */
        int[] parentStack = new int[maxStackDepth];

        /** depth of the parent stack */
        int stackDepth = 0;

        /**
         * Constructor for creating a node in the tree.
         *
         * @param file the index file that contains this node
         */
        Node(AbstractRecordStoreFile file) {
            treeFile = file;
        }

        /**
         * Adds the given offset to the parent stack. Grows the stack as needed.
         *
         * @param inp_offset the offset value to push on top of the stack
         */
        void pushParent(int inp_offset) {
            if (stackDepth == parentStack.length) {
                maxStackDepth++;
                int[] newStack = new int[maxStackDepth];
                // copy old stack
                for (int i = 0; i < stackDepth; i++) {
                    newStack[i] = parentStack[i];
                }
                parentStack = newStack;
                newStack = null;
            }

            parentStack[stackDepth++] = inp_offset;
        }

        /**
         * Removes the top value of the stack and returns it.
         *
         * @return the top value of the stack or 0 if the stack is empty
         */
        int popParent() {
            if (stackDepth > 0) {
                return parentStack[--stackDepth];
            }

            return 0;
        }

        /**
         * Copies the stack of the given node to this node.
         *
         * @param fromNode the node whose stack is to be copied
         */
        void copyStack(Node fromNode) {
            stackDepth = 0;
            for (int i = 0; i < fromNode.stackDepth; i++) {
                pushParent(fromNode.parentStack[i]);
            }
        }

        /**
         * Initialize this node with given offset.  Clear all key, value,
         * and child values.
         *
         * @param inp_offset the new offset of the node data
	 * this node represents
         */
        void init(int inp_offset) {
            offset = inp_offset;

            // initialize all the node members
            numKeys = 0;
            child[0] = 0;
            for (int i = 0; i < NODE_ELEMENTS+1; i++) {
                key[i] = 0;
                value[i] = 0;
                child[i+1] = 0;
            }
        }

        /**
         * Adds the key, value, child values to the node at the given index.
         *
         * @param newKey the new key to add
         * @param newValue the new value to add
         * @param rightChild the new right child of the new key
         * @param index the index at which to add the key, value, child values
         */
        void addKey(int newKey, int newValue, int rightChild, int index) {
            for (int i = numKeys; i > index; i--) {
                key[i] = key[i-1];
                value[i] = value[i-1];
                child[i+1] = child[i];
            }

            // add new triplet
            key[index] = newKey;
            value[index] = newValue;
            child[index+1] = rightChild;

            numKeys++;
        }

        /**
         * Deletes the key, value, right child values from the node
         * at the given index.
         *
         * @param index the index at which to remove the key, value,
         *              child values
         */
        void deleteKey(int index) {
            for (int i = index; i < numKeys; i++) {
                key[i] = key[i+1];
                value[i] = value[i+1];
                child[i+1] = child[i+2];
            }

            numKeys--;
        }

        /**
         * Loads the node data at the given offset in the tree file into
         * this node object.
         *
         * @param inp_offset the offset to the node data to load
         *
         * @exception IOException if there is an error accessing the tree file
         */
        void load(int inp_offset) throws IOException {
            offset = inp_offset;
            numKeys = 0;

            // seek to the start of the node
            treeFile.seek(inp_offset);

            byte[] buffer = new byte[12];

            // read the first child
            if (treeFile.read(buffer, 0, 4) != 4) {
                throw new IOException("Could not read first child " +
                                      inp_offset);
            }
            child[0] = RecordStoreUtil.getInt(buffer, 0);

            // read the key, value, child triplets
            int i = 0;
            for (; i < NODE_ELEMENTS; i++) {
                if (treeFile.read(buffer) != buffer.length) {
                    throw new IOException("Could not read entire buffer " +
                                          inp_offset);
                }

                int tempKey = RecordStoreUtil.getInt(buffer, 0);
                if (tempKey <= 0) {
                    break;
                }

                numKeys++;
                key[i] = tempKey;
                value[i] = RecordStoreUtil.getInt(buffer, 4);
                child[i+1] = RecordStoreUtil.getInt(buffer, 8);
            }

            // clear the rest of the entries
            for (; i < NODE_ELEMENTS+1; i++) {
                key[i] = 0;
                value[i] = 0;
                child[i+1] = 0;
            }
        }

        /**
         * Saves the node data in this node object to the given offset in the
         * tree file.
         *
         * @exception IOException if there is an error accessing the tree file
         */
        void save() throws IOException {
            // seek to the start of the node
            treeFile.seek(offset);

            byte[] buffer = new byte[12];

            // write the left most child
            RecordStoreUtil.putInt(child[0], buffer, 0);
            treeFile.write(buffer, 0, 4);

            // write the key, value, child triplets
            for (int i = 0; i < NODE_ELEMENTS; i++) {
                RecordStoreUtil.putInt(key[i], buffer, 0);
                RecordStoreUtil.putInt(value[i], buffer, 4);
                RecordStoreUtil.putInt(child[i+1], buffer, 8);
                treeFile.write(buffer);
            }

            treeFile.commitWrite();
        }

        /**
         * Returns a string representation of this node object
         *
         * @return the string representation of the node
         */
        public String toString() {
            String temp = "offset=" + offset + "\n" + child[0] + "\n";
            for (int i = 0; i < NODE_ELEMENTS+1; i++) {
                temp += i + " " + key[i] + " " +
                    value[i] + " " + child[i+1] + "\n";
            }

            return temp;
        }
    }
}
