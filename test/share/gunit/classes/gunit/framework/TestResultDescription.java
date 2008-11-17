/*
 * @(#)TestResultDescription.java	1.5 06/10/10
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

import java.io.* ;
import java.util.* ;
import java.awt.* ;
import java.net.* ;

/**
 * <code>TestResultDescription</code> describes the test result in text
 * and/or image formats. The testcase developer needs to provide the 
 * following for testcases that needs user verification.
 * <ul>
 *  <li>create a file with the same name as the TestCase java file with
 *      the <b>.java</b> extension replaced with <b>.xml</b> in the same
 *      same directory as the java source file</li>
 *  <li>Inside the xml file, specify the text and image description for
 *      each of the testcase method that needs user verification as 
 *      mentioned in the section <b>Format</b> below</li>
 * </ul>
 * <h4>Format</h4>
 * <pre>
 * &lt;methodName&gt;
 *    &lt;test&gt;
 *      <b>test description line 1</b>
 *      <b>test description line 2</b>
 *         ...
 *      <b>test description line n</b>
 *    &lt;/test&gt;
 *    &lt;image&gt;
 *      <b>URL of the imagefile</b>
 *    &lt;/image&gt;
 * &lt;/methodName&gt;
 * </pre>
 */
public class TestResultDescription {
    // Map<Class,Map<String,TestResultDescription>>
    static Map descriptionMap = new HashMap() ;
    static final TestResultDescription defaultDescription = 
        new TestResultDescription() ;

    static TestResultDescription getInstance(Class c, String methodName) {
        Map class_desc_map = (Map) descriptionMap.get(c) ;
        if ( class_desc_map == null ) {
            String file = c.getName() ;
            int index = file.lastIndexOf('.') ;
            if ( index > 0 ) {
                // there is a package name, extract just the class name
                file = file.substring(index+1) ;
            } 
            file = file + ".xml" ;
            InputStream stream = c.getResourceAsStream(file) ;
            class_desc_map = new HashMap() ; // create empty one
            if ( stream != null )
                XMLParser.parse(stream, class_desc_map) ;
            descriptionMap.put(c, class_desc_map) ;
        }

        TestResultDescription desc = (TestResultDescription)
            class_desc_map.get(methodName) ;
        if ( desc != null )
            return desc ;
        return defaultDescription ;
    }

    private String text ;
    private String imageURL ;

    private TestResultDescription() {
        this(null,null);
    }

    private TestResultDescription(String text, String imageURL) {
        this.text = text ;
        this.imageURL = imageURL ;
    }

    public String getText() {
        return this.text ;
    }

    public String getImageURL() {
        return this.imageURL ;
    }

    public Image getImage() {
        Image ret = null ;
        if ( this.imageURL == null )
            return null ;
        try {
            URL url = new URL(this.imageURL);
            ret = Toolkit.getDefaultToolkit().createImage(url) ;
            BaseTestCase.trackImage(ret) ;
        }
        catch ( Exception ex ) {
        }
        return ret ;
    }

    static class XMLParser {
        private static final String TEXT_START_TAG  = "<text>" ;
        private static final String IMAGE_START_TAG = "<image>" ;
        private static final String TEXT_END_TAG    = "</text>" ;
        private static final String IMAGE_END_TAG   = "</image>" ;
        
        static void parse(InputStream stream, Map map) {
            BufferedReader reader = null ;
            String line = null ;
            String method_start_tag = null ;
            boolean in_text_tag = false ;
            boolean in_image_tag = false ;
            String image_content = null ; 
            StringBuffer text_content = new StringBuffer("") ;
            try {
                reader = new BufferedReader(new InputStreamReader(stream));
                while ((line = reader.readLine()) != null ) {
                    line = line.trim() ; // trim white spaces
                    if ( line.startsWith("</") ) { // check if end tag
                        if ( TEXT_END_TAG.equals(line) && in_text_tag ) {
                            in_text_tag = false ;
                        } 
                        else
                        if ( IMAGE_END_TAG.equals(line) && in_image_tag ){
                            in_image_tag = false ;
                        } 
                        else 
                        if ( method_start_tag != null &&
                             line.substring(2).startsWith(method_start_tag)) {
                            // process an end tag only if
                            // we had processed a start tag AND
                            // the end tag markup is the same as the start tag

                            TestResultDescription desc = 
                            new TestResultDescription(text_content.toString(),
                                                      image_content) ;
                            map.put(method_start_tag, desc) ;    

                            text_content.delete(0, text_content.length());
                            image_content = null ;
                            method_start_tag = null;
                            in_text_tag = false ; // force it
                            in_image_tag = false ; // force it
                        }
                    }
                    else
                    if ( line.startsWith(TEXT_START_TAG)) {
                        in_text_tag = true ;
                    }
                    else
                    if ( line.startsWith(IMAGE_START_TAG)){
                        in_image_tag = true ;
                    }
                    else
                    if ( in_text_tag ) {
                        text_content.append(line+" ") ;
                    }
                    else
                    if ( in_image_tag ) {
                        image_content = line ;
                    }
                    else
                        if ( line.startsWith("<") ) { // check for start tag
                            int end_index = line.indexOf(">") ;
                            if ( end_index < 0 )
                                continue ; // no markup termination
                            line = line.substring(1, end_index).trim() ;
                            if ( line.length() < 0 )
                                continue ; // no content inside markup
                            method_start_tag = line ;
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
    }
}

