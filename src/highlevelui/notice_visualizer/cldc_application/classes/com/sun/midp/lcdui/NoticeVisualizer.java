package com.sun.midp.lcdui;


import com.sun.midp.ams.VMUtils;
import com.sun.midp.lcdui.DisplayContainer;
import com.sun.midp.lcdui.LCDUIEnvironment;
import com.sun.midp.main.NoticeManager;
import com.sun.midp.main.NoticeManagerListener;
import com.sun.midp.midlet.MIDletEventConsumer;
import com.sun.midp.midlet.MIDletStateHandler;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.chameleon.NoticePopup;

/**
 * 
 * 
 */
public class NoticeVisualizer implements NoticeManagerListener  {

    // current popup
    NoticePopup popup;
    private LCDUIEnvironment env;
    private SecurityToken token;

    private static NoticeVisualizer singleton;

    /* -------------------------- NoticeManagerListener interface ------------------ */
    public void notifyNotice(Notice notice) {
        DisplayContainer dc = env.getDisplayContainer();
        DisplayAccess da = dc.findForegroundDisplay();

        if (null != da) {
            // IMPL_NOTE: add relax timer to be unobtrusive
            if (null == popup) {
                popup = new NoticePopup(notice, this);
                if (VMUtils.getIsolateId() != VMUtils.getAmsIsolateId()) {
                    MIDletStateHandler handler = MIDletStateHandler.getMidletStateHandler();
                    String midlet = handler.getFirstRunningMidlet();
                    MIDletEventConsumer mec = handler.getMIDletEventConsumer(token, midlet);
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
    }

    private NoticeVisualizer(LCDUIEnvironment lcduienv, SecurityToken  t) {
        NoticeManager.getInstance().addListener(this);
        env = lcduienv;
        token = t;
    }

    public static void init(LCDUIEnvironment lcduienv, SecurityToken t) {
        if (null == singleton) {
            singleton = new NoticeVisualizer(lcduienv, t);
        } else {
            throw new RuntimeException("Second try of NoticeVisualizer creation");
        }
    }

}
