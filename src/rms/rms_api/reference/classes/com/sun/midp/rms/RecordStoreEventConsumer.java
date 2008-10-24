package com.sun.midp.rms;

public interface RecordStoreEventConsumer {
    public void handleRecordStoreChange(
        int suiteId, String recordStoreName,
        int changeType, int recordId);
}