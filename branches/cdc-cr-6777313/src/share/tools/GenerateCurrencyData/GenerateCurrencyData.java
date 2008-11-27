/*
 * @(#)GenerateCurrencyData.java	1.7 06/10/10
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

import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Locale;
import java.util.Properties;
import java.util.TimeZone;

/**
 * Reads currency data in properties format from the standard input stream
 * and generates an equivalent Java source file on the standard output stream.
 * See CurrencyData.properties for the input format description and
 * Currency.java for the format descriptions of the generated tables.
 */
public class GenerateCurrencyData {

    // input data: currency data obtained from properties on input stream
    private static Properties currencyData;
    private static String validCurrencyCodes;
    private static String currenciesWith0MinorUnitDecimals;
    private static String currenciesWith1MinorUnitDecimal;
    private static String currenciesWithMinorUnitsUndefined;

    // generated data
    private static String mainTable;

    private static final int maxSpecialCases = 30;
    private static int specialCaseCount = 0;
    private static long[] specialCaseCutOverTimes = new long[maxSpecialCases];
    private static String[] specialCaseOldCurrencies = new String[maxSpecialCases];
    private static String[] specialCaseNewCurrencies = new String[maxSpecialCases];
    private static int[] specialCaseOldCurrenciesDefaultFractionDigits = new int[maxSpecialCases];
    private static int[] specialCaseNewCurrenciesDefaultFractionDigits = new int[maxSpecialCases];

    private static final int maxOtherCurrencies = 50;
    private static int otherCurrenciesCount = 0;
    private static StringBuffer otherCurrencies = new StringBuffer();
    private static int[] otherCurrenciesDefaultFractionDigits = new int[maxOtherCurrencies];

    // handy constants - must match definitions in java.util.Currency
    // number of characters from A to Z
    private static final int A_TO_Z = ('Z' - 'A') + 1;
    // entry for invalid country codes
    private static final int INVALID_COUNTRY_ENTRY = 0x007F;
    // entry for countries without currency
    private static final int COUNTRY_WITHOUT_CURRENCY_ENTRY = 0x0080;
    // mask for simple case country entries
    private static final int SIMPLE_CASE_COUNTRY_MASK = 0x0000;
    // mask for simple case country entry final character
    private static final int SIMPLE_CASE_COUNTRY_FINAL_CHAR_MASK = 0x001F;
    // mask for simple case country entry default currency digits
    private static final int SIMPLE_CASE_COUNTRY_DEFAULT_DIGITS_MASK = 0x0060;
    // shift count for simple case country entry default currency digits
    private static final int SIMPLE_CASE_COUNTRY_DEFAULT_DIGITS_SHIFT = 5;
    // mask for special case country entries
    private static final int SPECIAL_CASE_COUNTRY_MASK = 0x0080;
    // mask for special case country index
    private static final int SPECIAL_CASE_COUNTRY_INDEX_MASK = 0x001F;
    // delta from entry index component in main table to index into special case tables
    private static final int SPECIAL_CASE_COUNTRY_INDEX_DELTA = 1;
    // mask for distinguishing simple and special case countries
    private static final int COUNTRY_TYPE_MASK = SIMPLE_CASE_COUNTRY_MASK | SPECIAL_CASE_COUNTRY_MASK;
    
    // date format for parsing cut-over times
    private static SimpleDateFormat format;

    public static void main(String[] args) {
        format = new SimpleDateFormat("yyyy-MM-dd-HH-mm-ss", Locale.US);
        format.setTimeZone(TimeZone.getTimeZone("GMT"));
        format.setLenient(false);

        try {
            readInput();
            buildMainAndSpecialCaseTables();
            buildOtherTables();
            writeOutput();
        } catch (Exception e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace(System.err);
            System.exit(1);
        }
    }
    
    private static void readInput() throws IOException {
        currencyData = new Properties();
        currencyData.load(System.in);
        
        // initialize other lookup strings
        validCurrencyCodes = (String) currencyData.get("all");
        currenciesWith0MinorUnitDecimals  = (String) currencyData.get("minor0");
        currenciesWith1MinorUnitDecimal  = (String) currencyData.get("minor1");
        currenciesWithMinorUnitsUndefined  = (String) currencyData.get("minorUndefined");
        if (validCurrencyCodes == null ||
                currenciesWith0MinorUnitDecimals == null ||
                currenciesWith1MinorUnitDecimal == null ||
                currenciesWithMinorUnitsUndefined == null) {
            throw new NullPointerException("not all required data is defined in input");
        }
    }

    private static void buildMainAndSpecialCaseTables() throws Exception {
        char[] mainTableArray = new char[A_TO_Z*A_TO_Z];
        for (int first = 0; first < A_TO_Z; first++) {
            for (int second = 0; second < A_TO_Z; second++) {
                char firstChar = (char) ('A' + first);
                char secondChar = (char) ('A' + second);
                String countryCode = (new StringBuffer()).append(firstChar).append(secondChar).toString();
                String currencyInfo = (String) currencyData.get(countryCode);
                int tableEntry = 0;
                if (currencyInfo == null) {
                    // no entry -> must be invalid ISO 3166 country code
                    tableEntry = INVALID_COUNTRY_ENTRY;
                } else {
                    int length = currencyInfo.length();
                    if (length == 0) {
                        // special case: country without currency
                       tableEntry = COUNTRY_WITHOUT_CURRENCY_ENTRY;
                    } else if (length == 3) {
                        // valid currency
                        if (currencyInfo.charAt(0) == firstChar && currencyInfo.charAt(1) == secondChar) {
                            checkCurrencyCode(currencyInfo);
                            int digits = getDefaultFractionDigits(currencyInfo);
                            if (digits < 0 || digits > 3) {
                                throw new RuntimeException("fraction digits out of range for " + currencyInfo);
                            }
                            tableEntry = SIMPLE_CASE_COUNTRY_MASK
                                    | (currencyInfo.charAt(2) - 'A')
                                    | (digits << SIMPLE_CASE_COUNTRY_DEFAULT_DIGITS_SHIFT);
                        } else {
                            tableEntry = SPECIAL_CASE_COUNTRY_MASK | (makeSpecialCaseEntry(currencyInfo) + SPECIAL_CASE_COUNTRY_INDEX_DELTA);
                        }
                    } else {
                        tableEntry = SPECIAL_CASE_COUNTRY_MASK | (makeSpecialCaseEntry(currencyInfo) + SPECIAL_CASE_COUNTRY_INDEX_DELTA);
                    }
                }
                mainTableArray[first * A_TO_Z + second] = (char) tableEntry;
            }
        }
        mainTable = new String(mainTableArray);
    }
    
    private static int getDefaultFractionDigits(String currencyCode) {
        if (currenciesWith0MinorUnitDecimals.indexOf(currencyCode) != -1) {
            return 0;
        } else if (currenciesWith1MinorUnitDecimal.indexOf(currencyCode) != -1) {
            return 1;
        } else if (currenciesWithMinorUnitsUndefined.indexOf(currencyCode) != -1) {
            return -1;
        } else {
            return 2;
        }
    }
    
    static HashMap specialCaseMap = new HashMap();
    
    private static int makeSpecialCaseEntry(String currencyInfo) throws Exception {
        Integer oldEntry = (Integer) specialCaseMap.get(currencyInfo);
        if (oldEntry != null) {
            return oldEntry.intValue();
        }
        if (specialCaseCount == maxSpecialCases) {
            throw new RuntimeException("too many special cases");
        }
        if (currencyInfo.length() == 3) {
            checkCurrencyCode(currencyInfo);
            specialCaseCutOverTimes[specialCaseCount] = Long.MAX_VALUE;
            specialCaseOldCurrencies[specialCaseCount] = currencyInfo;
            specialCaseOldCurrenciesDefaultFractionDigits[specialCaseCount] = getDefaultFractionDigits(currencyInfo);
            specialCaseNewCurrencies[specialCaseCount] = null;
            specialCaseNewCurrenciesDefaultFractionDigits[specialCaseCount] = 0;
        } else {
            int length = currencyInfo.length();
            if (currencyInfo.charAt(3) != ';' ||
                    currencyInfo.charAt(length - 4) != ';') {
                throw new RuntimeException("invalid currency info: " + currencyInfo);
            }
            String oldCurrency = currencyInfo.substring(0, 3);
            String newCurrency = currencyInfo.substring(length - 3, length);
            checkCurrencyCode(oldCurrency);
            checkCurrencyCode(newCurrency);
            String timeString = currencyInfo.substring(4, length - 4);
            long time = format.parse(timeString).getTime();
            if (Math.abs(time - System.currentTimeMillis()) > ((long) 10) * 365 * 24 * 60 * 60 * 1000) {
                throw new RuntimeException("time is more than 10 years from present: " + time);
            }
            specialCaseCutOverTimes[specialCaseCount] = time;
            specialCaseOldCurrencies[specialCaseCount] = oldCurrency;
            specialCaseOldCurrenciesDefaultFractionDigits[specialCaseCount] = getDefaultFractionDigits(oldCurrency);
            specialCaseNewCurrencies[specialCaseCount] = newCurrency;
            specialCaseNewCurrenciesDefaultFractionDigits[specialCaseCount] = getDefaultFractionDigits(newCurrency);
        }
        specialCaseMap.put(currencyInfo, new Integer(specialCaseCount));
        return specialCaseCount++;
    }
    
    private static void buildOtherTables() {
        if (validCurrencyCodes.length() % 4 != 3) {
            throw new RuntimeException("\"all\" entry has incorrect size");
        }
        for (int i = 0; i < (validCurrencyCodes.length() + 1) / 4; i++) {
            if (i > 0 && validCurrencyCodes.charAt(i * 4 - 1) != '-') {
                throw new RuntimeException("incorrect separator in \"all\" entry");
            }
            String currencyCode = validCurrencyCodes.substring(i * 4, i * 4 + 3);
            checkCurrencyCode(currencyCode);
            int tableEntry = mainTable.charAt((currencyCode.charAt(0) - 'A') * A_TO_Z + (currencyCode.charAt(1) - 'A'));
            if (tableEntry == INVALID_COUNTRY_ENTRY ||
                    (tableEntry & SPECIAL_CASE_COUNTRY_MASK) != 0 ||
                    (tableEntry & SIMPLE_CASE_COUNTRY_FINAL_CHAR_MASK) != (currencyCode.charAt(2) - 'A')) {
                if (otherCurrenciesCount == maxOtherCurrencies) {
                    throw new RuntimeException("too many other currencies");
                }
                if (otherCurrencies.length() > 0) {
                    otherCurrencies.append('-');
                }
                otherCurrencies.append(currencyCode);
                otherCurrenciesDefaultFractionDigits[otherCurrenciesCount] = getDefaultFractionDigits(currencyCode);
                otherCurrenciesCount++;
            }
        }
    }
    
    private static void checkCurrencyCode(String currencyCode) {
        if (currencyCode.length() != 3) {
            throw new RuntimeException("illegal length for currency code: " + currencyCode);
        }
        for (int i = 0; i < 3; i++) {
            char aChar = currencyCode.charAt(i);
            if ((aChar < 'A' || aChar > 'Z') && !currencyCode.equals("XB5")) {
                throw new RuntimeException("currency code contains illegal character: " + currencyCode);
            }
        }
        if (validCurrencyCodes.indexOf(currencyCode) == -1) {
            throw new RuntimeException("currency code not listed as valid: " + currencyCode);
        }
    }
    
    private static void writeOutput() {
        System.out.println("package java.util;\n");
        System.out.println("class CurrencyData {\n");
        writeStaticString("mainTable", mainTable, A_TO_Z);
        writeStaticLongArray("scCutOverTimes", specialCaseCutOverTimes, specialCaseCount);
        writeStaticStringArray("scOldCurrencies", specialCaseOldCurrencies, specialCaseCount);
        writeStaticStringArray("scNewCurrencies", specialCaseNewCurrencies, specialCaseCount);
        writeStaticIntArray("scOldCurrenciesDFD", specialCaseOldCurrenciesDefaultFractionDigits, specialCaseCount);
        writeStaticIntArray("scNewCurrenciesDFD", specialCaseNewCurrenciesDefaultFractionDigits, specialCaseCount);
        writeStaticString("otherCurrencies", otherCurrencies.toString(), otherCurrenciesCount * 4);
        writeStaticIntArray("otherCurrenciesDFD", otherCurrenciesDefaultFractionDigits, otherCurrenciesCount);
        System.out.println("}\n");
    }
    
    private static void writeStaticString(String name, String content, int chunkSize) {
        String prefix = "    static final String " + name + " = ";
        System.out.print(prefix);
        System.out.print("\"");
        int inChunk = 0;
        for (int i = 0; i < content.length(); i++) {
            if (inChunk == chunkSize) {
                System.out.print("\" +\n");
                for (int j = 0; j < prefix.length(); j++) {
                    System.out.print(" ");
                }
                System.out.print("\"");
                inChunk = 0;
            }
            writeChar(content.charAt(i));
            inChunk++;
        }
        System.out.println("\";\n");
    }
    
    private static void writeStaticStringArray(String name, String[] content, int count) {
        System.out.print("    static final String[] " + name + " = { ");
        for (int i = 0; i < count; i++) {
            if (content[i] == null) {
                System.out.print("null");
            } else {
                writeString(content[i]);
            }
            System.out.print(", ");
        }
        System.out.println("};\n");
    }
    
    private static void writeStaticIntArray(String name, int[] content, int count) {
        System.out.print("    static final int[] " + name + " = { ");
        for (int i = 0; i < count; i++) {
            System.out.print(content[i]);
            System.out.print(", ");
        }
        System.out.println("};\n");
    }
    
    private static void writeStaticLongArray(String name, long[] content, int count) {
        System.out.print("    static final long[] " + name + " = { ");
        for (int i = 0; i < count; i++) {
            System.out.print(content[i]);
            System.out.print("L, ");
        }
        System.out.println("};\n");
    }
    
    private static void writeString(String string) {
        System.out.print("\"");
        for (int i = 0; i < string.length(); i++) {
            writeChar(string.charAt(i));
        }
        System.out.print("\"");
    }
    
    private static void writeChar(char aChar) {
        if (aChar == '\n') {
            System.out.print("\\n");
        } else if (aChar == '\r') {
            System.out.print("\\r");
        } else if (aChar >= '\u0020' && aChar < '\u007F') {
            System.out.print(aChar);
        } else {
            System.out.print("\\u");
            String hexString = Integer.toHexString(aChar);
            for (int i = 0; i < 4 - hexString.length(); i++) {
                System.out.print("0");
            }
            System.out.print(hexString);
        }
    }
}
