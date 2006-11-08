/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

/*
 * @(#)Animator.java	1.9 03/01/23
 */

import java.awt.*;
import java.awt.event.*;
import java.applet.AudioClip;
import java.net.URL;
import java.net.MalformedURLException;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import javax.microedition.xlet.*;

/**
 * An applet that plays a sequence of images, as a loop or a one-shot.
 * Can have a soundtrack and/or sound effects tied to individual frames.
 * See the <a href="http://java.sun.com/applets/applets/Animator/">Animator
 * home page</a> for details and updates.
 *
 * @author Herb Jellinek
 * @version 1.9, 01/23/03
 */
public class Animator
    extends Panel
    implements Runnable, MouseListener, Xlet {
    int appWidth = 0; // Animator width
    int appHeight = 0; // Animator height
    Thread engine = null; // Thread animating the images
    boolean userPause = false; // True if thread currently paused by user
    boolean loaded = false; // Can we paint yet?
    boolean error = false; // Was there an initialization error?
    Animation animation = null; // Animation this animator contains
    String hrefTarget = null; // Frame target of reference URL if any
    URL hrefURL = null; // URL link for information if any

    static final String sourceLocation =
        "http://java.sun.com/applets/applets/Animator/";
    static final String userInstructions = "shift-click for errors, info";
    static final int STARTUP_ID = 0;
    static final int BACKGROUND_ID = 1;
    static final int ANIMATION_ID = 2;

    /************************ Modifiable parameters *****************************/
    String imagesource = "images";
    String backgroundcolor = "0xffffff";
    String endimage = "10";
    String startimage = null;
    String pause = "200";
    String repeat = "true";
    String startup = "loading-msg.gif";
//    String startup = null;
    String namepattern = null;
    String images = null;
    String background = null;
//    String positions = "0@10|0@30|0@50|0@70|0@90|0@110|0@130|0@150|0@170|0@190";
    String positions = null;
    /****************************************************************************/

    public Frame getRootFrame(Container rootContainer) {
        Container tmp = rootContainer;
        while (! (tmp instanceof Frame)) {
            tmp = tmp.getParent();
        }
        return (Frame) tmp;
    }

    public void initXlet(XletContext context) {
        System.err.println("***** INIT_XLET(Animator) *****");
        try {
            Container container = context.getContainer();
            container.add(this);
            container.setVisible(true);
	    init();
        } catch (UnavailableContainerException e) {
            System.out.println("Error in getting a root container: " + e);
            context.notifyDestroyed();
        }
    }

    public void startXlet() {
        System.err.println("***** START_XLET(Animator) *****");
        start();
    }

    public void pauseXlet() {
        System.err.println("***** PAUSE_XLET(Animator) *****");
        stop();
    }

    public void destroyXlet(boolean unconditional) {
        System.err.println("***** DESTROY_XLET(Animator) *****");
    }

    public Dimension getPreferredSize() {
        return new Dimension(480, 400);
    }

    /**
     * Get parameters and parse them
     */
    public void handleParams() {
        try {
            String param = imagesource;
            animation.imageSource = new URL(getClass().getResource(imagesource) +
                                            "/");

            param = pause;
            if (param != null) {
                animation.setGlobalPause(Integer.parseInt(param));
            }
            param = repeat;
            animation.repeat = (param == null) ? true :
                (param.equalsIgnoreCase("yes") ||
                 param.equalsIgnoreCase("true"));
            int startImage = 1;
            int endImage = 1;
            param = endimage;
            if (param != null) {
                endImage = Integer.parseInt(param);
                param = startimage;
                if (param != null) {
                    startImage = Integer.parseInt(param);
                }
                param = namepattern;
                animation.prepareImageRange(startImage, endImage, param);
            }
            else {
                param = startimage;
                if (param != null) {
                    startImage = Integer.parseInt(param);
                    param = namepattern;
                    animation.prepareImageRange(startImage, endImage, param);
                }
                else {
                    param = images;
                    if (param == null) {
                        System.out.println(
                            "No legal IMAGES, STARTIMAGE, or ENDIMAGE " +
                            "specified.");
                        error = true;
                        return;
                    }
                    else {
                        animation.parseImages(param, namepattern);
                    }
                }
            }

            param = background;
            if (param != null) {
                animation.backgroundImageURL = new URL(animation.imageSource,
                    param);
            }
            param = backgroundcolor;
            if (param != null) {
                animation.backgroundColor = decodeColor(param);
            }
            param = startup;
            if (param != null) {
                animation.startUpImageURL = new URL(animation.imageSource,
                    param);
            }
            param = pause;
            if (param != null) {
                animation.parseDurations(param);
            }
            param = positions;
            if (param != null) {
                animation.parsePositions(param);
            }
        }
        catch (MalformedURLException e) {
            showParseError(e);
        }
        catch (ParseException e) {
            showParseError(e);
        }
    }

    private Color decodeColor(String s) {
        int val = 0;
        try {
            if (s.startsWith("0x")) {
                val = Integer.parseInt(s.substring(2), 16);
            }
            else if (s.startsWith("#")) {
                val = Integer.parseInt(s.substring(1), 16);
            }
            else if (s.startsWith("0") && s.length() > 1) {
                val = Integer.parseInt(s.substring(1), 8);
            }
            else {
                val = Integer.parseInt(s, 10);
            }
            return new Color(val);
        }
        catch (NumberFormatException e) {
            return null;
        }
    }

    /**
     * Initialize the applet.  Get parameters.
     */
    public void init() {

        //animation.tracker = new MediaTracker(this);
        appWidth = getWidth();
        appHeight = getHeight();
        animation = new Animation(this);
        handleParams();
        animation.init();
        addMouseListener(this);
        Thread me = Thread.currentThread();
        me.setPriority(Thread.MIN_PRIORITY);
        userPause = false;
    }

    public void destroy() {
        removeMouseListener(this);
    }

    void tellLoadingMsg(String file, String fileType) {
        System.out.println("Animator: loading " + fileType + " " + file);
    }

    void tellLoadingMsg(URL url, String fileType) {
        tellLoadingMsg(url.toExternalForm(), fileType);
    }

    void clearLoadingMessage() {
        System.out.println("");
    }

    void loadError(String fileName, String fileType) {
        String errorMsg = "Animator: Couldn't load " + fileType + " " +
            fileName;
//        showStatus(errorMsg);
        System.out.println(errorMsg);
        error = true;
        repaint();
    }

    void loadError(URL badURL, String fileType) {
        loadError(badURL.toExternalForm(), fileType);
    }

    void showParseError(Exception e) {
        String errorMsg = "Animator: Parse error: " + e;
        System.out.println(errorMsg);
        error = true;
        repaint();
    }

    /**
     * Run the animation. This method is called by class Thread.
     * @see java.lang.Thread
     */
    public void run() {
        Thread me = Thread.currentThread();
        if (animation.frames == null) {
            return;
        }
        if ( (appWidth <= 0) || (appHeight <= 0)) {
            return;
        }
        try {
            while (engine == me) {
                // Get current frame and paint it, play its sound
                AnimationFrame thisFrame = (AnimationFrame)
                    animation.frames.get(animation.currentFrame);
                repaint();
                animation.currentFrame++;
                // Check if we are done
                if (animation.currentFrame >= animation.frames.size()) {
                    if (animation.repeat) {
                        animation.currentFrame = 0;
                    }
                    else {
                        return;
                    }
                }

                // Pause for duration or longer if user paused
                try {
                    Thread.sleep(thisFrame.duration);
                    synchronized (this) {
                        while (userPause) {
                            wait();
                        }
                    }
                }
                catch (InterruptedException e) {
                }
            }
        }
        finally {
            synchronized (this) {
                if (engine == me) {
                }
            }
        }

    }

    /**
     * No need to clear anything; just paint.
     */
    public void update(Graphics g) {
        paint(g);
    }

    /**
     * Paint the current frame
     */
    public void paint(Graphics g) {
        if (animation == null) {
            return;
        }
        if (error || !loaded) {
            if (animation.startUpImage != null) {
                if (animation.tracker.checkID(STARTUP_ID)) {
                    if (animation.backgroundColor != null) {
                        g.setColor(animation.backgroundColor);
                        g.fillRect(0, 0, appWidth, appHeight);
                    }
                    g.drawImage(animation.startUpImage, 0, 0, this);
                }
            }
            else {
                if ( (animation.backgroundImage != null) &&
                    (animation.tracker.checkID(BACKGROUND_ID))) {
                    g.drawImage(animation.backgroundImage, 0, 0, this);
                }
                else {
                    g.setColor(Color.black);
                    g.clearRect(0, 0, appWidth, appHeight);
                }
            }
        }
        else {
            animation.paint(g);
        }

    }

    /**
     * Start the applet by forking an animation thread.
     */
    public void start() {
        engine = new Thread(this);
        engine.start();
//        showStatus(getAppletInfo());
    }

    /**
     * Stop the insanity, um, applet.
     */
    public synchronized void stop() {
        engine = null;
        animation.stopPlaying();
        if (userPause) {
            userPause = false;
            notify();
        }
    }

    /**
     * Pause the thread when the user clicks the mouse in the applet.
     * If the thread has stopped (as in a non-repeat performance),
     * restart it.
     */
    public synchronized void mousePressed(MouseEvent event) {
        event.consume();
        if ( (event.getModifiers() & InputEvent.SHIFT_MASK) != 0) {
//            showDescription();
            return;
        }
        else if (hrefURL != null) {
            //Let mouseClicked handle this.
            return;
        }
        else if (loaded) {
            userPause = !userPause;
            if (!userPause) {
                animation.startPlaying();
                notifyAll();
            }
        }
    }

    public void mouseClicked(MouseEvent event) {
        if ( (hrefURL != null) &&
            ( (event.getModifiers() & InputEvent.SHIFT_MASK) == 0)) {
//            getAppletContext().showDocument(hrefURL, hrefTarget);
        }
    }

    public void mouseReleased(MouseEvent event) {
    }

    public void mouseEntered(MouseEvent event) {
//        showStatus(getAppletInfo() + " -- " + userInstructions);
    }

    public void mouseExited(MouseEvent event) {
//        showStatus("");
    }
}

/**
 * A class that represents an animation to be displayed by the applet
 */
class Animation
    extends Object {
    static final int STARTUP_ID = 0;
    static final int BACKGROUND_ID = 1;
    static final int ANIMATION_ID = 2;
    static final String imageLabel = "image";
    static final String soundLabel = "sound";

    int globalPause = 1300; // global pause in milleseconds
    List frames = null; // List holding frames of animation
    int currentFrame; // Index into images for current position
    Image startUpImage = null; // The startup image if any
    Image backgroundImage = null; // The background image if any
    AudioClip soundTrack = null; // The soundtrack for this animation
    Color backgroundColor = null; // Background color if any
    URL backgroundImageURL = null; // URL of background image if any
    URL startUpImageURL = null; // URL of startup image if any
    URL soundTrackURL = null; // URL of soundtrack
    URL imageSource = null; // Directory or URL for images
    URL soundSource = null; // Directory or URL for sounds
    boolean repeat; // Repeat the animation if true
    Image offScrImage; // Offscreen image
    Graphics offScrGC; // Offscreen graphics context
    MediaTracker tracker; // MediaTracker used to load images
    Animator owner; // Applet that contains this animation

    Animation(Animator container) {
        super();
        owner = container;
    }

    void init() {
        tracker = new MediaTracker(owner);
        currentFrame = 0;
        loadAnimationMedia();
        startPlaying();
    }

    void setGlobalPause(int pause) {
        globalPause = pause;
    }

    /**
     * Loads the images and sounds involved with this animation
     */
    void loadAnimationMedia() {
        URL badURL;
        boolean error;
        try {
            if (startUpImageURL != null) {
                owner.tellLoadingMsg(startUpImageURL, imageLabel);
                startUpImage = fetchImageAndWait(startUpImageURL, STARTUP_ID);
                if (tracker.isErrorID(STARTUP_ID)) {
                    owner.loadError(startUpImageURL, "start-up image");
                }
                owner.repaint();
            }

            if (backgroundImageURL != null) {
                owner.tellLoadingMsg(backgroundImageURL, imageLabel);
                backgroundImage = fetchImageAndWait(backgroundImageURL,
                    BACKGROUND_ID);
                if (tracker.isErrorID(BACKGROUND_ID)) {
                    owner.loadError(backgroundImageURL,
                                    "background image");
                }
                owner.repaint();
            }

            // Fetch the animation frame images
            Iterator iterator = frames.iterator();
            while (iterator.hasNext()) {
                AnimationFrame frame = (AnimationFrame) iterator.next();
                owner.tellLoadingMsg(frame.imageLocation, imageLabel);
                frame.image = createImage(frame.imageLocation);
                tracker.addImage(frame.image, ANIMATION_ID);
                try {
                    tracker.waitForID(ANIMATION_ID);
                }
                catch (InterruptedException e) {}
            }

            owner.clearLoadingMessage();
            offScrImage = owner.createImage(owner.appWidth, owner.appHeight);
            offScrGC = offScrImage.getGraphics();
            offScrGC.setColor(Color.lightGray);
            owner.loaded = true;
            error = false;
        }
        catch (Exception e) {
            error = true;
            e.printStackTrace();
        }
    }

    private Image createImage(URL url) {
        return Toolkit.getDefaultToolkit().createImage(url);
    }

    /**
     * Fetch an image and wait for it to come in.  Used to enforce a load
     * order for background and startup images.
     */
    Image fetchImageAndWait(URL imageURL, int trackerClass) throws
        InterruptedException {
        Image image = createImage(imageURL);
        tracker.addImage(image, trackerClass);
        tracker.waitForID(trackerClass);
        return image;
    }

    /**
     * Stuff a range of image names into a List
     * @return a List of image URLs.
     */
    void prepareImageRange(int startImage, int endImage, String pattern) throws
        MalformedURLException {
        frames = new ArrayList(Math.abs(endImage - startImage) + 1);
        if (pattern == null) {
            pattern = "T%N.gif";
        }
        if (startImage > endImage) {
            for (int i = startImage; i >= endImage; i--) {
                AnimationFrame frame = new AnimationFrame();
                frames.add(frame);
                frame.duration = globalPause;
                frame.imageLocation = new URL(imageSource,
                                              doSubst(pattern, i + ""));
            }
        }
        else {
            for (int i = startImage; i <= endImage; i++) {
                AnimationFrame frame = new AnimationFrame();
                frames.add(frame);
                frame.duration = globalPause;
                frame.imageLocation = new URL(imageSource,
                                              doSubst(pattern, i + ""));
            }
        }
    }

    /**
     * Parse the IMAGES parameter.  It looks like
     * 1|2|3|4|5, etc., where each number (item) names a source image.
     */
    void parseImages(String attr, String pattern) throws MalformedURLException {
        frames = new ArrayList();
        if (pattern == null) {
            pattern = "T%N.gif";
        }
        for (int i = 0; i < attr.length(); ) {
            int next = attr.indexOf('|', i);
            if (next == -1) {
                next = attr.length();
            }
            String file = attr.substring(i, next);
            AnimationFrame frame = new AnimationFrame();
            frames.add(frame);
            frame.imageLocation = new URL(imageSource, doSubst(pattern, file));
            frame.duration = globalPause;
            i = next + 1;
        }
    }

    /**
     * Parse the PAUSES parameter.  It looks like
     * 1000|500|||750, etc., where each item corresponds to a
     * source image.  Empty items mean that the corresponding image
     * has no special duration, and should use the global one.
     *
     * @return a Hashtable of Integer pauses keyed to Integer
     * frame numbers.
     */
    void parseDurations(String attr) {
        int imageNum = 0;
        int numImages = frames.size();
        for (int i = 0; (i < attr.length()) && (imageNum < numImages); ) {
            int next = attr.indexOf('|', i);
            if (next == -1) {
                next = attr.length();
            }
            AnimationFrame aFrame = (AnimationFrame) frames.get(imageNum);
            if (i != next) {
                int duration = Integer.parseInt(attr.substring(i, next));
                aFrame.duration = duration;
            }
            i = next + 1;
            imageNum++;
        }
    }

    /**
     * Parse a String of form xxx@yyy and return a Point.
     */
    Point parsePoint(String s) throws ParseException {
        int atPos = s.indexOf('@');
        if (atPos == -1) {
            throw new ParseException("Illegal position: " + s);
        }
        return new Point(Integer.parseInt(s.substring(0, atPos)),
                         Integer.parseInt(s.substring(atPos + 1)));
    }

    /**
     * Parse the POSITIONS parameter.  It looks like
     * 10@30|11@31|||12@20, etc., where each item is an X@Y coordinate
     * corresponding to a source image.  Empty items mean that the
     * corresponding image has the same position as the preceding one.
     *
     * @return a Hashtable of Points keyed to Integer frame numbers.
     */
    void parsePositions(String param) throws ParseException {
        int imageNum = 0;
        int numImages = frames.size();
        for (int i = 0; (i < param.length()) && (imageNum < numImages); ) {
            int next = param.indexOf('|', i);
            if (next == -1) {
                next = param.length();
            }
            if (i != next) {
                AnimationFrame frame = (AnimationFrame) frames.get(imageNum);
                frame.position = parsePoint(param.substring(i, next));
            }
            i = next + 1;
            imageNum++;
        }
    }

    /**
     * Substitute an integer some number of times in a string, subject to
     * parameter strings embedded in the string.
     * Parameter strings:
     *   %N - substitute the integer as is, with no padding.
     *   %<digit>, for example %5 - substitute the integer left-padded with
     *        zeros to <digits> digits wide.
     *   %% - substitute a '%' here.
     * @param inStr the String to substitute within
     * @param theInt the int to substitute, as a String.
     */
    String doSubst(String inStr, String theInt) {
        String padStr = "0000000000";
        int length = inStr.length();
        StringBuffer result = new StringBuffer(length);

        for (int i = 0; i < length; ) {
            char ch = inStr.charAt(i);
            if (ch == '%') {
                i++;
                if (i == length) {
                    result.append(ch);
                }
                else {
                    ch = inStr.charAt(i);
                    if (ch == 'N' || ch == 'n') {
                        // just stick in the number, unmolested
                        result.append(theInt);
                        i++;
                    }
                    else {
                        int pad;
                        if ( (pad = Character.digit(ch, 10)) != -1) {
                            // we've got a width value
                            String numStr = theInt;
                            String scr = padStr + numStr;
                            result.append(scr.substring(scr.length() - pad));
                            i++;
                        }
                        else {
                            result.append(ch);
                            i++;
                        }
                    }
                }
            }
            else {
                result.append(ch);
                i++;
            }
        }
        return result.toString();
    }

    void startPlaying() {
        if (soundTrack != null) {
            soundTrack.loop();
        }
    }

    void stopPlaying() {
        if (soundTrack != null) {
            soundTrack.stop();
        }
    }

    public void update(Graphics g) {
        paint(g);
    }

    public void paint(Graphics g) {
        int xPos = 0;
//        int yPos = 0;
        int yPos = owner.appHeight / 3;
        if ( (frames.size() > 0) && tracker.checkID(ANIMATION_ID) &&
            (offScrGC != null)) {
            AnimationFrame frame = (AnimationFrame) frames.get(currentFrame);
            Image image = frame.image;
            if (backgroundImage == null) {
                offScrGC.clearRect(0, 0, owner.appWidth, owner.appHeight);
            }
            else {
                offScrGC.drawImage(backgroundImage, 0, 0, owner);
            }
            Point pos = null;
            if (frame.position != null) {
                xPos = frame.position.x;
                yPos = frame.position.y;
            }
            if (backgroundColor != null) {
                offScrGC.setColor(backgroundColor);
                offScrGC.fillRect(0, 0, owner.appWidth, owner.appHeight);
                offScrGC.drawImage(image, xPos, yPos, backgroundColor, owner);
            }
            else {
                offScrGC.drawImage(image, xPos, yPos, owner);
            }
            if (offScrImage != null) {
                g.drawImage(offScrImage, 0, 0, owner);
            }
        }
    }
}

/**
 * Instances of this class represent a single frame of an animation
 * There can be an image, sound, and position associated with each frame
 */
class AnimationFrame
    extends Object {
    static final String imageLabel = "image";
    static final String soundLabel = "sound";

    URL imageLocation = null; // Directory or URL of this frames image
    URL soundLocation = null; // Directory or URL of this frames sound
    int duration; // Duration time for this frame in milliseconds
    AudioClip sound; // Sound associated with this frame object
    Image image; // Image associated with this frame
    Point position; // Position of this frame

}

/**
 * ParseException: signals a parameter parsing problem.
 */
class ParseException
    extends Exception {
    ParseException(String s) {
        super(s);
    }
}

/**
 * DescriptionFrame: implements a pop-up "About" box.
 */
class DescriptionFrame
    extends Frame
    implements ActionListener {
    static final int rows = 27;
    static final int cols = 70;
    TextArea info;
    Button cancel;

    DescriptionFrame() {
        super("Animator v1.10");
        add("Center", info = new TextArea(rows, cols));
        info.setEditable(false);
        info.setBackground(Color.white);
        Panel buttons = new Panel();
        add("South", buttons);
        buttons.add(cancel = new Button("Cancel"));
        cancel.addActionListener(this);
        pack();
    }

    public void show() {
        info.select(0, 0);
        super.show();
    }

    void tell(String s) {
        info.append(s);
    }

    public void actionPerformed(ActionEvent e) {
        setVisible(false);
    }
}
