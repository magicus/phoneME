import javax.microedition.rms.*;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;
import javax.microedition.io.*;

public class RMSLockTestThread implements Runnable {
    private RecordStore rs = null;
    private byte[] magic = {(byte)0xDE, (byte)0xAD, (byte)0xBE, (byte)0xEF };

    RMSLockTestThread(RecordStore rs) {
        this.rs = rs;
    }

    public void run() {
        System.err.println("Starting thread...");
        byte[] buf = new byte[magic.length];
        while (true) {
            try {
                int totalRecords = rs.getNumRecords();
                if (totalRecords > 0) {
                    RecordEnumeration re = rs.enumerateRecords(null, null, false);
                    while (re.hasNextElement()) {
                        int id = re.nextRecordId();
                        rs.getRecord(id, buf, 0);
                        System.err.println("Deleting record...");                        
                        rs.deleteRecord(id);
                        re.rebuild();
                    }                    
                } else {
                    System.err.println("Adding record...");
                    rs.addRecord(magic, 0, magic.length);
                }
            } catch (RecordStoreException e) {
                e.printStackTrace();
            }
        }
    }    
}
