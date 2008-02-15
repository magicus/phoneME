/*
 *  
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.ukit.jaxp;

/**
 * SAX Attributes interface implementation.
 */

/* package */ abstract class Attrs implements org.xml.sax.Attributes
{
	private static final String CDATA = "CDATA";
	/**
	 * Attributes string array. Each individual attribute is represented by 
	 * several strings.
	 * In order to find attribute by the attribute index, the attribute 
	 * index MUST be multiplied by 'attrStringsNum'.
	 */
	protected String[]	mItems;
	final static protected int qnameOff = 0, typeOff = 1, valueOff = 2;
	final protected int attrStringsNum; 

	/**
	 * Number of attributes in the object.
	 */
	protected char mCount;

	/**
	 * Constructor.
	 */
	protected Attrs( int attrStringsNum ) {
		this.attrStringsNum = attrStringsNum;
		// The default number of attributes capacity is 8.
		mItems = new String[ 8 * attrStringsNum ];
		clean();
	}
	
	public void clean(){
		mCount = 0;
	}
	
	public int add( String qname, String value ){
		// assert( qname != null && value != null );
		if( Parser.DEBUG_OUT != null ){
			Parser.DEBUG_OUT.println( "Attrs.add( '" + value + "' )" );
		}
		if( (mCount + 1) * attrStringsNum > mItems.length ){
			String[] items = new String[ mItems.length + 8 * attrStringsNum ];
			System.arraycopy(mItems, 0, items, 0, mItems.length);
			mItems = items;
		}
		mItems[ mCount * attrStringsNum + qnameOff ] = qname;
		mItems[ mCount * attrStringsNum + valueOff ] = value;
		
		// set CDATA as a default type (will be rewritten later if known)
		// (3.3.3) All attributes for which no declaration has been read SHOULD be 
		// treated by a non-validating processor as if declared CDATA.
		mItems[ mCount * attrStringsNum + typeOff ] = CDATA;
		return mCount++;
	}

	public void setType(int idx, String type) {
		// assert( type != null );
		if( idx < 0 || idx >= mCount )
			throw new ArrayIndexOutOfBoundsException();
		if( Parser.DEBUG_OUT != null ){
			Parser.DEBUG_OUT.println( "Attrs.setType( '" + mItems[ idx * attrStringsNum + qnameOff ] + "', '" + type + "' )" );
		}
		mItems[ idx * attrStringsNum + typeOff ] = type;
		if( !(type.charAt(0) == 'C' && CDATA.equals(type)) ){
			// add non-CDATA conversion (3.3.3)
			String value = getValue(idx);
			StringBuffer b = new StringBuffer( value.length() );
			int nonSpaceCount = 0;
			for( int i = 0; i < value.length(); i++){
				if( value.charAt(i) != ' ' ){
					nonSpaceCount++;
				} else if( nonSpaceCount > 0 ){
					// flush non-space chars with one trailing space
					b.append(value.substring(i - nonSpaceCount, i + 1));
					nonSpaceCount = 0;
				}
			}
			if( nonSpaceCount > 0 )
				b.append(value.substring(value.length() - nonSpaceCount, value.length()));
			// delete the last space if there exist
			if( b.length() > 0 && b.charAt(b.length() - 1) == ' ' )
				b.setLength(b.length() - 1);
			// set value
			if( Parser.DEBUG_OUT != null ){
				Parser.DEBUG_OUT.println( "\t'" + getValue(idx) + "' -> '" + b.toString() + "'" );
			}
			mItems[ idx * attrStringsNum + valueOff ] = b.toString();
		}
	}
	
	/**
	 * Return the number of attributes in the list.
	 *
	 * <p>Once you know the number of attributes, you can iterate
	 * through the list.</p>
	 *
	 * @return The number of attributes in the list.
	 * @see #getURI(int)
	 * @see #getLocalName(int)
	 * @see #getQName(int)
	 * @see #getType(int)
	 * @see #getValue(int)
	 */
	public int getLength() {
		return mCount;
	}

	final protected String get(int index, int stringOff) {
		return (index >= 0 && index < mCount)? 
					mItems[index * attrStringsNum + stringOff] : null;
	}

	/**
	 * Look up an attribute's type by index.
	 *
	 * <p>The attribute type is one of the strings "CDATA", "ID",
	 * "IDREF", "IDREFS", "NMTOKEN", "NMTOKENS", "ENTITY", "ENTITIES",
	 * or "NOTATION" (always in upper case).</p>
	 *
	 * <p>If the parser has not read a declaration for the attribute,
	 * or if the parser does not report attribute types, then it must
	 * return the value "CDATA" as stated in the XML 1.0 Recommendation
	 * (clause 3.3.3, "Attribute-Value Normalization").</p>
	 *
	 * <p>For an enumerated attribute that is not a notation, the
	 * parser will report the type as "NMTOKEN".</p>
	 *
	 * @param index The attribute index (zero-based).
	 * @return The attribute's type as a string, or null if the
	 *         index is out of range.
	 * @see #getLength
	 */
	public String getType(int index)
	{
		return get(index, typeOff);
	}

	/**
	 * Look up an attribute's value by index.
	 *
	 * <p>If the attribute value is a list of tokens (IDREFS,
	 * ENTITIES, or NMTOKENS), the tokens will be concatenated
	 * into a single string with each token separated by a
	 * single space.</p>
	 *
	 * @param index The attribute index (zero-based).
	 * @return The attribute's value as a string, or null if the
	 *         index is out of range.
	 * @see #getLength
	 */
	public String getValue(int index)
	{
		return get(index, valueOff);
	}

	/**
	 * Look up the index of an attribute by XML 1.0 qualified name.
	 *
	 * @param qName The qualified (prefixed) name.
	 * @return The index of the attribute, or -1 if it does not
	 *	appear in the list.
	 */
	public int getIndex(String qName)
	{
		char len = mCount;
		for( char idx = 0; idx < len; idx++) {
			if (getQName(idx).equals(qName))
				return idx;
		}
		return -1;
	}

	public String getQName(int index) {
		return get(index, qnameOff);
	}
	
	/**
	 * Look up an attribute's type by Namespace name.
	 *
	 * <p>See {@link #getType(int) getType(int)} for a description
	 * of the possible types.</p>
	 *
	 * @param uri The Namespace URI, or the empty String if the
	 *	name has no Namespace URI.
	 * @param localName The local name of the attribute.
	 * @return The attribute type as a string, or null if the
	 *	attribute is not in the list or if Namespace
	 *	processing is not being performed.
	 */
	public String getType(String uri, String localName)
	{
		return get( getIndex(uri, localName), typeOff );
	}

	/**
	 * Look up an attribute's type by XML 1.0 qualified name.
	 *
	 * <p>See {@link #getType(int) getType(int)} for a description
	 * of the possible types.</p>
	 *
	 * @param qName The XML 1.0 qualified name.
	 * @return The attribute type as a string, or null if the
	 *	attribute is not in the list or if qualified names
	 *	are not available.
	 */
	public String getType(String qName)
	{
		return get( getIndex(qName), typeOff );
	}

	/**
	 * Look up an attribute's value by Namespace name.
	 *
	 * <p>See {@link #getValue(int) getValue(int)} for a description
	 * of the possible values.</p>
	 *
	 * @param uri The Namespace URI, or the empty String if the
	 *	name has no Namespace URI.
	 * @param localName The local name of the attribute.
	 * @return The attribute value as a string, or null if the
	 *	attribute is not in the list.
	 */
	public String getValue(String uri, String localName)
	{
		return get( getIndex(uri, localName), valueOff);
	}

	/**
	 * Look up an attribute's value by XML 1.0 qualified name.
	 *
	 * <p>See {@link #getValue(int) getValue(int)} for a description
	 * of the possible values.</p>
	 *
	 * @param qName The XML 1.0 qualified name.
	 * @return The attribute value as a string, or null if the
	 *	attribute is not in the list or if qualified names
	 *	are not available.
	 */
	public String getValue(String qName)
	{
		return get( getIndex(qName), valueOff);
	}
	
	protected int compare( int j, int k ) {
		return mItems[ j * attrStringsNum + qnameOff ].
					compareTo(mItems[ k * attrStringsNum + qnameOff ]);
	}

	/**
	 * Sorts attributes using URL and local name as a sort key 
	 */
	protected void sort() {
		String tmp;
		for( int i = 1; i < getLength(); i++){
			int j = i;
			while( j > 0 && compare( j, j - 1 ) < 0 ){
				// change jth and (j-1)th elements
				int idx = (j - 1) * attrStringsNum, count = attrStringsNum;
				for(; count-- > 0; idx++){
					tmp = mItems[ idx ];
					mItems[ idx ] = mItems[ idx + attrStringsNum ];
					mItems[ idx + attrStringsNum ] = tmp;
				}
			}
		}
	}

	public boolean hasDuplications() {
		sort();
		for( int i = 1; i < getLength(); i++){
			if( compare(i - 1, i) == 0 )
				return true;
		}
		return false;
	}

	public void remove(int idx) {
		if( idx < 0 || idx >= mCount )
			throw new ArrayIndexOutOfBoundsException();
		mCount--;
		for( idx *= attrStringsNum; idx < mCount * attrStringsNum; idx++)
			mItems[ idx ] = mItems[ idx + attrStringsNum ]; 
	}
	
	//-----------------------------------------------------
	
	static class NSAware extends Attrs {
		final static private int namespaceOff = valueOff + 1, nameOff = namespaceOff + 1; 
		
		protected NSAware() {
			super(nameOff + 1);
		}

		public int getIndex(String uri, String localName) {
			char len = mCount;
			for( char idx = 0; idx < len; idx++) {
				if (getURI(idx).equals(uri) && getLocalName(idx).equals(localName)){
					return idx;
				}
			}
			return -1;
		}

		public String getURI(int index) {
			return get(index, namespaceOff);
		}

		public String getLocalName(int index) {
			return get(index, nameOff);
		}

		String /*error message*/ resolveNamespace(int idx, Namespace.Stack nsStack) {
			String qname = mItems[idx * attrStringsNum + qnameOff];
			int cidx = qname.indexOf(':');
			if( cidx != -1 ){
				// attribute name has prefix
				String prefix = qname.substring(0, cidx );
				Namespace ns = nsStack.find(prefix);
				if( ns == null )
					return Parser.FAULT;
				mItems[ idx * attrStringsNum + namespaceOff ] = ns.URI;
				mItems[idx * attrStringsNum + nameOff] = qname.substring(cidx + 1);
			} else {
				// attribute has no namespace specification
				mItems[ idx * attrStringsNum + namespaceOff ] = "";
				mItems[idx * attrStringsNum + nameOff] = qname;
			}
			return null;
		}
		
		protected int compare( int j, int k ) {
			// local names can't be null
			int rc = mItems[ j * attrStringsNum + nameOff ].
						compareTo(mItems[ k * attrStringsNum + nameOff ]);
			if( rc != 0 )
				return rc;
			String nsj = mItems[ j * attrStringsNum + namespaceOff ];
			if( nsj == null ){
				if( mItems[ k * attrStringsNum + namespaceOff ] == null )
					return 0; // equals
				return -1; 
			}
			String nsk = mItems[ k * attrStringsNum + namespaceOff ];
			if( nsk == null )
				return +1;
			return nsj.compareTo( nsk );
		}

	}
	
	//-----------------------------------------------------
	
	static class NotNSAware extends Attrs {
		protected NotNSAware() {
			super(valueOff + 1);
		}

		public int getIndex(String uri, String localName) {
			return -1;
		}
		
		public String getURI(int index) {
			return ( index < 0 || index >= mCount )? null : "";
		}

		public String getLocalName(int index) {
			return ( index < 0 || index >= mCount )? null : "";
		}
	}
}
