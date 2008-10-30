package com.sun.midp.rms;

import com.sun.midp.events.EventListener;
import com.sun.midp.events.Event;
import com.sun.midp.events.NativeEvent;
import com.sun.midp.events.EventTypes;
import com.sun.midp.log.Logging;
import com.sun.midp.log.LogChannels;

public class RecordStoreEventListener implements EventListener {
    /** Event consumer for record store change events */
    static private RecordStoreEventConsumer
        recordStoreEventConsumer;

    public static void setConsumer(RecordStoreEventConsumer consumer) {
        System.out.println("Set new record store events consumer: " + consumer);
        recordStoreEventConsumer = consumer;
    }

    public boolean preprocess(Event event, Event waitingEvent) {
        return true;
    }

    public void process(Event event) {
        if (recordStoreEventConsumer != null) {
            // Remember consumer, it can be changed until we call it
            RecordStoreEventConsumer consumer = recordStoreEventConsumer;
            NativeEvent nativeEvent = (NativeEvent) event;
            if (event.getType() == EventTypes.RECORD_STORE_CHANGE_EVENT) {
                int changeType = nativeEvent.intParam1;
                int recordId = nativeEvent.intParam2;
                String keyBase = nativeEvent.stringParam1;
                String recordStoreName = nativeEvent.stringParam2;
                if (true || Logging.REPORT_LEVEL <= Logging.INFORMATION) {
                    Logging.report(Logging.INFORMATION, LogChannels.LC_RMS,
                        "RecordStoreEventListener.process(): " +
                            "keyBase = " + keyBase + ", "  +
                            "storeName = " + recordStoreName + ", " +
                            "changeType = " + changeType + ", " +
                            "recordId = " + recordId);
                }
                consumer.handleRecordStoreChange(
                    keyBase, recordStoreName, changeType, recordId);
            }
        }
    }
}