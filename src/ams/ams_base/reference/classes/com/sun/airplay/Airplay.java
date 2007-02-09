package com.sun.airplay;

import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.midlet.MIDletStateHandler;

public class Airplay {

    public static final void RunS3E(String call) {
        MIDletSuite midletSuite =
            MIDletStateHandler.getMidletStateHandler().getMIDletSuite();
        int suiteId = midletSuite.getID();
        RunS3E_(call, suiteId);
    }

    public static final native void RunS3E_(String call, int suiteId);

}