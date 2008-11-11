package com.sun.midp.rms;

/**
 * This interface provides a set of methods to handle asynchronous record store
 * notification events on record store changes like record addition, deletion or
 * modification
 */
public interface RecordStoreEventConsumer {

    /**
     * Called by event delivery when an record
     * store change event is processed
     *
     * @param suiteId suite ID of record store
     * @param recordStoreName name of record store
     * @param changeType type of record change: ADDED, DELETED or CHANGED
     * @param recordId ID of the changed record
     */
    public void handleRecordStoreChange(
        int suiteId, String recordStoreName,
        int changeType, int recordId);
}