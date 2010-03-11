/*
 * %W% %E%
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
 *
 */
package sun.io;

import java.io.UnsupportedEncodingException;

public class ByteToCharJISAutoDetect extends ByteToCharConverter {
    private String convName = null;
    private ByteToCharConverter detectedConv = null;
    private ByteToCharConverter defaultConv = null;
    public ByteToCharJISAutoDetect() {
        super();
        try {
            defaultConv = ByteToCharConverter.getConverter("8859_1");
        } catch (UnsupportedEncodingException e) {
            defaultConv = new ByteToCharISO8859_1();
        }
        defaultConv.subChars = subChars;
        defaultConv.subMode = subMode;
    }

    public int flush(char[] output, int outStart, int outEnd)
        throws MalformedInputException, ConversionBufferFullException {
        badInputLength = 0;
        if (detectedConv != null)
            return detectedConv.flush(output, outStart, outEnd);
        else
            return defaultConv.flush(output, outStart, outEnd);
    }

    /**
     * Character conversion
     */
    public int convert(byte[] input, int inOff, int inEnd,
        char[] output, int outOff, int outEnd)
        throws UnknownCharacterException, MalformedInputException,
            ConversionBufferFullException {
        int start_pos = inOff;
        int end_pos = inOff + inEnd;
        int num;
        String candidatedConvName = null;
        ByteToCharConverter anticipatedConv = null;
        try {
            if (detectedConv == null) {
                for (int cnt = start_pos; cnt < end_pos; cnt++) {
                    int itmp = input[cnt] & 0xff;
                    if (itmp == 0x1b) {
                        convName = "ISO2022JP";
                        break;
                    } else if ((0x81 <= itmp) && (itmp <= 0x9f)) {
                        convName = "SJIS";
                        break;
                    } else if (((0xa1 <= itmp) && (itmp <= 0xdf)) ||
                        ((0xfd <= itmp) && (itmp <= 0xfe))) {
                        convName = "EUC_JP";
                        break;
                    } else if ((0xe0 <= itmp) && (itmp <= 0xea)) {
                        if (cnt + 1 < end_pos) {
                            cnt++;
                            itmp = input[cnt] & 0xff;
                            if ((0x40 <= itmp) && (itmp <= 0x7e)) {
                                convName = "SJIS";
                                break;
                            } else {
                                if (candidatedConvName == null)
                                    candidatedConvName = "SJIS";
                            }
                        } else {
                            throw new MalformedInputException();
                        }
                        //convName = "SJIS";
                        break;
                    } else if ((0xeb <= itmp) && (itmp <= 0xfc)) {
                        if (cnt + 1 < end_pos) {
                            cnt++;
                            itmp = input[cnt] & 0xff;
                            if ((0x40 <= itmp) && (itmp <= 0x7e)) {
                                convName = "SJIS";
                                break;
                            } else {
                                if (candidatedConvName == null)
                                    candidatedConvName = "EUC_JP";
                            }
                        } else {
                            throw new MalformedInputException();
                        }
                        break;
                    }
                }
                if (convName != null) {
                    try {
                        detectedConv = ByteToCharConverter.getConverter(convName);
                        detectedConv.subChars = subChars;
                        detectedConv.subMode = subMode;
                    } catch (UnsupportedEncodingException e) {
                        detectedConv = null;
                        convName = null;
                    }
                } else if (candidatedConvName != null) {
                    try {
                        anticipatedConv =
                                ByteToCharConverter.getConverter(candidatedConvName);
                        anticipatedConv.subChars = subChars;
                        anticipatedConv.subMode = subMode;
                    } catch (UnsupportedEncodingException e) {
                        anticipatedConv = null;
                        candidatedConvName = null;
                    }
                }
            }
        } catch (Exception e) {
            // If we fail to detect the converter needed for any reason,
            // use the default converter.
            detectedConv = defaultConv;
        }
        if (detectedConv != null) {
            try {
                num = detectedConv.convert(input, inOff, inEnd,
                            output, outOff, outEnd);
                charOff = detectedConv.nextCharIndex();
                byteOff = detectedConv.nextByteIndex();
                badInputLength = detectedConv.badInputLength;
            } catch (UnknownCharacterException e) {
                charOff = detectedConv.nextCharIndex();
                byteOff = detectedConv.nextByteIndex();
                badInputLength = detectedConv.badInputLength;
                throw e;
            } catch (MalformedInputException e) {
                charOff = detectedConv.nextCharIndex();
                byteOff = detectedConv.nextByteIndex();
                badInputLength = detectedConv.badInputLength;
                throw e;
            } catch (ConversionBufferFullException e) {
                charOff = detectedConv.nextCharIndex();
                byteOff = detectedConv.nextByteIndex();
                badInputLength = detectedConv.badInputLength;
                throw e;
            }
        } else if (anticipatedConv != null) {
            try {
                num = anticipatedConv.convert(input, inOff, inEnd,
                            output, outOff, outEnd);
                charOff = anticipatedConv.nextCharIndex();
                byteOff = anticipatedConv.nextByteIndex();
                badInputLength = anticipatedConv.badInputLength;
            } catch (UnknownCharacterException e) {
                charOff = anticipatedConv.nextCharIndex();
                byteOff = anticipatedConv.nextByteIndex();
                badInputLength = anticipatedConv.badInputLength;
                throw e;
            } catch (MalformedInputException e) {
                charOff = anticipatedConv.nextCharIndex();
                byteOff = anticipatedConv.nextByteIndex();
                badInputLength = anticipatedConv.badInputLength;
                throw e;
            } catch (ConversionBufferFullException e) {
                charOff = anticipatedConv.nextCharIndex();
                byteOff = anticipatedConv.nextByteIndex();
                badInputLength = anticipatedConv.badInputLength;
                throw e;
            }
        } else {
            try {
                num = defaultConv.convert(input, inOff, inEnd,
                            output, outOff, outEnd);
                charOff = defaultConv.nextCharIndex();
                byteOff = defaultConv.nextByteIndex();
                badInputLength = defaultConv.badInputLength;
            } catch (UnknownCharacterException e) {
                charOff = defaultConv.nextCharIndex();
                byteOff = defaultConv.nextByteIndex();
                badInputLength = defaultConv.badInputLength;
                throw e;
            } catch (MalformedInputException e) {
                charOff = defaultConv.nextCharIndex();
                byteOff = defaultConv.nextByteIndex();
                badInputLength = defaultConv.badInputLength;
                throw e;
            } catch (ConversionBufferFullException e) {
                charOff = defaultConv.nextCharIndex();
                byteOff = defaultConv.nextByteIndex();
                badInputLength = defaultConv.badInputLength;
                throw e;
            }
        }
        return num;
    }

    public void reset() {
        if (detectedConv != null) {
            detectedConv.reset();
            detectedConv = null;
            convName = null;
        } else
            defaultConv.reset();
    }

    public String getCharacterEncoding() {
        return "JISAutoDetect";
    }
}
