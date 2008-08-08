/*
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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

/*
 * Copyright (c) 2006 World Wide Web Consortium,
 *
 * (Massachusetts Institute of Technology, European Research Consortium for
 * Informatics and Mathematics, Keio University). All Rights Reserved. This
 * work is distributed under the W3C(r) Software License [1] in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * [1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
 */

package org.w3c.dom.events;

/**
 * <p>A progress event occurs when the user agent makes progress in some data
 * transfer operation, such as loading a resource from the web via
 * XMLHttpRequest. 
 *
 * <p><b>Note: </b> This interface has not yet been accepted for
 * inclusion in the W3C DOM 3 Event specification. It may be removed from the
 * JSR 280 specification or added to a different namespace in a
 * subsequent version of the specification.</p>
 *
 * Other specifications which have a
 * use case for these events <em>should</em> define when ProgressEvent events
 * are dispatched.</p>
 *
 * <ul>
 * <li>The user agent <em>must</em> dispatch a <code>start</code> event when a
 *     relevant operation has begun. </li>
 * <li>The user agent <span class="term"><em>may</em></span> dispatch one or
 *     more <code>progress</code> events while a network operation is taking
 *     place.</li>
 * <li>If the operation fails to complete the user agent <em>must</em>
 *     dispatch either an <code>error</code> event, if the failure to complete
 *     was due to an error (such as a timeout, or network error), or an
 *     <code>abort</code> event if the operation was deliberately cancelled
 *     (e.g. by user interaction or through a script call).</li>
 * <li>If the operation successfully completes, the user agent <em>must</em>
 *     dispatch a <code>load</code> event. </li>
 * </ul>
 * <p>In short, there must be at least one <code>start</code> event, followed by
 * zero or more <code>progress</code> events, followed by one event which may be
 * any of <code>error</code>, <code>abort</code> or <code>load</code>, according
 * to the outcome of the operation.</p>
 *
 * <h3 id="Event">Event definitions</h3>
 *
 * <p>The following events are defined in this specification</p>
 *
 * <table border="1">
 *  <caption></caption>
 *  <tbody>
 *    <tr>
 *      <th>Name</th>
 *      <th>Description</th>
 *      <th>How many?</th>
 *      <th>When?</th>
 *    </tr>
 *    <tr>
 *      <td><code>loadstart</code></td>
 *      <td>The operation has begun</td>
 *      <td>once</td>
 *      <td>Must be dispatched first</td>
 *    </tr>
 *    <tr>
 *      <td><code>progress</code></td>
 *      <td>The operation is in progress</td>
 *      <td>zero or more</td>
 *      <td>May be dispatched zero or more times after a <code>loadstart</code>
 *        event, before any of error, abort or load events are dispatched</td>
 *    </tr>
 *    <tr>
 *      <td><code>error</code></td>
 *      <td>The operation failed to complete, e.g. as a result of a network
 *        error</td>
 *      <td>never or once</td>
 *      <td rowspan="3" valign="middle">Exactly one of these must be
 *      dispatched</td>
 *    </tr>
 *    <tr>
 *      <td><code>abort</code></td>
 *      <td>The operation was cancelled, e.g. as a result of user
 *      interaction</td>
 *      <td>never or once</td>
 *    </tr>
 *    <tr>
 *      <td><code>load</code></td>
 *      <td>The operation successfully completed</td>
 *      <td>never or once</td>
 *    </tr>
 *  </tbody>
 * </table>
 *
 * <p>These events <em>must not</em> bubble, and <em>must not</em> be 
 * cancelable.</p> 
 * 
 * <p>These events trigger event listeners attached on <code>Element</code>
 * nodes for that event and on the capture and target phases. 
 *
 * <p>No default action is defined for these events. </p>
 *
 * <p>These events are in the <code>null</code> namespace. Two kinds of 
 * initialization methods are provided: one in which the namespace is 
 * required (and must be <code>null</code>) and one which assigns the 
 * <code>null</code> namespace automatically, This specification does
 * not recommend use of one method over the other, and authors may choose
 * whichever method suits them better for any given usage.
 *
 */
public interface ProgressEvent extends Event {

    /**
     * Specifies whether the total size of the transfer is known.
     */
    public boolean getLengthComputable();

    /**
     * Specifies the number of bytes downloaded since the beginning of the 
     * download. This refers to the content, excluding headers and overhead 
     * from the transaction,
     * and where there is a content-encoding or transfer-encoding refers
     * to the number of bytes to be transferred, i.e. with the relevant
     * encodings applied. For more details on HTTP see [RFC2616].
     */
    public int getLoaded();

    /**
     * Specifies the expected total number of bytes of the content 
     * transferred in the operation. Where the size of the transfer is for
     * some reason unknown, the value of this attribute <em>must</em> be zero.
     */
    public int getTotal();

    /**
     * The initProgressEvent method is used to initialize the value of a 
     * progress event created through the DocumentEvent interface. 
     * If this method is called multiple times, the final invocation takes 
     * precedence .
     *
     * @param typeArg  
     *   This <em>must</em> be one of <code>loadstart</code>, 
     *   <code>progress</code>, <code>error</code>, <code>abort</code>,
     *   <code>load</code>. If it is not one of those values then this
     *   specification does not define the resulting event.
     * @param canBubbleArg
     *   This <em>must</em> be <var>false</var>. Where a
     *   value of <var>true</var> is passed, implementations
     *   <em>must</em> override that and change the value to
     *   <var>false</var>.
     * @param cancelableArg
     *   This <em>must</em> be <var>false</var>. Where a
     *   value of <var>true</var> is passed, implementations 
     *   <em>must</em> override that and change the value to
     *   <var>false</var>.
     * @param lengthComputableArg
     *   If the implementation has reliable information about
     *   the value of <code>total</code>, then this should be <var>true</var>. 
     *   If the implementation does not have reliable information about
     *   the value of <code>total</code>, this should be <var>false</var>.
     * @param loadedArg
     *   Specifies the total number of bytes already loaded. If this value 
     *   is not a non-negative number, 
     *   the implementation <em>must</em> change it to zero.
     * @param totalArg
     *   Specifies the total number of bytes to be
     *   loaded. If <code>lengthComputable</code> is <var>false</var>, 
     *   this <em>must</em> be zero. If any other parameter is passed, and
     *   <code>lengthComputable</code> is <var>false</var>, the implementation 
     *   <em>must</em> override this and set the value to zero. If
     *   <code>lengthComputable</code> is <var>true</var>, and the value 
     *   of this parameter is not a non-negative number, the implementation 
     *   <em>must</em> set <code>lengthComputable</code> to <var>false</var>
     *   and the value of <code>total</code> to zero. 
     */
    public void initProgressEvent(String typeArg,
				  boolean canBubbleArg, 
				  boolean cancelableArg,
				  boolean lengthComputableArg,
				  int loadedArg, 
				  int totalArg);


    /**
     * The initProgressEventNS method is used to initialize the value of a 
     * namespaced progress event created through the DocumentEvent interface.
     * This method may only be called before the progress event has been 
     * dispatched via the dispatchEvent method, though it may be called 
     * multiple times during that phase if necessary. If called multiple 
     * times, the final invocation takes precedence.
     *
     * @param namespaceURIArg 
     *   Specifies the URI for the namespace of the event.
     *   For all events defined in this specification, the
     *   value of this parameter is <code>null</code>.
     * @param typeArg
     *   This must be one of <code>loadstart</code>, 
     *   <code>progress</code>, <code>error</code>,
     *   <code>abort</code>, <code>load</code>. If it is not one
     *   of those values then this specification does not define 
     *   the resulting event.
     * @param canBubbleArg
     *   This <em>must</em> be <var>false</var>. Where a
     *   value of <var>true</var> is passed, implementations
     *   <em>must</em> override that and change the value to
     *   <var>false</var>.
     * @param cancelableArg
     *   This <em>must</em> be <var>false</var>. Where a
     *   value of <var>true</var> is passed, implementations
     *   <em>must</em> override that and change the value to
     *   <var>false</var>.
     * @param lengthComputableArg
     *   If the implementation has reliable information about
     *   the value of total, then this should be <var>true</var>. If the
     *   implementation does not have reliable information about
     *   the value of total, this should be <var>false</var>.
     * @param loadedArg
     *   This parameter specifies the total number of bytes
     *   already loaded.
     *   If this value is not a non-negative number, the implementation 
     *   <em>must</em> change it to zero.
     * @param totalArg
     *   This specifies the total number of bytes to be
     *   loaded. If <code>lengthComputable</code> is <var>false</var>, 
     *   this <em>must</em> be zero. If any other parameter is passed,
     *   and <code>lengthComputable</code> is <var>false</var>, the
     *   implementation <em>must</em> override this and set the value to
     *   zero. If <code>lengthComputable</code> is <var>true</var>, and 
     *   the value of this parameter is not a non-negative number, the 
     *   implementation <em>must</em> set <code>lengthComputable</code> 
     *   to <var>false</var> and the value of <code>total</code> to zero.
     *
     */
    public void initProgressEventNS(String namespaceURIArg, 
                                    String typeArg,
                                    boolean canBubbleArg, 
                                    boolean cancelableArg,
                                    boolean lengthComputableArg,
                                    int loadedArg, 
                                    int totalArg);
}
