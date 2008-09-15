/*
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
#ifndef __JAVACALL_AMS_RMS_H
#define __JAVACALL_AMS_RMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "javacall_defs.h"
#include "javacall_ams_config.h"

/*
 * Functions declared in this file are exported by the MIDP runtime.
 * They should be used in case if the RMS is implemented on the
 * Platform's side.
 */

/**
 * @file javacall_ams_rms.h
 * @ingroup NAMS
 * @brief Javacall interfaces for plugging RMS located on the Platform's side
 *
 * IMPL_NOTE: functions to add/remove listeners and to enumerate records must
 *            be added.
 */

/**
 * @defgroup RMSAPI API for plugging RMS located on the Platform's side
 * @ingroup NAMS
 *
 *
 * @{
 */

/**
 * Java invokes this function to open a record store.
 *
 * @param suiteId [in]  unique ID of the MIDlet suite
 * @param rmsName [in]  name of the RMS to open
 * @param createIfNecessary [in] if JAVACALL_TRUE, the record store
                                 will be created if necessary
 * @param pRmsId  [out] pointer to a place where ID of the opened RMS
 *                      will be saved
 *
 * @return <tt>JAVACALL_OK</tt> if the record store was
 *         successfully opened, an error code otherwise
 */
javacall_result
javacall_ams_rms_open(javacall_suite_id suiteId,
                      javacall_utf16_string rmsName,
                      javacall_bool createIfNecessary,
                      javacall_rms_id* pRmsId);

/**
 * Java invokes this function to close a record store.
 *
 * @param rmsId [in] unique ID of the record store
 *
 * @return <tt>JAVACALL_OK</tt> if the record store was
 *         successfully closed, an error code otherwise
 */
javacall_result
javacall_ams_rms_close(javacall_rms_id rmsId);

/**
 * App Manager or Java invokes this function to delete a record store.
 *
 * @param suiteId [in] unique ID of the MIDlet suite
 * @param rmsName [in] name of the RMS to delete
 *
 * @return <tt>JAVACALL_OK</tt> if the record store was
 *         successfully deleted, an error code otherwise
 */
javacall_result
javacall_ams_rms_delete(javacall_suite_id suiteId,
                        javacall_utf16_string rmsName);

/**
 * Java invokes this function to get a record store size.
 *
 * @param rmsId [in]  unique ID of the record store
 * @param pSize [out] on exit will hold a size of the RMS
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_size(javacall_rms_id rmsId,
                          javacall_int32* pSize);

/**
 * Java invokes this function to get the amount of additional room (in bytes)
 * available for the given record store to grow.
 *
 * @param rmsId [in]  unique ID of the record store
 * @param pSize [out] on exit will hold the available size for the RMS
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_size_available(javacall_rms_id rmsId,
                                    javacall_int32* pSize);

/**
 * Java invokes this function to get the size of the given record.
 *
 * @param rmsId    [in]  unique ID of the record store
 * @param recordId [in]  unique ID of the record in this record store
 * @param pSize    [out] on exit will hold the size of the record
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_size(javacall_rms_id rmsId,
                          javacall_rms_record_id recordId,
                          javacall_int32* pSize);

/**
 * Java invokes this function to get the number of records
 * currently in the given record store.
 *
 * @param rmsId            [in]  unique ID of the record store
 * @param pNumberOfRecords [out] on exit will hold the number
 *                               of records in this RMS
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_number_of_records(javacall_rms_id rmsId,
                                       javacall_int32* pNumberOfRecords);

/**
 * Java invokes this function to get a record store version.
 * Each time a record store is modified (by add_record, set_record,
 * delete_record) its version is incremented.
 *
 * @param rmsId    [in]  unique ID of the record store
 * @param pVersion [out] on exit will hold the record store version
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_version(javacall_rms_id rmsId,
                             javacall_int32* pVersion);

/**
 * Java invokes this function to get the name of the record store having
 * the given ID.
 *
 * @param rmsId       [in]  unique ID of the record store
 * @param name        [out] on exit will hold the name of the record store
 * @param maxNameSize [in] size of buffer pointed by rmsName
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_name(javacall_rms_id rmsId,
                          javacall_utf16_string rmsName,
                          javacall_in32 maxNameSize);

/**
 * Java invokes this function to get a record store modification time.
 * The returned value is a difference in milliseconds between the
 * modification time and midnight, January 1, 1970 UTC.
 *
 * @param rmsId [in]  unique ID of the record store
 * @param pTime [out] on exit will hold the record store modification time 
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_last_modified(javacall_rms_id rmsId,
                                   javacall_int64* pTime);

/**
 * Java invokes this function to add a new record to the given record store.
 *
 * @param rmsId       [in]  unique ID of the record store
 * @param pData       [in]  the data to be stored in this record. If the record
 *                          is to have zero-length data (no data), this parameter
 *                          may be NULL.
 * @param offset      [in]  the index into the data buffer of the first relevant
 *                          byte for this record
 * @param sizeInBytes [in]  the number of bytes of the data buffer to use for
 *                          this record (may be zero)
 * @param pRecordId   [out] on exit will hold an unique ID of the new record
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_add_record(javacall_rms_id rmsId,
                            javacall_uint8* pData,
                            javacall_int32  offset,
                            javacall_int32  sizeInBytes,
                            javacall_rms_record_id* pRecordId);

/**
 * Java invokes this function to set the data in the given record of the given
 * record store to that passed in. After this method returns, a call to
 * java_rms_get_gecord(rmsId, recordId) will return an array of sizeInBytes
 * size containing the data supplied here.
 *
 * @param rmsId       [in]  unique ID of the record store
 * @param recordId    [in]  unique ID of the record in this record store
 * @param pNewData    [in]  the new data to store in the record
 * @param sizeInBytes [in]  the number of bytes of the data buffer to use for
 *                          this record
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_set_record(javacall_rms_id rmsId,
                            javacall_rms_record_id recordId,
                            javacall_uint8* pNewData,
                            javacall_int32  sizeInBytes);

/**
 * Java invokes this function to delete the given record from the given
 * record store. The recordId for this record is NOT reused.
 *
 * @param rmsId    [in]  unique ID of the record store
 * @param recordId [in]  unique ID of the record in this record store
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_delete_record(javacall_rms_id rmsId,
                               javacall_rms_record_id recordId);

/**
 * Java invokes this function to get the data stored in the given record
 * of the given record store.
 *
 * @param rmsId       [in]  unique ID of the record store
 * @param recordId    [in]  unique ID of the record in this record store
 * @param pBuffer     [out] pointer to the buffer in which to copy the data
 * @param bufSize     [in]  size of buffer pointed by pBuffer, in bytes
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_record(javacall_rms_id rmsId,
                            javacall_rms_record_id recordId,
                            javacall_uint8* pBuffer,
                            javacall_int32  bufSize);

/**
 * Java invokes this function to the record Id of the next record to be added
 * to the given record store. Note that this record Id is only valid while the
 * record store remains open and until a call to java_rms_add_record().
 *
 * @param rmsId     [in]  unique ID of the record store
 * @param pRecordId [out] on exit will hold the record Id of the next record
 *                        to be added to the record store
 *
 * @return <tt>JAVACALL_OK</tt> if successful, an error code otherwise
 */
javacall_result
javacall_ams_rms_get_next_record_id(javacall_rms_id rmsId,
                                    javacall_rms_record_id* pRecordId);

/** @} */

#ifdef __cplusplus
}
#endif

#endif  /* __JAVACALL_AMS_RMS_H */
