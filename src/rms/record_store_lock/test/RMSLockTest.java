import javax.microedition.rms.*;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.io.*;

public class RMSLockTest extends MIDlet {
    private RecordStore rs = null;
    private static final String REC_STORE = "rmslocktest";

    public RMSLockTest() {
        try {
            System.err.println("Opening record store");
            rs = RecordStore.openRecordStore("RMSLockTest", true, RecordStore.AUTHMODE_ANY, true);
        } catch (RecordStoreException e) {
            e.printStackTrace();
        }        
    }

    public void destroyApp(boolean unconditional) {
    }

    public void startApp() {
        RMSLockTestThread t = new RMSLockTestThread(rs);
        Thread myThread = new Thread(t);
        myThread.start();
    }

    public void pauseApp() {
    }
}
