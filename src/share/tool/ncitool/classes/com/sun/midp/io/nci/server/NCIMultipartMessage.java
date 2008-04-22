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

package com.sun.midp.io.nci.server;

import com.sun.tck.wma.*;
import java.io.*;
import java.util.Date;
import java.util.Vector;

/* Referenced classes of package com.sun.midp.io.nci.server: NCIMessage */

public class NCIMultipartMessage extends NCIMessage
    implements MultipartMessage
{

    public NCIMultipartMessage()
    {
        to = new Vector();
        cc = new Vector();
        bcc = new Vector();
        parts = new Vector();
        startContentID = null;
        subject = null;
        applicationID = null;
        replyToApplicationID = null;
        setupHeaderFields();
    }

    public void setTimeStamp(long l)
    {
        sentAt = l;
    }

    public Date getTimestamp()
    {
        if(sentAt == 0L)
            return null;
        else
            return new Date(sentAt);
    }

    String getReplyToApplicationID()
    {
        return replyToApplicationID;
    }

    public void setReplyToApplicationID(String s)
    {
        replyToApplicationID = s;
    }

    public String getApplicationID()
    {
        return applicationID;
    }

    public void setFromAddress(String s)
    {
        super.setAddress(s);
    }

    public void fixupReceivedMessageAddresses(String s, String s1)
    {
        String s2 = s1;
        String s3 = s1;
        if(s2.charAt(0) == '+')
            s2 = s2.substring(1);
        else
        if(s3.charAt(0) != '+')
            s3 = "+" + s3;
        Vector vector = to;
        for(int i = 0; i < 2; i++)
        {
            int j = vector.size();
            for(int k = 0; k < j; k++)
            {
                String s5 = (String)vector.elementAt(k);
                MMSAddress mmsaddress1 = MMSAddress.getParsedMMSAddress(s5);
                if(mmsaddress1 != null && (s2.equals(mmsaddress1.address) || s3.equals(mmsaddress1.address)))
                {
                    j--;
                    vector.removeElementAt(k);
                    k--;
                }
            }

            vector = cc;
        }

        if(s != null)
        {
            String s4 = s;
            to.insertElementAt(s4, 0);
            MMSAddress mmsaddress = MMSAddress.getParsedMMSAddress(s4);
            applicationID = mmsaddress.appId;
        } else
        {
            applicationID = null;
        }
    }

    Vector getAddressList(String s)
    {
        String s1 = s.toLowerCase();
        if(s1.equals("to"))
            return to;
        if(s1.equals("cc"))
            return cc;
        if(s1.equals("bcc"))
            return bcc;
        else
            throw new IllegalArgumentException("Address type is not 'to', 'cc', or 'bcc'");
    }

    MMSAddress checkValidAddress(String s)
        throws IllegalArgumentException
    {
        MMSAddress mmsaddress = MMSAddress.getParsedMMSAddress(s);
        if(mmsaddress == null || mmsaddress.type == -1 || mmsaddress.type == 5)
            throw new IllegalArgumentException("Invalid destination address: " + s);
        else
            return mmsaddress;
    }

    void checkApplicationID(String s)
        throws IllegalArgumentException
    {
        if(applicationID != null)
        {
            if(!applicationID.equals(s))
                throw new IllegalArgumentException("Only one Application-ID can be specified per message");
        } else
        {
            applicationID = s;
        }
    }

    static boolean isKnownHeaderField(String s)
    {
        String s1 = s.toLowerCase();
        for(int i = 0; i < KNOWN_HEADER_FIELDS.length; i++)
            if(s1.equals(KNOWN_HEADER_FIELDS[i].toLowerCase()))
                return true;

        return false;
    }

    void setupHeaderFields()
    {
        headerValues = new String[ALLOWED_HEADER_FIELDS.length];
        for(int i = 0; i < DEFAULT_HEADER_VALUES.length; i++)
            headerValues[i] = DEFAULT_HEADER_VALUES[i];

    }

    static int getHeaderFieldIndex(String s)
    {
        String s1 = s.toLowerCase();
        for(int i = 0; i < ALLOWED_HEADER_FIELDS.length; i++)
            if(s1.equals(ALLOWED_HEADER_FIELDS[i].toLowerCase()))
                return i;

        return -1;
    }

    boolean isAllowedToAccessHeaderField(String s)
    {
        return getHeaderFieldIndex(s) != -1;
    }

    static void checkHeaderValue(int i, String s)
    {
        switch(i)
        {
        case 0: // '\0'
            try
            {
                Long.parseLong(s);
                return;
            }
            catch(NumberFormatException numberformatexception) { }
            break;

        case 1: // '\001'
            String s1 = s.toLowerCase();
            if(s1.equals("normal") || s1.equals("high") || s1.equals("low"))
                return;
            break;

        default:
            throw new Error("Unknown headerIndex: " + i);
        }
        throw new IllegalArgumentException("Illegal value for header " + ALLOWED_HEADER_FIELDS[i] + ": " + s);
    }

    public boolean addAddress(String s, String s1)
        throws IllegalArgumentException
    {
        MMSAddress mmsaddress = checkValidAddress(s1);
        String s2 = mmsaddress.appId;
        if(s2 != null)
            checkApplicationID(s2);
        Vector vector = getAddressList(s);
        if(!vector.contains(s1))
        {
            vector.addElement(s1);
            return true;
        } else
        {
            return false;
        }
    }

    public void addMessagePart(MessagePart messagepart)
        throws SizeExceededException
    {
        String s = messagepart.getContentID();
        boolean flag = false;
        int i = 0;
        int j = parts.size();
        for(int k = 0; k < j; k++)
        {
            MessagePart messagepart1 = (MessagePart)parts.elementAt(k);
            if(s.equals(messagepart1.getContentID()))
                throw new IllegalArgumentException("Cannot add duplicate content-id: " + s);
            i += messagepart1.getLength();
        }

        if(i + messagepart.getLength() > 30730)
        {
            throw new SizeExceededException("Adding this MessagePart would exceed max size of 30730 bytes");
        } else
        {
            parts.addElement(messagepart);
            return;
        }
    }

    public String getAddress()
    {
        String s = null;
        Date date = getTimestamp();
        if(date == null || date.getTime() == 0L)
        {
            if(to.size() > 0)
                s = (String)to.elementAt(0);
        } else
        {
            s = super.getAddress();
        }
        return s;
    }

    public String[] getAddresses(String s)
    {
        if(s.toLowerCase().equals("from"))
        {
            String s1 = super.getAddress();
            if(s1 == null)
                return null;
            else
                return (new String[] {
                    s1
                });
        }
        Vector vector = getAddressList(s);
        int i = vector.size();
        if(i == 0)
        {
            return null;
        } else
        {
            String as[] = new String[i];
            vector.copyInto(as);
            return as;
        }
    }

    public String getHeader(String s)
    {
        if(s == null)
            throw new IllegalArgumentException("headerField must not be null");
        if(isAllowedToAccessHeaderField(s))
        {
            int i = getHeaderFieldIndex(s);
            if(i != -1)
                return headerValues[i];
            else
                throw new Error("Allowed to access field but it has no index");
        }
        if(isKnownHeaderField(s))
            throw new SecurityException("Cannot access restricted header field: " + s);
        else
            throw new IllegalArgumentException("Unknown header field: " + s);
    }

    public MessagePart getMessagePart(String s)
    {
        if(s == null)
            throw new NullPointerException("contentID must not be null");
        int i = parts.size();
        for(int j = 0; j < i; j++)
        {
            MessagePart messagepart = (MessagePart)parts.elementAt(j);
            if(s.equals(messagepart.getContentID()))
                return messagepart;
        }

        return null;
    }

    public MessagePart[] getMessageParts()
    {
        int i = parts.size();
        if(i == 0)
        {
            return null;
        } else
        {
            MessagePart amessagepart[] = new MessagePart[i];
            parts.copyInto(amessagepart);
            return amessagepart;
        }
    }

    public String getStartContentId()
    {
        return startContentID;
    }

    public String getSubject()
    {
        return subject;
    }

    private void cleanupAppID()
        throws IllegalStateException
    {
        Vector vector = to;
        boolean flag = false;
        boolean flag1 = false;
        int i = 0;
        boolean flag2 = false;
        do
            if(i >= vector.size())
            {
                if(!flag)
                {
                    flag = true;
                    vector = cc;
                    i = 0;
                    continue;
                }
                if(flag1)
                    break;
                flag1 = true;
                vector = bcc;
                i = 0;
            } else
            {
                String s = (String)vector.elementAt(i++);
                MMSAddress mmsaddress = MMSAddress.getParsedMMSAddress(s);
                if(mmsaddress == null || mmsaddress.type == -1 || mmsaddress.type == 5)
                    throw new IllegalStateException("Invalid MMS address: " + s);
                String s1 = mmsaddress.appId;
                if(s1 != null && s1.equals(applicationID))
                    flag2 = true;
            }
        while(true);
        if(!flag2)
            applicationID = null;
    }

    public boolean removeAddress(String s, String s1)
    {
        Vector vector = getAddressList(s);
        boolean flag = vector.removeElement(s1);
        cleanupAppID();
        return flag;
    }

    public void removeAddresses()
    {
        to.removeAllElements();
        cc.removeAllElements();
        bcc.removeAllElements();
        applicationID = null;
    }

    public void removeAddresses(String s)
    {
        Vector vector = getAddressList(s);
        vector.removeAllElements();
        cleanupAppID();
    }

    public boolean removeMessagePart(MessagePart messagepart)
    {
        if(messagepart == null)
            throw new NullPointerException("part must not be null");
        if(messagepart.getContentID().equals(startContentID))
            startContentID = null;
        return parts.removeElement(messagepart);
    }

    public boolean removeMessagePartId(String s)
    {
        if(s == null)
            throw new NullPointerException("contentID must not be null");
        int i = parts.size();
        for(int j = 0; j < i; j++)
        {
            MessagePart messagepart = (MessagePart)parts.elementAt(j);
            if(s.equals(messagepart.getContentID()))
            {
                if(s.equals(startContentID))
                    startContentID = null;
                parts.removeElementAt(j);
                return true;
            }
        }

        return false;
    }

    public boolean removeMessagePartLocation(String s)
    {
        if(s == null)
            throw new NullPointerException("contentLocation must not be null");
        int i = parts.size();
        boolean flag = false;
        for(int j = 0; j < i; j++)
        {
            MessagePart messagepart = (MessagePart)parts.elementAt(j);
            if(!s.equals(messagepart.getContentLocation()))
                continue;
            if(messagepart.getContentID().equals(startContentID))
                startContentID = null;
            parts.removeElementAt(j);
            i--;
            j--;
            flag = true;
        }

        return flag;
    }

    public void setAddress(String s)
    {
        if(s != null)
            addAddress("to", s);
    }

    public void setHeader(String s, String s1)
    {
        if(isAllowedToAccessHeaderField(s))
        {
            int i = getHeaderFieldIndex(s);
            if(i != -1)
            {
                if(s1 != null)
                    checkHeaderValue(i, s1);
                headerValues[i] = s1;
                return;
            } else
            {
                throw new Error("Allowed to access field but it has no index");
            }
        }
        if(isKnownHeaderField(s))
            throw new SecurityException("Cannot access restricted header field: " + s);
        else
            throw new IllegalArgumentException("Unknown header field: " + s);
    }

    public void setStartContentId(String s)
    {
        if(s != null && getMessagePart(s) == null)
        {
            throw new IllegalArgumentException("Unknown contentId: " + s);
        } else
        {
            startContentID = s;
            return;
        }
    }

    public void setSubject(String s)
    {
        if(s != null && s.length() > 40)
        {
            throw new IllegalArgumentException("Subject exceeds 40 chars");
        } else
        {
            subject = s;
            return;
        }
    }

    static String getDevicePortionOfAddress(String s)
        throws IllegalArgumentException
    {
        MMSAddress mmsaddress = MMSAddress.getParsedMMSAddress(s);
        if(mmsaddress == null || mmsaddress.address == null)
            throw new IllegalArgumentException("MMS Address has no device portion");
        else
            return mmsaddress.address;
    }

    static void writeVector(DataOutputStream dataoutputstream, Vector vector, boolean flag)
        throws IOException
    {
        StringBuffer stringbuffer = new StringBuffer();
        int i = vector.size();
        Object obj = null;
        if(i > 0)
        {
            String s = (String)vector.elementAt(0);
            if(flag)
                s = getDevicePortionOfAddress(s);
            stringbuffer.append(s);
        }
        for(int j = 1; j < i; j++)
        {
            stringbuffer.append("; ");
            String s1 = (String)vector.elementAt(j);
            if(flag)
                s1 = getDevicePortionOfAddress(s1);
            stringbuffer.append(s1);
        }

        dataoutputstream.writeUTF(stringbuffer.toString());
    }

    static void readVector(DataInputStream datainputstream, Vector vector, boolean flag)
        throws IOException
    {
        String s = datainputstream.readUTF();
        int i = -2;
        String s1 = "";
        if(flag)
            s1 = "mms://";
        int j;
        for(; i != -1; i = j)
        {
            j = s.indexOf("; ", i + 2);
            String s2 = null;
            if(j == -1)
                s2 = s1 + s.substring(i + 2);
            else
                s2 = s1 + s.substring(i + 2, j);
            vector.addElement(s2);
        }

    }

    static void writeMessagePart(DataOutputStream dataoutputstream, MessagePart messagepart)
        throws IOException
    {
        dataoutputstream.writeUTF("Content-Type");
        StringBuffer stringbuffer = new StringBuffer(messagepart.getMIMEType());
        String s = messagepart.getContentLocation();
        if(s != null)
        {
            stringbuffer.append("; name=\"");
            stringbuffer.append(s);
            stringbuffer.append("\"");
        }
        dataoutputstream.writeUTF(stringbuffer.toString());
        String s1 = messagepart.getContentID();
        if(s1 != null)
        {
            dataoutputstream.writeUTF("Content-ID");
            dataoutputstream.writeUTF(s1);
        }
        String s2 = messagepart.getEncoding();
        if(s2 != null)
        {
            dataoutputstream.writeUTF("Encoding");
            dataoutputstream.writeUTF(s2);
        }
        dataoutputstream.writeUTF("Content-Length");
        dataoutputstream.writeInt(messagepart.getLength());
        dataoutputstream.writeUTF("Content");
        dataoutputstream.write(messagepart.getContent());
    }

    static MessagePart createMessagePart(DataInputStream datainputstream)
        throws IOException
    {
        String s = datainputstream.readUTF();
        String s1 = datainputstream.readUTF();
        s = datainputstream.readUTF();
        String s2 = null;
        if(s.equals("Content-ID"))
        {
            s2 = datainputstream.readUTF();
            s = datainputstream.readUTF();
        }
        String s3 = null;
        if(s.equals("Encoding"))
        {
            s3 = datainputstream.readUTF();
            s = datainputstream.readUTF();
        }
        int i = datainputstream.readInt();
        byte abyte0[] = new byte[i];
        s = datainputstream.readUTF();
        datainputstream.readFully(abyte0);
        String s4 = s1;
        String s5 = null;
        int j = s1.indexOf(';');
        if(j != -1 && s1.substring(j).startsWith("; name=\""))
        {
            s5 = s1.substring(j + 8, s1.length() - 1);
            s4 = s1.substring(0, j);
        }
        return new MessagePart(abyte0, s4, s2, s5, s3);
    }

    public byte[] getAsByteArray()
        throws IOException
    {
        ByteArrayOutputStream bytearrayoutputstream = new ByteArrayOutputStream();
        DataOutputStream dataoutputstream = new DataOutputStream(bytearrayoutputstream);
        dataoutputstream.writeUTF("application/vnd.wap.mms-message");
        dataoutputstream.writeUTF("X-Mms-Message-Type");
        dataoutputstream.writeUTF("m-send-req");
        dataoutputstream.writeUTF("X-Mms-Transaction-ID");
        dataoutputstream.writeUTF(String.valueOf(System.currentTimeMillis()));
        dataoutputstream.writeUTF("X-Mms-Version");
        dataoutputstream.writeUTF("1.0");
        for(int i = 0; i < ALLOWED_HEADER_FIELDS.length; i++)
        {
            String s1 = headerValues[i];
            if(s1 != null)
            {
                dataoutputstream.writeUTF(ALLOWED_HEADER_FIELDS[i]);
                dataoutputstream.writeUTF(s1);
            }
        }

        String s = super.getAddress();
        if(s != null)
        {
            dataoutputstream.writeUTF("From");
            dataoutputstream.writeUTF(getDevicePortionOfAddress(s));
        }
        if(to.size() != 0)
        {
            dataoutputstream.writeUTF("To");
            writeVector(dataoutputstream, to, true);
        }
        if(cc.size() != 0)
        {
            dataoutputstream.writeUTF("Cc");
            writeVector(dataoutputstream, cc, true);
        }
        if(bcc.size() != 0)
        {
            dataoutputstream.writeUTF("Bcc");
            writeVector(dataoutputstream, bcc, true);
        }
        long l = 0L;
        Date date = getTimestamp();
        if(date != null && (l = date.getTime()) != 0L)
        {
            dataoutputstream.writeUTF("Date");
            dataoutputstream.writeUTF(String.valueOf(l));
        }
        if(subject != null)
        {
            dataoutputstream.writeUTF("Subject");
            dataoutputstream.writeUTF(subject);
        }
        dataoutputstream.writeUTF("Content-Type");
        Vector vector = new Vector();
        if(startContentID != null)
            vector.addElement("application/vnd.wap.multipart.related");
        else
            vector.addElement("application/vnd.wap.multipart.mixed");
        if(startContentID != null)
        {
            vector.addElement("start = <" + startContentID + ">");
            vector.addElement("type = " + getMessagePart(startContentID).getMIMEType());
        }
        if(applicationID != null)
            vector.addElement("Application-ID = " + applicationID);
        if(replyToApplicationID != null)
            vector.addElement("Reply-To-Application-ID = " + replyToApplicationID);
        writeVector(dataoutputstream, vector, false);
        dataoutputstream.writeUTF("nEntries");
        int j = parts.size();
        dataoutputstream.writeUTF(String.valueOf(j));
        for(int k = 0; k < j; k++)
        {
            MessagePart messagepart = (MessagePart)parts.elementAt(k);
            writeMessagePart(dataoutputstream, messagepart);
        }

        dataoutputstream.close();
        byte abyte0[] = bytearrayoutputstream.toByteArray();
        bytearrayoutputstream.close();
        return abyte0;
    }

    public static NCIMultipartMessage createFromByteArray(byte abyte0[])
        throws IOException
    {
        ByteArrayInputStream bytearrayinputstream = new ByteArrayInputStream(abyte0);
        DataInputStream datainputstream = new DataInputStream(bytearrayinputstream);
        String s = datainputstream.readUTF();
        if(!s.equals("application/vnd.wap.mms-message"))
            throw new IOException("invalid data format");
        for(int i = 0; i < 6; i++)
            datainputstream.readUTF();

        String as[] = new String[ALLOWED_HEADER_FIELDS.length];
        String s1;
        int j;
        for(s1 = datainputstream.readUTF(); (j = getHeaderFieldIndex(s1)) != -1; s1 = datainputstream.readUTF())
            as[j] = datainputstream.readUTF();

        String s2 = null;
        if(s1.equals("From"))
        {
            s2 = "mms://" + datainputstream.readUTF();
            s1 = datainputstream.readUTF();
        }
        Vector vector = new Vector();
        if(s1.equals("To"))
        {
            readVector(datainputstream, vector, true);
            s1 = datainputstream.readUTF();
        }
        Vector vector1 = new Vector();
        if(s1.equals("Cc"))
        {
            readVector(datainputstream, vector1, true);
            s1 = datainputstream.readUTF();
        }
        Vector vector2 = new Vector();
        if(s1.equals("Bcc"))
        {
            readVector(datainputstream, vector2, true);
            s1 = datainputstream.readUTF();
        }
        long l = 0L;
        if(s1.equals("Date"))
        {
            String s3 = datainputstream.readUTF();
            try
            {
                l = Long.parseLong(s3);
            }
            catch(NumberFormatException numberformatexception)
            {
                l = 0L;
            }
            s1 = datainputstream.readUTF();
        }
        String s4 = null;
        if(s1.equals("Subject"))
        {
            s4 = datainputstream.readUTF();
            s1 = datainputstream.readUTF();
        }
        String s5 = null;
        String s6 = null;
        String s7 = null;
        Vector vector3 = new Vector();
        readVector(datainputstream, vector3, false);
        int k = vector3.size();
        for(int i1 = 0; i1 < k; i1++)
        {
            String s8 = (String)vector3.elementAt(i1);
            if(s8.startsWith("start = <"))
            {
                s5 = s8.substring(9);
                s5 = s5.substring(0, s5.length() - 1);
                continue;
            }
            if(s8.startsWith("Application-ID = "))
            {
                s6 = s8.substring(17);
                continue;
            }
            if(s8.startsWith("Reply-To-Application-ID = "))
                s7 = s8.substring(26);
        }

        s1 = datainputstream.readUTF();
        System.out.println(s1);
        int j1 = 0;
        String s9 = datainputstream.readUTF();
        try
        {
            j1 = Integer.parseInt(s9);
        }
        catch(NumberFormatException numberformatexception1)
        {
            j1 = 0;
        }
        Vector vector4 = new Vector();
        for(int k1 = 0; k1 < j1; k1++)
            vector4.addElement(createMessagePart(datainputstream));

        datainputstream.close();
        bytearrayinputstream.close();
        NCIMultipartMessage ncimultipartmessage = new NCIMultipartMessage();
        ncimultipartmessage.setFromAddress(s2);
        ncimultipartmessage.setTimeStamp(l);
        ncimultipartmessage.headerValues = as;
        ncimultipartmessage.subject = s4;
        ncimultipartmessage.startContentID = s5;
        ncimultipartmessage.to = vector;
        ncimultipartmessage.cc = vector1;
        ncimultipartmessage.parts = vector4;
        ncimultipartmessage.applicationID = s6;
        ncimultipartmessage.replyToApplicationID = s7;
        return ncimultipartmessage;
    }

    public byte[] getHeaderAsByteArray()
        throws IOException
    {
        ByteArrayOutputStream bytearrayoutputstream = new ByteArrayOutputStream();
        DataOutputStream dataoutputstream = new DataOutputStream(bytearrayoutputstream);
        dataoutputstream.writeUTF("application/vnd.wap.mms-message");
        dataoutputstream.writeUTF("X-Mms-Message-Type");
        dataoutputstream.writeUTF("m-send-req");
        dataoutputstream.writeUTF("X-Mms-Transaction-ID");
        dataoutputstream.writeUTF(String.valueOf(System.currentTimeMillis()));
        dataoutputstream.writeUTF("X-Mms-Version");
        dataoutputstream.writeUTF("1.0");
        for(int i = 0; i < ALLOWED_HEADER_FIELDS.length; i++)
        {
            String s1 = headerValues[i];
            if(s1 != null)
            {
                dataoutputstream.writeUTF(ALLOWED_HEADER_FIELDS[i]);
                dataoutputstream.writeUTF(s1);
            }
        }

        String s = super.getAddress();
        if(s != null)
        {
            dataoutputstream.writeUTF("From");
            dataoutputstream.writeUTF(getDevicePortionOfAddress(s));
        }
        if(to.size() != 0)
        {
            dataoutputstream.writeUTF("To");
            writeVector(dataoutputstream, to, true);
        }
        if(cc.size() != 0)
        {
            dataoutputstream.writeUTF("Cc");
            writeVector(dataoutputstream, cc, true);
        }
        if(bcc.size() != 0)
        {
            dataoutputstream.writeUTF("Bcc");
            writeVector(dataoutputstream, bcc, true);
        }
        long l = 0L;
        Date date = getTimestamp();
        if(date != null && (l = date.getTime()) != 0L)
        {
            dataoutputstream.writeUTF("Date");
            dataoutputstream.writeUTF(String.valueOf(l));
        }
        if(subject != null)
        {
            dataoutputstream.writeUTF("Subject");
            dataoutputstream.writeUTF(subject);
        }
        dataoutputstream.writeUTF("Content-Type");
        Vector vector = new Vector();
        if(startContentID != null)
            vector.addElement("application/vnd.wap.multipart.related");
        else
            vector.addElement("application/vnd.wap.multipart.mixed");
        if(startContentID != null)
        {
            vector.addElement("start = <" + startContentID + ">");
            vector.addElement("type = " + getMessagePart(startContentID).getMIMEType());
        }
        if(applicationID != null)
            vector.addElement("Application-ID = " + applicationID);
        if(replyToApplicationID != null)
            vector.addElement("Reply-To-Application-ID = " + replyToApplicationID);
        writeVector(dataoutputstream, vector, false);
        dataoutputstream.close();
        byte abyte0[] = bytearrayoutputstream.toByteArray();
        bytearrayoutputstream.close();
        return abyte0;
    }

    public byte[] getBodyAsByteArray()
        throws IOException
    {
        ByteArrayOutputStream bytearrayoutputstream = new ByteArrayOutputStream();
        DataOutputStream dataoutputstream = new DataOutputStream(bytearrayoutputstream);
        dataoutputstream.writeUTF("nEntries");
        int i = parts.size();
        dataoutputstream.writeUTF(String.valueOf(i));
        for(int j = 0; j < i; j++)
        {
            MessagePart messagepart = (MessagePart)parts.elementAt(j);
            writeMessagePart(dataoutputstream, messagepart);
        }

        dataoutputstream.close();
        byte abyte0[] = bytearrayoutputstream.toByteArray();
        bytearrayoutputstream.close();
        return abyte0;
    }

    public String toString()
    {
        String s = new String();
        try
        {
            s = s + "From:";
            String as[] = getAddresses("from");
            if(as != null)
            {
                for(int i = 0; i < as.length; i++)
                    s = s + as[i] + ";";

            }
            s = s + "\nTo:";
            as = getAddresses("to");
            if(as != null)
            {
                for(int j = 0; j < as.length; j++)
                    s = s + as[j] + ";";

            }
            s = s + "\nCc:";
            as = getAddresses("cc");
            if(as != null)
            {
                for(int k = 0; k < as.length; k++)
                    s = s + as[k] + ";";

            }
            s = s + "\nBcc:";
            as = getAddresses("bcc");
            if(as != null)
            {
                for(int l = 0; l < as.length; l++)
                    s = s + as[l] + ";";

            }
            MessagePart amessagepart[] = getMessageParts();
            s = s + "\nEntries:" + (amessagepart != null ? amessagepart.length : 0);
        }
        catch(Exception exception)
        {
            exception.printStackTrace();
        }
        return s;
    }

    Vector to;
    Vector cc;
    Vector bcc;
    Vector parts;
    String startContentID;
    String subject;
    String headerValues[];
    String applicationID;
    String replyToApplicationID;
    long sentAt;
    static final String ALLOWED_HEADER_FIELDS[] = {
        "X-Mms-Delivery-Time", "X-Mms-Priority"
    };
    static final String DEFAULT_HEADER_VALUES[] = {
        null, "Normal"
    };
    static final String KNOWN_HEADER_FIELDS[] = {
        "X-Mms-Message-Type", "X-Mms-Transaction-ID", "X-Mms-MMS-Version", "X-Mms-Content-Type", "X-Mms-Subject", "X-Mms-From", "X-Mms-To", "X-Mms-CC", "X-Mms-BCC"
    };
    static final int MAX_TOTAL_SIZE = 30730;
    static final String STREAM_SIGNATURE = "application/vnd.wap.mms-message";

}
