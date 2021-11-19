/*
 * @(#)BaseTestCase.java	1.13 06/10/10
 * 
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation. 
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt). 
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA 
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions. 
 */

package gunit.framework ;

import java.awt.* ;
import java.awt.image.* ;
import java.io.* ;
import java.net.* ;
import java.util.* ;
import java.lang.reflect.* ;

import junit.framework.AssertionFailedError ;
import gunit.image.RefImageNotFoundException ;

/**
 * <code>BaseTestCase</code> is an abstract class that extends JUnit's
 * <code>TestCase</code> and contains methods that should run on PBP/PP.
 * All profile and optional package specific functionality are defined
 * as abstract methods. 
 * <p>
 * <code>BaseTestCase</code> overrides setUp(), tearDown() and runTest() 
 * methods from the base class to provide gunit's Testcase behavioir.
 */
public abstract class BaseTestCase extends junit.framework.TestCase {
    private Container container ;

    private String[] args ;

    // Graphics object for the testcase. 
    private Graphics gc ;

    // created only for Graphics testcases
    private BufferedImage backBuffer ;

    /**
     * Indicates if the testcase asserted using any of the assert 
     * functions provided by gunit.
     */
    protected boolean isAsserted = false ;

    private boolean isGraphicsTestcase = false ;

    // A non-null value indicates that the testcase attempted to assert
    // and the reference image was not found. 
    private String referenceImageFile = null ;

    static TestContainer defaultTestContainer = null ;
    static String[] refImageDirs = null ;
    static TestArguments testArgs = null ;
    static TestFilter testFilter = null ;
    static TestResultVerifier resultVerifier = null ;
                                                      
    static {
        TestContext context = TestContext.getInstance() ;
        defaultTestContainer = (TestContainer)
            context.getObjectValue(TestContext.TEST_CONTAINER_CLASS);
        refImageDirs = getRefImageDirs(
                      context.getStringValue(TestContext.REF_IMAGE_PATH)) ;
        resultVerifier = (TestResultVerifier)
            context.getObjectValue(TestContext.TEST_RESULT_VERIFIER_CLASS);
        testArgs = new TestArguments();
        testFilter = (TestFilter)context.getValue(TestContext.TEST_FILTER) ;
        if ( testFilter == null ) {
            testFilter = new TestFilter() {
                public boolean allowExecution(Class c, String method) {
                    return true ;
                }
            };
        }
    }

    static String[] getRefImageDirs(String path) {
        StringTokenizer st = new StringTokenizer(path, ":") ;
        String[] dirs = new String[st.countTokens()] ;
        for ( int i = 0 ; st.hasMoreTokens() ; i ++ ) {
            dirs[i] = st.nextToken() ;
        }
        return dirs ;
    }

    // junit.framework.TestCase
    protected void setUp() {
        if ( !testFilter.allowExecution(getClass(), getName()) ) 
            return ;

        this.isAsserted = false ;
        this.isGraphicsTestcase = false ;
        this.referenceImageFile = null ;
        this.args = testArgs.getArgs(getClass(), getName()) ;
        this.container = createContainer() ;
        this.container.removeAll() ;
        this.gc   = createGraphics() ;
    }

    // junit.framework.TestCase
    protected void tearDown() {
        if ( !testFilter.allowExecution(getClass(), getName()) ) 
            return ;

        disposeGraphics() ;
        if ( this.referenceImageFile != null ) {
            System.out.println(this.referenceImageFile+
                             " does not exist, in the following directories");
            for ( int i = 0 ; i < this.refImageDirs.length ; i ++ ) {
                System.out.println("    "+this.refImageDirs[i]) ;
            }
            System.out.print("Create image[(y)es/(n)o][default=y]:") ;
            BufferedReader br  
                = new BufferedReader(new InputStreamReader(System.in));
            try {
                String result_code = br.readLine().trim() ;
                if ( result_code.length() <= 0 || 
                     "y".equals(result_code) ) {
                    File f = new File(this.referenceImageFile) ;
                    String dir = f.getParent() ;
                    if ( dir == null ) 
                        dir = this.refImageDirs[this.refImageDirs.length-1];
                    String file = f.getName() ;
                    System.out.print("Enter directory [default="+dir+"]:");
                    result_code = br.readLine().trim() ;
                
                    if ( result_code.length() > 0 ) {
                        dir = result_code ;
                    }
                    file = dir+File.separator+file ;
                    encodeImage(this.backBuffer,file);
                }
            }
            catch (Exception ex) {
            }
        }
        this.container = null ;
        this.backBuffer = null ;
    }

    // junit.framework.TestCase 
    protected void runTest() throws Throwable {
        Class cls = getClass();
        String method = getName() ;
        //System.out.println(cls.getName()+"."+method);
        try {
            if ( testFilter.allowExecution(cls, method) ) {
                super.runTest() ;
            }
            else {
                return ;
            }
            if ( !this.isGraphicsTestcase && this.container != null ) {
                this.container.validate() ;
            }
            // if ( testcase has not asserted using image compare ) {
            //     chances are it had used Assert.assert() methods or
            //     it did not do anything w.r.t the verification
            //
            //     if there is a description information for the testcase
            //     then we display the description and ask the user to
            //     manually verify the result
            //  }
            if ( this.isAsserted ) {
                return ;
            }

            TestResultDescription desc = 
                TestResultDescription.getInstance(cls, method);
            resultVerifier.verify(cls, method, desc) ;
        }
        catch (RefImageNotFoundException refimg_nf_ex) {
            this.referenceImageFile = refimg_nf_ex.getImage() ;
            throw refimg_nf_ex ;
        }
        catch ( Throwable t ) {
            throw t ;
        }
    }

    /**
     * Returns the container for the testcase to create GUI artifacts
     */
    protected final Container getContainer() {
        return this.container ;
    }

    /**
     * Returns the arguments for the testcase. It returns a zero length
     * string if there are no arguments for the testcase
     */
    protected final String[] getArguments() {
        return this.args ;
    }

    /**
     * Returns a <code>Graphics</code> for the graphics testcase to
     * render.
     */
    protected final Graphics getGraphics() {
        this.isGraphicsTestcase = true ;
        return this.gc ;
    }

    protected Container createContainer() {
        return defaultTestContainer.getContainer() ;
    }

    /**
     * Returns the image that contains the testcase rendition
     */
    protected final Object getTestImage() {
        return this.backBuffer ;
    }

    /**
     * Returns the reference image for the testcase.
     */
    protected final Object getReferenceImage() {
        return getReferenceImage(getReferenceImageFilename());
    }

    /**
     * Returns the image file name for the testcase
     */
    protected final String getReferenceImageFilename() {
        if ( !this.isGraphicsTestcase )
            throw new IllegalArgumentException("Not a graphics testcase");

        return getClass().getName().replace('.', '_')+
            "_"+
            getName()+
            "_"+
            this.backBuffer.getWidth()+"x"+this.backBuffer.getHeight()+
            ".png";
    }

    /**
     * Returns the expected image for the fileName relative to the 
     * image directory
     *
     * @param fileName file name relative to the image directory
     */
    protected final Object getReferenceImage(String fileName) {
        for ( int i = 0 ; i < this.refImageDirs.length ; i ++ ) {
            // make the relative fileName absolute with the ref image 
            // directory
            String file = this.refImageDirs[i] + File.separator + fileName ;
            if ( new File(file).exists() ) 
                return file ;
        }

        throw new RefImageNotFoundException("", fileName);
    }

    /**
     * A convenience method that does 
     * <code>assertImageEquals(getReferenceImage(),getTestImage())</code>
     */
    protected final void assertImageEquals() {
        assertImageEquals(getReferenceImage(), getTestImage());
    }

    /**
     * Assertion method for image equal operation.
     *
     * @param expected the value returned from getReferenceImage()
     * @param actual the value returned from getTestImage()
     */
    protected final void assertImageEquals(Object expected, 
                                           Object actual) {
        assertImageEquals("", expected, actual) ;
    }

    /**
     * An utility method that loads an image from a file
     */
    protected final Image loadImage(String imageFile) {
        Image img = Toolkit.getDefaultToolkit().getImage(imageFile);
        return trackImage(img, this.container) ;
    }

    /**
     * An utility method that loads an image from a URL
     */
    protected final Image loadImage(URL url) {
        Image img = Toolkit.getDefaultToolkit().getImage(url);
        return trackImage(img, this.container) ;
    }

    /**
     * Indicates if a back buffer should be used to render the testcase
     * graphics operations. The default value is <code>true</code>, but
     * the testcase can override to return <code>false</code>,so that 
     * screen graphics is used instead of the image buffer
     */
    protected boolean useBackBuffer() {
        return true ;
    }

    /**
     * Creates a <Code>BufferedImage</code> for the specified width
     * and height with INT_ARGB format
     */
    protected abstract BufferedImage createBufferedImage(int width,
                                                         int height) ;

    /**
     * Encode the contents of the <code>BufferedImage</code> with an
     * image compression format (preferably PNG) onto the file specified
     */
    protected abstract void encodeImage(BufferedImage image, 
                                        String filename);

    /**
     * Ensures that the assertion is valid
     *
     * @param message message for the failure case.
     * @param expected the value returned from getReferenceImage()
     * @param actual the value returned from getTestImage()
     */
    protected abstract void assertImageEquals(String message, 
                                              Object expected, 
                                              Object actual) ;

    /**
     * Set the background color for the graphics
     */
    protected abstract void clearToBackgroundColor(Graphics g, Color c,
        int x, int y, int w, int h) ;

    private Graphics createGraphics() {
        if ( useBackBuffer() ) {
            return createImageGraphics() ;
        }
        else {
            return createScreenGraphics() ;
        }
    }

    private void disposeGraphics() {
        if ( this.backBuffer != null ) {
            disposeImageGraphics() ;
        }
        else {
            disposeScreenGraphics() ;
        }
    }

    private BufferedImage createBackBuffer() {
        Dimension size = this.container.getSize() ;
        return createBufferedImage(size.width, size.height) ;
    }

    private Graphics createScreenGraphics() {
        Dimension size = this.container.getSize() ;
        Graphics g = container.getGraphics() ;
        g.clearRect(0,0,size.width, size.height);
        Insets insets = this.container.getInsets() ;
        if ( (insets.top|insets.left) != 0 ) 
            g.translate(insets.left, insets.top) ;
        return g ;
    }

    private void disposeScreenGraphics() {
        this.gc.dispose() ;
        this.gc = null ;
    }
    
    private Graphics createImageGraphics() {
        this.backBuffer = createBackBuffer() ;
        Graphics2D g = (Graphics2D)this.backBuffer.getGraphics() ;
        Color bg = this.container.getBackground() ;
        clearToBackgroundColor(g, bg, 0, 0, this.backBuffer.getWidth(null),
                                            this.backBuffer.getHeight(null)) ;
/*
        g.setBackground(bg) ;
        g.clearRect(0,
                    0,
                    this.backBuffer.getWidth(null),
                    this.backBuffer.getHeight(null)) ;
*/
        return g ;
    }

    private void disposeImageGraphics() {
        this.gc.dispose() ;
        this.gc = null ;
        if ( this.isGraphicsTestcase ) {
            Graphics g = createScreenGraphics() ;
            try {
                g.drawImage(this.backBuffer,0,0,null) ;
            }
            finally {
                g.dispose() ;
            }
        }
    }
    

    private BufferedImage toBufferedImage(Image image) {
        if ( image instanceof BufferedImage ) 
            return ((BufferedImage)image) ;
        BufferedImage bi = createBufferedImage(image.getWidth(null),
                                               image.getHeight(null)) ;
        Graphics big = bi.getGraphics() ;
        big.drawImage(image, 0,0, null) ;
        big.dispose() ;

        return bi ;
    }
        
    static Image trackImage(Image img) {
        return trackImage(img, new Container());
    }

    static Image trackImage(Image img, Component comp) {
        MediaTracker tracker = new MediaTracker(comp);
        if(img != null) {
            // Track an image to make sure it loads completely.
            tracker.addImage(img, 0);
            try {
                tracker.waitForID(0);
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
        return img ;
    }
}

/**
 * <code>TestArguments</code> abstracts the arguments for all the testcases
 */
class TestArguments {
    static final String DEFAULT_TAG = "default" ;
    static final String[] STRING_ARRAY = new String[0] ;

    String[] defaultArgs = STRING_ARRAY ;
    // Map<String, String[]>
    // Map<classname.methodname, args>
    Map argsMap = new HashMap() ;

    TestArguments() {
        this(TestContext.getInstance().getStringValue(
                                       TestContext.TEST_ARGS_FILENAME));
    }

    TestArguments(String file) {
        if ( file == null )
            return ;
        try {
            init(new FileInputStream(file)) ;
        }
        catch ( Exception ex ) {
            System.err.println("Unable to load args file "+file) ;
        }
    }

    void init(InputStream stream) {
        BufferedReader reader = null ;
        String line = null ;
        String start_tag = null ;
        String class_name = null ;
        String method_name = null ;
        java.util.List args = new ArrayList() ;
        try {
            reader = new BufferedReader(new InputStreamReader(stream));
            while ((line = reader.readLine()) != null ) {
                line = line.trim() ; // trim white spaces
                if ( line.startsWith("</") ) { // check if end tag
                    // process an end tag only if
                    // we had processed a start tag AND
                    // the end tag markup is the same as the start tag
                    if ( start_tag != null &&
                         line.substring(2).startsWith(start_tag)) {
                        if ( DEFAULT_TAG.equals(start_tag) ) {
                            this.defaultArgs = (String[])
                                args.toArray(STRING_ARRAY);
                        }
                        else {
                            argsMap.put(class_name+"."+method_name,
                                        args.toArray(STRING_ARRAY));
                        }
                        args.clear();
                    }
                    start_tag = null;
                }
                else
                if ( line.startsWith("<args>")) { // check for <args>
                    int end_index = line.indexOf("</args>") ;
                    if ( start_tag == null || end_index == -1) {
                        continue ; // we have not seen a start tag or
                                   // the end markup for args is missing
                    }
                    String arg = line.substring("<args>".length(),
                                                end_index).trim() ;
                    if ( arg != null && arg.length() > 0 ) {
                        args.add(arg) ;
                    }
                }
                else
                if ( line.startsWith("<") ) { // check for start tag
                    int end_index = line.indexOf(">") ;
                    if ( end_index < 0 )
                        continue ; // no markup termination
                    line = line.substring(1, end_index).trim() ;
                    if ( line.length() < 0 )
                        continue ; // no content inside markup
                    end_index = line.lastIndexOf('-') ;
                    start_tag = line ;
                    // start_tag is either a valid testcase or "default"
                    // tag
                    if ( end_index > 0 ) {
                        class_name =
                        line.substring(0, end_index).replace('-', '.');
                        method_name = line.substring(end_index+1);
                    }
                    else
                    if ( !DEFAULT_TAG.equals(line) ) {
                        start_tag = null ;
                    }
                }
            }
        }
        catch ( Exception ex ) {
        }
        finally {
            try {
                reader.close();
            }
            catch( Exception ex) {
            }
        }
    }

    String[] getArgs(Class testClass, String methodName) {
        String[] args = null ;
        args = (String[])
            this.argsMap.get(testClass.getName()+"."+methodName) ;
        if ( args != null ) 
            return args ;
        return defaultArgs ;
    }
}
