package com.sun.midp.appmanager;

import javax.microedition.midlet.*;
import javax.microedition.lcdui.*;

import com.sun.midp.io.j2me.storage.*;
import com.sun.midp.main.MIDletSuiteUtils;
import com.sun.midp.midletsuite.*;

public class MIDletSuiteLauncher extends MIDlet implements ApplicationManager {
    
    /** MIDlet Suite storage object. */
    private MIDletSuiteStorage midletSuiteStorage;

        /** UI to display error alerts. */
    private DisplayError displayError;

    /** Display for the Launcher MIDlet. */
    Display display;


    public MIDletSuiteLauncher() {
	Display display = Display.getDisplay(this);
	displayError = new DisplayError(display);

	midletSuiteStorage = MIDletSuiteStorage.getMIDletSuiteStorage();
	try {
	    String suiteID = getAppProperty("arg-0");
            int id = Integer.parseInt(suiteID);
	    MIDletSuiteImpl midletSuite = 
                midletSuiteStorage.getMIDletSuite(id, false);
            if (midletSuite == null) {
		/* check if the suite is the index of the suite
                 * as returned by -Xjam:list */
		try {
		    int suiteIndex = id - 1;
		    int[] suites = midletSuiteStorage.getListOfSuites();
		    id = suites[suiteIndex];
		    midletSuite = midletSuiteStorage.getMIDletSuite(id, false);
		} catch (Exception e) {
		    displayError.showErrorAlert("MIDlet Suite not found",
                                                e, null,
                                                "MIDlet Suite " + 
                                                suiteID + " not found");
		    return;
		}
	    }
	    RunningMIDletSuiteInfo suiteInfo = 
                new RunningMIDletSuiteInfo(id, midletSuite, midletSuiteStorage);
	
	    if (suiteInfo.hasSingleMidlet()) {
		launchSuite(suiteInfo, suiteInfo.midletToRun);
	    } else {
		new MIDletSelector(suiteInfo, display, null, this);
	    }
	} catch (Throwable t) {
	     t.printStackTrace();
	}
    }

    protected void startApp() throws MIDletStateChangeException {
    }
    
    protected void pauseApp() {
    }

    protected void destroyApp(boolean unconditional) {
    }

    /** Discover and install a suite. */
    public void installSuite() {
    }

    /** Launch the CA manager. */
    public void launchCaManager() {
    }

    /**
     * Launches a suite.
     *
     * @param suiteInfo information for suite to launch
     * @param midletToRun class name of the MIDlet to launch
     */
    public void launchSuite(RunningMIDletSuiteInfo suiteInfo,
                            String midletToRun) {
	try {
	    // Create an instance of the MIDlet class
	    // All other initialization happens in MIDlet constructor
	    MIDletSuiteUtils.execute(suiteInfo.suiteId, midletToRun, null);
	} catch (Exception ex) {
	    displayError.showErrorAlert(suiteInfo.displayName, ex, null, null);
	}
    }

    /**
     * Update a suite.
     *
     * @param suiteInfo information for suite to update
     */
    public void updateSuite(RunningMIDletSuiteInfo suiteInfo) {
    }

    /**
     * Shut downt the system
     */
    public void shutDown() {
	destroyApp(false);
	notifyDestroyed();
    }

    /**
     * Bring the midlet with the passed in midlet suite info to the 
     * foreground.
     * 
     * @param suiteInfo information for the midlet to be put to foreground
     */
    public void moveToForeground(RunningMIDletSuiteInfo suiteInfo) {
    }
    
    /**
     * Exit the midlet with the passed in midlet suite info.
     * 
     * @param suiteInfo information for the midlet to be terminated
     */
    public void exitMidlet(RunningMIDletSuiteInfo suiteInfo) {
    }
}
