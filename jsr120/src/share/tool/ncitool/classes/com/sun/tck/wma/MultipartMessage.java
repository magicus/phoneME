/*
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

package com.sun.tck.wma;


/* Referenced classes of package com.sun.tck.wma: Message, SizeExceededException, MessagePart*/

public interface MultipartMessage
    extends Message
{

    public abstract boolean addAddress(String s, String s1);

    public abstract void addMessagePart(MessagePart messagepart)
        throws SizeExceededException;

    public abstract String getAddress();

    public abstract String[] getAddresses(String s);

    public abstract String getHeader(String s);

    public abstract MessagePart getMessagePart(String s);

    public abstract MessagePart[] getMessageParts();

    public abstract String getStartContentId();

    public abstract String getSubject();

    public abstract boolean removeAddress(String s, String s1);

    public abstract void removeAddresses();

    public abstract void removeAddresses(String s);

    public abstract boolean removeMessagePart(MessagePart messagepart);

    public abstract boolean removeMessagePartId(String s);

    public abstract boolean removeMessagePartLocation(String s);

    public abstract void setAddress(String s);

    public abstract void setHeader(String s, String s1);

    public abstract void setStartContentId(String s);

    public abstract void setSubject(String s);
}
