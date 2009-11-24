package com.sun.midp.lcdui;


import java.util.Timer;
import java.util.TimerTask;

import com.sun.midp.ams.VMUtils;
import com.sun.midp.chameleon.NoticePopup;
import com.sun.midp.lcdui.DisplayContainer;
import com.sun.midp.lcdui.LCDUIEnvironment;
import com.sun.midp.main.Configuration;
import com.sun.midp.main.NoticeManager;
import com.sun.midp.main.NoticeManagerListener;
import com.sun.midp.midlet.MIDletEventConsumer;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.security.SecurityToken;

/**
 * 
 * 
 */
public class NoticeVisualizer implements NoticeManagerListener  {

    // current popup
    NoticePopup popup;
    private DisplayContainer dc;
    private MIDletEventConsumer mec;
    private boolean enableInterruption;

    private static NoticeVisualizer singleton;

    /* -------------------------- NoticeManagerListener interface ------------------ */
    public void notifyNotice(Notice notice) {
        DisplayAccess da = dc.findForegroundDisplay();
        if (null != da) {
            if (null == popup && enableInterruption) {
                popup = new NoticePopup(notice, this);
                if (null != mec) {
                    mec.handleMIDletPauseEvent();
                }
                da.showPopup(popup);
            }
        }
    }

    public void removeNotice(Notice notice) {
        if (null != popup) {
            // to be removed from layers array by itself
            popup.dismiss();
        }
    }

    public void updateNotice(Notice notice) {
    }


    /* ------------------------------------------------------------------------------ */
    public void removeNotify(NoticePopup np) {
        popup = null;

        Notice n = NoticeManager.getInstance().pop();
        DisplayAccess da = dc.findForegroundDisplay();
        if (null != n && null != da) {
            popup = new NoticePopup(n, this);
            da.showPopup(popup);

        } else {
            // disable popup, don't annoy user
            enableInterruption = false;
            /* Replace duration value with more conform value*/
            int timeout = 
                Configuration.getIntProperty("NOTIFICATION.INTERRUPTION_TIME", 10000);
            new Timer().schedule(new PauseTask(), timeout); 

            if (null != mec) {
                mec.handleMIDletActivateEvent();
            }
        }

    }


    /* ------------------------------------------------------------------------------ */

    private NoticeVisualizer(LCDUIEnvironment lcduienv, SecurityToken  t) {
        NoticeManager.getInstance().addListener(this);
        dc = lcduienv.getDisplayContainer();

        if (VMUtils.getIsolateId() != VMUtils.getAmsIsolateId()) {
            MIDletStateHandler handler = MIDletStateHandler.getMidletStateHandler();
            String midlet = handler.getFirstRunningMidlet();
            mec = handler.getMIDletEventConsumer(t, midlet);
        }

        enableInterruption = true;
    }

    public static void init(LCDUIEnvironment lcduienv, SecurityToken t) {
        if (null == singleton) {
            singleton = new NoticeVisualizer(lcduienv, t);
        } else {
            throw new RuntimeException("Second try of NoticeVisualizer creation");
        }
    }


    /* ------------------------------------------------------------------------------ */
    class PauseTask extends TimerTask {
        /**
         * The action to be performed by this timer task.
         */
        public void run() {
            enableInterruption = true;
        }
    }
}

