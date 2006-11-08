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

/**
 * @version @(#)DescriptorFileDD.java	1.10 05/10/20
 */

package com.sun.provisioning.adapters.oma;

import java.io.*;
import java.net.URLEncoder;
import java.net.URL;
import java.util.*;

import org.w3c.dom.*;
import org.xml.sax.*;
import javax.xml.parsers.*;
import javax.xml.transform.*;
import javax.xml.transform.dom.*;
import javax.xml.transform.stream.StreamResult;

import javax.provisioning.*;
import javax.provisioning.adapter.*;
import com.sun.provisioning.Utils;


/**
 * Represents a modifiable copy of a DD file (see
 * OMA-Download-OTA-v1_0-20020912-a specification).
 */
public class DescriptorFileDD extends DescriptorFile {

    // this is an official MIME type for this kind of the descriptor
    static final String DD_MIME_TYPE = "application/vnd.oma.dd+xml";
    // DDX namespace
    static final String DDX_NS = "http://www.sun.com/appmanager/ddx";
    // this DOM tree represents the descriptor
    private Document ddDocument;
    // this node corresponds to the root MEDIA node
    private Node mediaNode;

    /**
     * These constants define the element names used by OMA
     */
    public static final String ATTR_TYPE	= "type";
    public static final String ATTR_SIZE	= "size";
    public static final String ATTR_OBJECT_URI  = "objectURI";
    public static final String ATTR_INSTALL_NOTIFY_URI = "installNotifyURI";
    public static final String ATTR_NEXT_URL	= "nextURL";
    public static final String ATTR_DD_VERSION	= "DDversion";
    public static final String ATTR_NAME	= "name";
    public static final String ATTR_DESCRIPTOR	= "description";
    public static final String ATTR_VENDOR	= "vendor";
    public static final String ATTR_INFO_URL	= "infoURL";
    public static final String ATTR_ICON_URI	= "iconURI";
    public static final String ATTR_INSTALL_PARAM	= "installParam";

    /**
     * Constructs a new object representation of the specified Download
     * Descriptor
     */
    public DescriptorFileDD(URL descriptorURL, String contentType) throws IOException {
        super(descriptorURL, contentType);

	InputStream is = 
	    new BufferedInputStream(getURL().openStream());
	try {
	    parseStream (is);
	} catch (Exception e) {
	    Utils.log("Failed to parse the descriptor "+descriptorURL+
		", exception is "+e.getMessage());
	    if (e instanceof IOException) {
		throw (IOException)e;
	    }
	}
    }

    /**
     * Parses the download descriptor
     */
    private void parseStream(InputStream in) throws SAXException, IOException, 
	ParserConfigurationException {

	DocumentBuilderFactory dbFactory = DocumentBuilderFactory.newInstance();
	// needed to correctly process DDX elements
	dbFactory.setNamespaceAware(true);
	DocumentBuilder db = dbFactory.newDocumentBuilder();
	this.ddDocument = db.parse(in);
	NodeList nl = ddDocument.getElementsByTagName("media");
	// getting the root media node
	mediaNode = nl.item(0);
	Utils.log("OMA descriptor parsed successfully");
    }

    public List getContentFiles() {
	return Arrays.asList(new String[]{getNodeTextValue(ATTR_OBJECT_URI)});
    }

    public String getAppProperty(String name) {
	return getNodeTextValue(name);
    }

    public void setAppProperty(String name, String value) {
	if (name.equalsIgnoreCase(ATTR_TYPE)) {
	    // we will not support the updating of the type attribute because
	    // there can be more than one its instance
	    return;
	}
	updateNodeTextValue(name, value);
    }

    
    /**
     * Returns the string array of all MIME types specified in the descriptor
     *
     * @return String array of MIME types
     */
    public String[] getTypes() {
	NodeList children = mediaNode.getChildNodes();
	ArrayList list = new ArrayList();
	for (int i=0, maxC = children.getLength(); i<maxC; i++) {
	    Node n = children.item(i);
	    if ((n instanceof Element) && n.getNodeName().equalsIgnoreCase("type")) {
		list.add(getNodeTextValue(n));
	    }
	}
	return (String[])list.toArray(new String[list.size()]);
    }

    /**
     * Returns the value of the objectURI element
     *
     * @return objectURI element value
     */
    public String getObjectURI() {
	return getNodeTextValue(ATTR_OBJECT_URI);
    }

    /**
     * Sets a new value for objectURI element
     *
     * @param newURI new objectURI value
     */
    public void setObjectURI(String newURI) {
	updateNodeTextValue(ATTR_OBJECT_URI, newURI);
    }

    /**
     * Returns the value of the installNotifyURL
     *
     * @return installNotifyURL element value
     */
    public String getInstallNotifyURI() {
	return getNodeTextValue(ATTR_INSTALL_NOTIFY_URI);
    }

    /**
     * Returns the version of the depolyment descriptor
     *
     * @return deployment descriptor version
     */
    public String getDDVersion() {
	return getNodeTextValue(ATTR_DD_VERSION);
    }

    /**
     * Returns the installParam attribute value
     *
     * @return installParam attribute value
     */
    public String getInstallParam() {
	return getNodeTextValue(ATTR_INSTALL_PARAM);
    }

    /**
     * Sets the installParam attribute value
     *
     * @param newParam installParam attribute value
     */

    public void setInstallParam(String newParam) {
	updateNodeTextValue(ATTR_INSTALL_PARAM, newParam);
    }

    /**
     * Returns the Icon URI
     *
     * @return Icon URI
     */
    public String getIconURI() {
	return getNodeTextValue(ATTR_ICON_URI);
    }

    public void setIconURI(String newURI) {
	updateNodeTextValue(ATTR_ICON_URI, newURI);
    }

    public void setInstallNotifyURI(String newURI) {
	updateNodeTextValue(ATTR_INSTALL_NOTIFY_URI, newURI);
    }

    /**
     * Returns the text contents of the specified node. 
     *
     * @return node text contents 
     */
    private String getNodeTextValue(Node node) {
	StringBuffer buf = new StringBuffer();
	NodeList subnodes = node.getChildNodes();
	for (int i=0, maxI = subnodes.getLength(); i<maxI; i++) {
	    Node subnode = subnodes.item(i);
	    if (subnode instanceof Text) {
		buf.append(subnode.getNodeValue());
	    }
	}
	return buf.toString();
    }

    /**
     * Writes the descriptor in XML format into the specified output stream
     *
     * @param out target output stream
     */
    public void writeContents(OutputStream out) {
	try {
	    DOMSource src = new DOMSource(ddDocument);
	    StreamResult result = new StreamResult(out);
	    Transformer t = TransformerFactory.newInstance().newTransformer();
	    t.transform(src, result);
	} catch (Exception e) {
	    Utils.log("Failed to produce XML output : "+e.getMessage());
	}
    }

    /**
     * Returns the text contents of the specified element. Note : the
     * method assumes that there is only one occurence of the specified
     * element.
     *
     * @return element text contents 
     */
    private String getNodeTextValue(String nodeName) {
	NodeList children = mediaNode.getChildNodes();
	for (int i=0, maxC = children.getLength(); i<maxC; i++) {
	    Node n = children.item(i);
	    if ((n instanceof Element) && n.getNodeName().equalsIgnoreCase(nodeName)) {
		return getNodeTextValue(n);
	    }
	}
	return null;
    }

    /**
     * Updates the element contents. If there is no such element in the
     * descriptor, the method creates a new element with the specified name by
     * adding it to the root MEDIA node and sets its text contents to the
     * specified value.
     *
     * @param nodeName element name
     * @param nodeValue element contents
     */
    private void updateNodeTextValue(String nodeName, String nodeValue) {
	NodeList matchList = ddDocument.getElementsByTagName(nodeName);
	int len = matchList.getLength();
	if (len > 1) {
	    // we will not update an element if there is more than one its
	    // instance
	    return;
	}
	if (len == 0) {
	    // no such element - creating new one
	    Element newElement = ddDocument.createElement(nodeName);
	    Text textNode = ddDocument.createTextNode(nodeValue);
	    newElement.appendChild(textNode);
	    mediaNode.appendChild(newElement);
	    return;
	}

	Element e = (Element)matchList.item(0);
	if (e.hasChildNodes()) {
	    NodeList c = e.getChildNodes();
	    for (int i=0, l=c.getLength(); i<l; i++) {
		e.removeChild(c.item(i));
	    }
	} 
	e.appendChild(ddDocument.createTextNode(nodeValue));
    }

    void rewriteDeliveryURLs(FulfillmentTask ft, String adapterURL) {
	String base = adapterURL+"/"+URLEncoder.encode(ft.getFulfillmentID())+"/";
	setObjectURI(base+AdapterOMA.OBJECT_DIR);
	String newIconURI = null;
	if (getIconURI() != null) {
	    // updating the icon URL only if there is one defined
	    newIconURI = base+AdapterOMA.ICON_DIR;
	    setIconURI(base+AdapterOMA.ICON_DIR);
	    // bugfix for 4800468
	    // rewriteInstallParam(ft, newIconURI);
	}
	setInstallNotifyURI(base+AdapterOMA.NOTIFY_DIR);
	return;
    }

    private void rewriteInstallParam(FulfillmentTask ft, String iconURI) {
	String ip = getInstallParam();
	if (ip == null) {
	    // failed, can not continue without this attribute
	    Utils.log("No installParam specified in the desriptor for bundle id="+
		ft.getBundleDescriptor().getBundleID()+": "+getInstallParam());
	    return;
	}
	StringTokenizer st = new StringTokenizer(ip, ",");
	try {
	    String xletName = st.nextToken();
	    st.nextToken();
	    String xletClass = st.nextToken();
	    setInstallParam(xletName+","+iconURI+","+xletClass);
	} catch (NoSuchElementException e) {
	    Utils.log("Invalid installParam specified in the desriptor for bundle id="+
		ft.getBundleDescriptor().getBundleID()+": "+getInstallParam());
	}
    }


    Deliverable getIcon() {
	return getResource(getIconURI(), null);
    }

    Deliverable getObject() {
	return getResource(getObjectURI(), getTypes()[0]);
    }

    /**
     * Reweites the dependencies defined in the descriptor
     * @param adapter OMA adapter (used for callbacks)
     */
    void rewriteDependencies(AdapterOMA adapter, DeliveryContext context, String baseURL) {
	NodeList depNodes = ddDocument.getElementsByTagNameNS(DDX_NS,
		"dependencies");
	for (int i=0, len=depNodes.getLength(); i<len; i++) {
	    Element depElement = (Element)depNodes.item(i);
	    rewriteDependencyHref(depElement.getElementsByTagNameNS(DDX_NS, "content"), 
		    adapter, context, baseURL);
	    rewriteDependencyHref(depElement.getElementsByTagNameNS(DDX_NS, "export"), 
		    adapter, context, baseURL);
	    rewriteDependencyHref(depElement.getElementsByTagNameNS(DDX_NS, "mime"), 
		    adapter, context, baseURL);
	}
    }

    /**
     * Replaces the &quot;href&quot; attribute value of the specified element with
     * the delivery URL of the bundle whose content-id is equal to the attribute value.
     * @param depNode DOM element corresponding to the <tt>dependencies</tt> element.
     * @param adaptere OMA adapter reference
     */
    private void rewriteDependencyHref(NodeList depElements, AdapterOMA adapter,
	    DeliveryContext context, String baseURL) {

	for (int i=0, len=depElements.getLength(); i<len; i++) {
	    Element depElement = (Element)depElements.item(i);
	    String refContentID = depElement.getAttribute("href");
	    if (refContentID == null) {
		return;
	    }
	    String url = adapter.getDeliveryURLForCID(refContentID, context);
	    if (url != null) {
		depElement.setAttribute("href", baseURL+"/"+url);
	    }
	}
    }
    
    private Deliverable getResource(String ref, String mimeType) {
	try {
	    URL resURL = new URL(getURL(), ref);
	    if (mimeType == null) {
		return new Deliverable(resURL, MimeTool.getMimeTypeFor(resURL));
	    } else {
		return new Deliverable(resURL, mimeType);
	    }
	} catch (IOException e) {
	    Utils.log("Failed to access "+ref);
	}
	return null;
    }
}
