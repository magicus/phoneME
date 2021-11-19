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

package com.sun.j2me.rms;


import java.lang.reflect.Method;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Proxy;

/**
 * A class representing a record store. 
 */

public class RecordStore {

    private Object obj;  /* javax.microedition.rms.RecordStore object */

    // preloaded Class object for javax.microedition.rms.RecordStore
    private static Class clazz;
    private static Class recordEnumerationClass;    

    // preloaded Method objects for the methods in javax.microedition.rms.RecordStore
    private static Method openRecordStoreMethod;
    private static Method enumerateRecordsMethod;
    private static Method getRecordMethod;

    // preloaded Class objects for exceptions in javax.microedition.rms.*
    static Class recordStoreException;
    static Class recordStoreFullException;
    static Class recordStoreNotFoundException;
    static Class recordStoreNotOpenException;
    static Class invalidRecordIDException;

    static {
	    try {
            ClassLoader classLoader = sun.misc.MIDPConfig.getMIDPImplementationClassLoader();
            if (classLoader == null) {
                throw new RuntimeException("Cannot get ClassLoader");
            }
            
            /* Classes */
            clazz = Class.forName("javax.microedition.rms.RecordStore", true, classLoader);
            recordEnumerationClass = Class.forName("javax.microedition.rms.RecordEnumeration", true, classLoader);

            /* Exceptions */
            recordStoreException = Class.forName("javax.microedition.rms.RecordStoreException", true, classLoader);
            recordStoreFullException = Class.forName("javax.microedition.rms.RecordStoreFullException", true, classLoader);
            recordStoreNotFoundException = Class.forName("javax.microedition.rms.RecordStoreNotFoundException", true, classLoader);
            recordStoreNotOpenException = Class.forName("javax.microedition.rms.RecordStoreNotOpenException", true, classLoader);
            invalidRecordIDException = Class.forName("javax.microedition.rms.InvalidRecordIDException", true, classLoader);

            /* Methods */
            openRecordStoreMethod = clazz.getMethod("openRecordStore", new Class[] { String.class, boolean.class });

            Class recordFilter = Class.forName("javax.microedition.rms.RecordFilter", true, classLoader);
            Class recordComparator = Class.forName("javax.microedition.rms.RecordComparator", true, classLoader);
            enumerateRecordsMethod = clazz.getMethod("enumerateRecords", new Class[] { recordFilter,
                 recordComparator, boolean.class });

            getRecordMethod = clazz.getMethod("getRecord", new Class[] { int.class });
        } catch (ClassNotFoundException cnfe) {
            throw new Error("Record store is not available" + cnfe.getMessage());
        } catch (NoSuchMethodException nsme) {
	        throw new Error("Record store is not available");
	    } 

    }
    
    public RecordStore(Object rs) { 
        obj = rs;        
    }

    /**
     * Open (and possibly create) a record store associated with the
     * given MIDlet suite. If this method is called by a MIDlet when
     * the record store is already open by a MIDlet in the MIDlet suite,
     * this method returns a reference to the same RecordStore object.
     *
     * @param recordStoreName the MIDlet suite unique name for the
     *          record store, consisting of between one and 32 Unicode
     *          characters inclusive.
     * @param createIfNecessary if true, the record store will be
     *          created if necessary
     *
     * @return <code>RecordStore</code> object for the record store
     *
     * @exception RecordStoreException if a record store-related
     *          exception occurred
     * @exception RecordStoreNotFoundException if the record store
     *          could not be found
     * @exception RecordStoreFullException if the operation cannot be
     *          completed because the record store is full
     * @exception IllegalArgumentException if
     *          recordStoreName is invalid
     */
    public static RecordStore openRecordStore(String recordStoreName,
                                              boolean createIfNecessary)
        throws RecordStoreException, RecordStoreFullException,
        RecordStoreNotFoundException {
    
        try {
            Object args[] = {recordStoreName, new Boolean(createIfNecessary)};
            return new RecordStore(openRecordStoreMethod.invoke(clazz, args));
        } catch (InvocationTargetException ite){
            Throwable th = ite.getCause();
            if (recordStoreFullException.isInstance(th) ) {
                throw new RecordStoreFullException(th.getMessage());
            } else if (recordStoreNotFoundException.isInstance(th) ) {
                throw new RecordStoreNotFoundException(th.getMessage());
            } else if (recordStoreException.isInstance(th) ) {
                throw new RecordStoreException(th.getMessage());
            } else {
                throw new RecordStoreException("Exception while openning '" + recordStoreName + "' record store");
            }
        } catch (IllegalAccessException e) {
            throw new RuntimeException("Record store is not available: " + e.toString());
        } catch (IllegalArgumentException e) {
            throw new RuntimeException("Record store is not available: " + e.toString());
        }
        
    }

    /**
     * Returns an enumeration for traversing a set of records in the
     * record store in an optionally specified order.<p>
     *
     * The filter, if non-null, will be used to determine what
     * subset of the record store records will be used.<p>
     *
     * The comparator, if non-null, will be used to determine the
     * order in which the records are returned.<p>
     *
     * If both the filter and comparator is null, the enumeration
     * will traverse all records in the record store in an undefined
     * order. This is the most efficient way to traverse all of the
     * records in a record store.  If a filter is used with a null
     * comparator, the enumeration will traverse the filtered records
     * in an undefined order.
     *
     * The first call to <code>RecordEnumeration.nextRecord()</code>
     * returns the record data from the first record in the sequence.
     * Subsequent calls to <code>RecordEnumeration.nextRecord()</code>
     * return the next consecutive record's data. To return the record
     * data from the previous consecutive from any
     * given point in the enumeration, call <code>previousRecord()</code>.
     * On the other hand, if after creation the first call is to
     * <code>previousRecord()</code>, the record data of the last element
     * of the enumeration will be returned. Each subsequent call to
     * <code>previousRecord()</code> will step backwards through the
     * sequence.
     *
     * @param filter if non-null, will be used to determine what
     *          subset of the record store records will be used
     * @param comparator if non-null, will be used to determine the
     *          order in which the records are returned
     * @param keepUpdated if true, the enumerator will keep its enumeration
     *          current with any changes in the records of the record
     *          store. Use with caution as there are possible
     *          performance consequences. If false the enumeration
     *          will not be kept current and may return recordIds for
     *          records that have been deleted or miss records that
     *          are added later. It may also return records out of
     *          order that have been modified after the enumeration
     *          was built. Note that any changes to records in the
     *          record store are accurately reflected when the record
     *          is later retrieved, either directly or through the
     *          enumeration. The thing that is risked by setting this
     *          parameter false is the filtering and sorting order of
     *          the enumeration when records are modified, added, or
     *          deleted.
     *
     * @exception RecordStoreNotOpenException if the record store is
     *          not open
     *
     * @see RecordEnumeration#rebuild
     *
     * @return an enumeration for traversing a set of records in the
     *          record store in an optionally specified order
     */
    public RecordEnumeration enumerateRecords(RecordFilter filter,
                                              RecordComparator comparator,
                                              boolean keepUpdated)
        throws RecordStoreNotOpenException {

        Object recordFilter;
        Object recordComparator;

        if (filter != null) {
            throw new RuntimeException("Not implemented yet");
        } else {
            recordFilter = null;
        }

        if (comparator != null) {
            throw new RuntimeException("Not implemented yet");
        } else {
            recordComparator = null;
        }

        try {
            Object args[] = { recordFilter, recordComparator, new Boolean(keepUpdated) };
            Object enumeration = enumerateRecordsMethod.invoke(obj, args);            
            return (RecordEnumeration)Proxy.newProxyInstance(
                                             recordEnumerationClass.getClassLoader(),
                                             new Class[] { recordEnumerationClass, RecordEnumeration.class },
                                             new RecordStoreInvocationHandler(enumeration));
        } catch (InvocationTargetException ite){
            throw new RecordStoreNotOpenException(ite.getMessage());
        } catch (IllegalAccessException e) {
            throw new RuntimeException("Record store is not available: " + e.toString());
        } catch (IllegalArgumentException e) {
            throw new RuntimeException("Record store is not available: " + e.toString());
        }
    }

    /**
     * Returns a copy of the data stored in the given record.
     *
     * @param recordId the ID of the record to use in this operation
     *
     * @exception RecordStoreNotOpenException if the record store is
     *          not open
     * @exception InvalidRecordIDException if the recordId is invalid
     * @exception RecordStoreException if a general record store
     *          exception occurs
     *
     * @return the data stored in the given record. Note that if the
     *          record has no data, this method will return null.
     * @see #setRecord
     */
    public byte[] getRecord(int recordId)
        throws RecordStoreNotOpenException, InvalidRecordIDException,
            RecordStoreException {

        try {            
            return (byte [])getRecordMethod.invoke(obj, new Object[] { new Integer(recordId) });       
        } catch (InvocationTargetException ite){
            Throwable th = ite.getCause();
            if (recordStoreNotOpenException.isInstance(th) ) {
                throw new RecordStoreNotOpenException(th.getMessage());
            } else if (invalidRecordIDException.isInstance(th) ) {
                throw new InvalidRecordIDException(th.getMessage());
            } else if (recordStoreException.isInstance(th) ) {
                throw new RecordStoreException(th.getMessage());
            } else {
                throw new RecordStoreException("Exception while getting '" + recordId + "' record");
            }
        } catch (IllegalAccessException e) {
            throw new RuntimeException("Record store is not available: " + e.toString());
        } catch (IllegalArgumentException e) {
            throw new RuntimeException("Record store is not available: " + e.toString());
        }
    }

}
