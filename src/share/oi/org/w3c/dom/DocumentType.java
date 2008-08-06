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
 * Copyright (c) 2000 World Wide Web Consortium,
 * (Massachusetts Institute of Technology, Institut National de
 * Recherche en Informatique et en Automatique, Keio University). All
 * Rights Reserved. This program is distributed under the W3C's Software
 * Intellectual Property License. This program is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.
 * See W3C License http://www.w3.org/Consortium/Legal/ for more details.
 * Copyright (c) 2004 World Wide Web Consortium,
 *
 * (Massachusetts Institute of Technology, European Research Consortium for
 * Informatics and Mathematics, Keio University). All Rights Reserved. This
 * work is distributed under the W3C(r) Software License [1] in the hope that
 * it will be useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * [1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
 */

package org.w3c.dom;

/**
 * Each <code>Document</code> has a <code>doctype</code> attribute whose value 
 * is either <code>null</code> or a <code>DocumentType</code> object. The 
 * <code>DocumentType</code> interface in the DOM Core provides an interface 
 * to the list of entities that are defined for the document, and little 
 * else because the effect of namespaces and the various XML schema efforts 
 * on DTD representation are not clearly understood as of this writing.
 * <br><b>Note:</b> If a parser expands entity references, the information
 * contained in the corresponding Entity node becomes redundant. In the interest
 * of footprint and performance, JSR 280 allows implementations to exclude
 * Entity nodes corresponding to expanded entity references from
 * the DOM tree if those references have been fully expanded by the parser. 
 * <p>The DOM Level 2 and Level 3 do not support editing <code>DocumentType</code> nodes.
 * <p>See also the <a href='http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113'>Document Object Model (DOM) Level 2 Core Specification</a>.
 */
public interface DocumentType extends Node {
    /**
     * The name of DTD; i.e., the name immediately following the 
     * <code>DOCTYPE</code> keyword.
     * @return the name of the DTD
     */
    public String getName();

    /**
     * A <code>NamedNodeMap</code> containing the general entities, both 
     * external and internal, declared in the DTD. Parameter entities are 
     * not contained. Duplicates are discarded. For example in: 
     * <pre>&lt;!DOCTYPE 
     * ex SYSTEM "ex.dtd" [ &lt;!ENTITY foo "foo"&gt; &lt;!ENTITY bar 
     * "bar"&gt; &lt;!ENTITY bar "bar2"&gt; &lt;!ENTITY % baz "baz"&gt; 
     * ]&gt; &lt;ex/&gt;</pre>
     *  the interface provides access to <code>foo</code> 
     * and the first declaration of <code>bar</code> but not the second 
     * declaration of <code>bar</code> or <code>baz</code>. Every node in 
     * this map also implements the <code>Entity</code> interface.
     * <br>The DOM Level 2 does not support editing entities, therefore 
     * <code>entities</code> cannot be altered in any way.
     * @return A <code>NamedNodeMap</code> containing the general entities 
     * contained in the DTD
     */
    public NamedNodeMap getEntities();

    /**
     * A <code>NamedNodeMap</code> containing the notations declared in the 
     * DTD. Duplicates are discarded. Every node in this map also implements 
     * the <code>Notation</code> interface.
     * <br>The DOM Level 2 does not support editing notations, therefore 
     * <code>notations</code> cannot be altered in any way.
     * @return A <code>NamedNodeMap</code> containing the notations declared in the DTD
     */
    public NamedNodeMap getNotations();

    /**
     * The public identifier of the external subset.
     * @since DOM Level 2
     * @return the public identifier of the external subset
     */
    public String getPublicId();

    /**
     * The system identifier of the external subset.
     * @since DOM Level 2
     * @return The system identifier of the external subset
     */
    public String getSystemId();

    /**
     * The internal subset as a string, or <code>null</code> if there is none.
     * <p><b>Note:</b> The actual content returned depends on how much
     * information is available to the implementation. This may vary
     * depending on various parameters, including the XML processor used to
     * build the document.
     * @since DOM Level 2
     * @return a String containing a representation of the internal subset
     */
    public String getInternalSubset();

}
