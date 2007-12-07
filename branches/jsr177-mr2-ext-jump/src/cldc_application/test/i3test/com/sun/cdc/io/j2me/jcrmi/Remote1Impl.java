/*
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

package com.sun.cdc.io.j2me.jcrmi;

import javacard.framework.service.CardRemoteObject;
import javacard.framework.APDU;
import java.rmi.RemoteException;
import java.rmi.Remote;

public class Remote1Impl extends CardRemoteObject implements Remote1 {

    private short[] ids = new short[4];

    public Remote1Impl() {
        super();
    }

    public void setChannelId(short id) throws RemoteException {
        ids[DataTestApplet.channel] = id;
    }

    public short getChannelId() throws RemoteException {
        return ids[DataTestApplet.channel];
    }

    public short getChannel() throws RemoteException {
        return DataTestApplet.channel;
    }

    public boolean getBoolean(boolean v) {
        return v;
    }

    public byte getByte(byte v) {
        return v;
    }

    public short getShort(short v) {
        return v;
    }

//    public int getInt(int v) {
//        return v;
//    }

    public boolean[] getBooleanArray(boolean[] v) {
        return v;
    }

    public byte[] getByteArray(byte[] v) {
        return v;
    }

    public short[] getShortArray(short[] v) {
        return v;
    }

//    public int[] getIntArray(int[] v) {
//        return v;
//    }

    public void voidMethod() {
    }

    static final short reason = 0x1234;

    public void throwException(short code) throws Throwable {
        switch (code) {
            case 0: throw new java.lang.Throwable();
            case 1: throw new java.lang.ArithmeticException();
            case 2: throw new java.lang.ArrayIndexOutOfBoundsException();
            case 3: throw new java.lang.ArrayStoreException();
            case 4: throw new java.lang.ClassCastException();
            case 5: throw new java.lang.Exception();
            case 6: throw new java.lang.IndexOutOfBoundsException();
            case 7: throw new java.lang.NegativeArraySizeException();
            case 8: throw new java.lang.NullPointerException();
            case 9: throw new java.lang.RuntimeException();
            case 10: throw new java.lang.SecurityException();
            case 11: throw new java.io.IOException();
            case 12: throw new java.rmi.RemoteException();
            case 13: throw new javacard.framework.APDUException(reason);
            case 14: throw new javacard.framework.CardException(reason);
            case 15: throw new javacard.framework.CardRuntimeException(reason);
            case 16: throw new javacard.framework.ISOException(reason);
            case 17: throw new javacard.framework.PINException(reason);
            case 18: throw new javacard.framework.SystemException(reason);
            case 19: throw new javacard.framework.TransactionException(reason);
            case 20: throw new javacard.framework.UserException(reason);
            case 21: throw new javacard.security.CryptoException(reason);
            case 22: throw new javacard.framework.service.ServiceException(reason);
        }
    }
 

    public void throwSubclass(short code) throws Throwable {
        switch (code) {
            case 0: throw new Throwable_();
            case 1: throw new ArithmeticException_();
            case 2: throw new ArrayIndexOutOfBoundsException_();
            case 3: throw new ArrayStoreException_();
            case 4: throw new ClassCastException_();
            case 5: throw new Exception_();
            case 6: throw new IndexOutOfBoundsException_();
            case 7: throw new NegativeArraySizeException_();
            case 8: throw new NullPointerException_();
            case 9: throw new RuntimeException_();
            case 10: throw new SecurityException_();
            case 11: throw new IOException_();
            case 12: throw new RemoteException_();
            case 13: throw new APDUException_(reason);
            case 14: throw new CardException_(reason);
            case 15: throw new CardRuntimeException_(reason);
            case 16: throw new ISOException_(reason);
            case 17: throw new PINException_(reason);
            case 18: throw new SystemException_(reason);
            case 19: throw new TransactionException_(reason);
            case 20: throw new UserException_(reason);
            case 21: throw new CryptoException_(reason);
            case 22: throw new ServiceException_(reason);
        }
    }

    public byte[] getNewArray(short length) throws RemoteException {
        return new byte[(short) length];
    }

    public Remote1 getReference1() throws RemoteException {
        return null;
    }

    public Remote getReference2() throws RemoteException {
        return null;
    }
}


class Throwable_ extends java.lang.Throwable {}
class ArithmeticException_ extends java.lang.ArithmeticException {}
class ArrayIndexOutOfBoundsException_ extends java.lang.ArrayIndexOutOfBoundsException {}
class ArrayStoreException_ extends java.lang.ArrayStoreException {}
class ClassCastException_ extends java.lang.ClassCastException {}
class Exception_ extends java.lang.Exception {}
class IndexOutOfBoundsException_ extends java.lang.IndexOutOfBoundsException {}
class NegativeArraySizeException_ extends java.lang.NegativeArraySizeException {}
class NullPointerException_ extends java.lang.NullPointerException {}
class RuntimeException_ extends java.lang.RuntimeException {}
class SecurityException_ extends java.lang.SecurityException {}
class IOException_ extends java.io.IOException {}
class RemoteException_ extends java.rmi.RemoteException {}
class APDUException_ extends javacard.framework.APDUException {
    APDUException_(short reason) {
        super(reason);
    }
}
class CardException_ extends javacard.framework.CardException {
    CardException_(short reason) {
        super(reason);
    }
}
class CardRuntimeException_ extends javacard.framework.CardRuntimeException {
    CardRuntimeException_(short reason) {
        super(reason);
    }
}
class ISOException_ extends javacard.framework.ISOException {
    ISOException_(short reason) {
        super(reason);
    }
}
class PINException_ extends javacard.framework.PINException {
    PINException_(short reason) {
        super(reason);
    }
}
class SystemException_ extends javacard.framework.SystemException {
    SystemException_(short reason) {
        super(reason);
    }
}
class TransactionException_ extends javacard.framework.TransactionException {
    TransactionException_(short reason) {
        super(reason);
    }
}
class UserException_ extends javacard.framework.UserException {
    UserException_(short reason) {
        super(reason);
    }
}
class CryptoException_ extends javacard.security.CryptoException {
    CryptoException_(short reason) {
        super(reason);
    }
}
class ServiceException_ extends javacard.framework.service.ServiceException {
    ServiceException_(short reason) {
        super(reason);
    } 
}
