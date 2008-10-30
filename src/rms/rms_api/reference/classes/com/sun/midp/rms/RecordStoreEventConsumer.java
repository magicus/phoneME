package com.sun.midp.rms;

public interface RecordStoreEventConsumer {
    public void handleRecordStoreChange(
        String keyBase, String recordStoreName,
        int changeType, int recordId);
}