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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setjmp.h>

#include <jvmdi.h>

#include "util.h"
#include "SDE.h"

/**
 * This SourceDebugExtension code does not
 * allow concurrent translation - due to caching method.
 * A separate thread setting the default stratum ID 
 * is, however, fine.
 */

#define INIT_SIZE_LINE 100
#define INIT_SIZE_STRATUM 3

#define BASE_STRATUM_NAME "Java"

#define null NULL
#define true JNI_TRUE
#define false JNI_FALSE
#define String char *
#define private static

typedef struct {
    int jplsStart;
    int jplsEnd;
    int jplsLineInc;
    int njplsStart;
    int njplsEnd;
    int fileId;
} LineTableRecord;
        
typedef struct {
    String id;
    int fileIndex;
    int lineIndex;
} StratumTableRecord;

/* back-end wide value for default stratum */
private String globalDefaultStratumId = null;

/* reference type default */
private String defaultStratumId = null;

private jclass cachedClass = NULL;

private LineTableRecord* lineTable;
private StratumTableRecord* stratumTable;
private int lineTableSize;
private int stratumTableSize;

private int fileIndex;
private int lineIndex;
private int stratumIndex = 0;
private int currentFileId;

private int defaultStratumIndex;
private int baseStratumIndex;
private char* sdePos;

/* mangled in parse, cannot be parsed.  Must be kept. */
private String sourceDebugExtension;

private jboolean sourceMapIsValid;

private jmp_buf jmp_buf_env;

private int stratumTableIndex(String stratumId);
private int stiLineTableIndex(int sti, int jplsLine);
private int stiLineNumber(int sti, int lti, int jplsLine);
private void decode();
private void ignoreWhite();
private jboolean isValid();

    private void
    loadDebugInfo(JNIEnv *env, jclass clazz) {
        if (!(*env)->IsSameObject(env, clazz, cachedClass)) {
            /* Not the same - swap out the info */

            /* Delete existing info */
            (*env)->DeleteGlobalRef(env, cachedClass);
            cachedClass = null;
            jdwpFree(sourceDebugExtension);
            sourceDebugExtension = null; 

            /* Init info */
            lineTable = null;
            stratumTable = null;
            lineTableSize = 0;
            stratumTableSize = 0;
            fileIndex = 0;
            lineIndex = 0;
            stratumIndex = 0;
            currentFileId = 0;
            defaultStratumId = null;
            defaultStratumIndex = -1;
            baseStratumIndex = -2; /* so as not to match -1 above */
            sourceMapIsValid = false;

            if (getSourceDebugExtension(clazz, &sourceDebugExtension) ==
                JVMDI_ERROR_NONE) {
                sdePos = sourceDebugExtension;
                if (setjmp(jmp_buf_env) == 0) {
                    /* this is the initial (non-error) case, do parse */
                    decode();
                }
            }

            cachedClass = (*env)->NewGlobalRef(env, clazz);
        }
    }

    /**
     * Convert a line number table, as returned by the JVMDI
     * function GetLineNumberTable, to one for another stratum.
     * Conversion is by overwrite.
     * Actual line numbers are not returned - just a unique
     * number (file ID in top 16 bits, line number in 
     * bottom 16 bits) - this is all stepping needs.
     */
    void
    convertLineNumberTable(JNIEnv *env, jclass clazz,
                           jint *entryCountPtr, 
                           JVMDI_line_number_entry **tablePtr) {
        JVMDI_line_number_entry *fromEntry = *tablePtr;
        JVMDI_line_number_entry *toEntry = *tablePtr;
        int cnt = *entryCountPtr;
        int lastLn = 0;
        int sti;

        loadDebugInfo(env, clazz);
        if (!isValid()) {
            return; /* no SDE or not SourceMap - return unchanged */
        }
        sti = stratumTableIndex(globalDefaultStratumId);
        if (sti == baseStratumIndex) {
            return; /* Java stratum - return unchanged */
        }
        for (; cnt-->0; ++fromEntry) {
            int jplsLine = fromEntry->line_number;
            int lti = stiLineTableIndex(sti, jplsLine);
            if (lti >= 0) {
                int fileId = lineTable[lti].fileId;
                int ln = stiLineNumber(sti, lti, jplsLine);
                ln += (fileId << 16); /* create line hash */
                if (ln != lastLn) {
                    lastLn = ln;
                    toEntry->start_location = fromEntry->start_location;
                    toEntry->line_number = ln;
                    ++toEntry;
                }
            }
        }
        *entryCountPtr = toEntry - *tablePtr;
    }

    /**
     * Set back-end wide default stratum ID .
     */
    void
    setGlobalStratumId(char *id) {
        globalDefaultStratumId = id;
    }


    private void syntax(String msg) {
        char buf[200];
        sprintf(buf,
                "bad SourceDebugExtension syntax - position %d - %s\n", 
                (int)(sdePos-sourceDebugExtension),
                msg);
        JDI_ASSERT_FAILED(buf);

        longjmp(jmp_buf_env, 1);  /* abort parse */
    }

    private char sdePeek() {
        if (*sdePos == 0) {
            syntax("unexpected EOF");
        }
        return *sdePos;
    }

    private char sdeRead() {
        if (*sdePos == 0) {
            syntax("unexpected EOF");
        }
        return *sdePos++;
    }

    private void sdeAdvance() {
        sdePos++;
    }

    private void assureLineTableSize() {
        if (lineIndex >= lineTableSize) {
            size_t allocSize;

            lineTableSize = lineTableSize == 0?
                                  INIT_SIZE_LINE :
                                  lineTableSize * 2;
            allocSize = sizeof(LineTableRecord) * lineTableSize;
            
            lineTable = jdwpRealloc(lineTable, allocSize);
        }
    }

    private void assureStratumTableSize() {
        if (stratumIndex >= stratumTableSize) {
            size_t allocSize;

            stratumTableSize = stratumTableSize == 0?
                                  INIT_SIZE_STRATUM :
                                  stratumTableSize * 2;
            allocSize = sizeof(StratumTableRecord) * 
                                             stratumTableSize;
            
            stratumTable = jdwpRealloc(stratumTable, allocSize);
        }
    }

    private String readLine() {
        char *initialPos;
        char ch;

        ignoreWhite();
        initialPos = sdePos;
        while (((ch = *sdePos) != '\n') && (ch != '\r')) {
            if (ch == 0) {
                syntax("unexpected EOF");
            }
            ++sdePos;
        }
        *sdePos++ = 0; /* null terminate string - mangles SDE */

        /* check for CR LF */
        if ((ch == '\r') && (*sdePos == '\n')) {
            ++sdePos;
        }
        ignoreWhite(); /* leading white */
        return initialPos;
    }

    private int defaultStratumTableIndex() {
        if ((defaultStratumIndex == -1) && (defaultStratumId != null)) {
            defaultStratumIndex = 
                stratumTableIndex(defaultStratumId);
        }
        return defaultStratumIndex;
    }   

    private int stratumTableIndex(String stratumId) {
        int i;

        if (stratumId == null) {
            return defaultStratumTableIndex();
        }
        for (i = 0; i < (stratumIndex-1); ++i) {
            if (strcmp(stratumTable[i].id, stratumId) == 0) {
                return i;
            }
        }
        return defaultStratumTableIndex();
    }   


/*****************************
 * below functions/methods are written to compile under either Java or C
 * 
 * Needed support functions:
 *   sdePeek()
 *   sdeRead()
 *   sdeAdvance()
 *   readLine()
 *   assureLineTableSize()
 *   assureFileTableSize()
 *   assureStratumTableSize()
 *   syntax(String)
 *
 *   stratumTableIndex(String)
 *
 * Needed support variables:
 *   lineTable
 *   lineIndex
 *   fileTable
 *   fileIndex
 *   currentFileId
 *
 * Needed types:
 *   String
 *
 * Needed constants:
 *   NullString
 */

    private void ignoreWhite() {
        char ch;

        while (((ch = sdePeek()) == ' ') || (ch == '\t')) {
            sdeAdvance();
        }
    }

    private void ignoreLine() {
        char ch;

        while (((ch = sdeRead()) != '\n') && (ch != '\r')) {
        }
        /* check for CR LF */
        if ((ch == '\r') && (sdePeek() == '\n')) {
            sdeAdvance();
        }
        ignoreWhite(); /* leading white */
    }

    private int readNumber() {
        int value = 0;
        char ch;

        ignoreWhite();
        while (((ch = sdePeek()) >= '0') && (ch <= '9')) {
            sdeAdvance();
            value = (value * 10) + ch - '0';
        }
        ignoreWhite();
        return value;
    }

/***
    void storeFile(int fileId, String sourceName, String sourcePath) {
        assureFileTableSize();
        fileTable[fileIndex].fileId = fileId;
        fileTable[fileIndex].sourceName = sourceName;
        fileTable[fileIndex].sourcePath = sourcePath;
        ++fileIndex;
    }
***/

    private void fileLine() {
        int hasAbsolute = 0; /* acts as boolean */
        int fileId;
        String sourceName;
        String sourcePath = null;

        /* is there an absolute filename? */
        if (sdePeek() == '+') {
            sdeAdvance();
            hasAbsolute = 1;
        }
        fileId = readNumber();
        sourceName = readLine();
        if (hasAbsolute == 1) {
            sourcePath = readLine();
        }

        /* storeFile(fileId, sourceName, sourcePath); */
    }

    private void storeLine(int jplsStart, int jplsEnd, int jplsLineInc, 
                  int njplsStart, int njplsEnd, int fileId) {
        assureLineTableSize();
        lineTable[lineIndex].jplsStart = jplsStart;
        lineTable[lineIndex].jplsEnd = jplsEnd;
        lineTable[lineIndex].jplsLineInc = jplsLineInc;
        lineTable[lineIndex].njplsStart = njplsStart;
        lineTable[lineIndex].njplsEnd = njplsEnd;
        lineTable[lineIndex].fileId = fileId;
        ++lineIndex;
    }

    /**
     * Parse line translation info.  Syntax is
     *     <NJ-start-line> [ # <file-id> ] [ , <line-count> ] : 
     *                 <J-start-line> [ , <line-increment> ] CR
     */
    private void lineLine() {
        int lineCount = 1;
        int lineIncrement = 1;
        int njplsStart;
        int jplsStart;

        njplsStart = readNumber();

        /* is there a fileID? */
        if (sdePeek() == '#') {
            sdeAdvance();
            currentFileId = readNumber();
        }

        /* is there a line count? */
        if (sdePeek() == ',') {
            sdeAdvance();
            lineCount = readNumber();
        }

        if (sdeRead() != ':') {
            syntax("expected ':'");
        }
        jplsStart = readNumber();
        if (sdePeek() == ',') {
            sdeAdvance();
            lineIncrement = readNumber();
        }
        ignoreLine(); /* flush the rest */
        
        storeLine(jplsStart,
                  jplsStart + (lineCount * lineIncrement) -1,
                  lineIncrement,
                  njplsStart,
                  njplsStart + lineCount -1,
                  currentFileId);
    }

    /**
     * Until the next stratum section, everything after this
     * is in stratumId - so, store the current indicies.
     */
    private void storeStratum(String stratumId) {
        /* remove redundant strata */
        if (stratumIndex > 0) {
            if ((stratumTable[stratumIndex-1].fileIndex 
                                            == fileIndex) &&
                (stratumTable[stratumIndex-1].lineIndex 
                                            == lineIndex)) {
                /* nothing changed overwrite it */
                --stratumIndex;
            }
        }
        /* store the results */
        assureStratumTableSize();
        stratumTable[stratumIndex].id = stratumId;
        stratumTable[stratumIndex].fileIndex = fileIndex;
        stratumTable[stratumIndex].lineIndex = lineIndex;
        ++stratumIndex;
        currentFileId = 0;
    }

    /**
     * The beginning of a stratum's info
     */
    private void stratumSection() {
        storeStratum(readLine());
    }

    private void fileSection() {
        ignoreLine();
        while (sdePeek() != '*') {
            fileLine();
        }
    }

    private void lineSection() {
        ignoreLine();
        while (sdePeek() != '*') {
            lineLine();
        }
    }

    /**
     * Ignore a section we don't know about.
     */
    private void ignoreSection() {
        ignoreLine();
        while (sdePeek() != '*') {
            ignoreLine();
        }
    }

    /**
     * A base "Java" stratum is always available, though
     * it is not in the SourceDebugExtension.
     * Create the base stratum.
     */
    private void createJavaStratum() {
        baseStratumIndex = stratumIndex;
        storeStratum(BASE_STRATUM_NAME);
        /* storeFile(1, jplsFilename, NullString); */
        /* JPL line numbers cannot exceed 65535 */
        storeLine(1, 65536, 1, 1, 65536, 1);
        storeStratum("Aux"); /* in case they don't declare */
    }

    /**
     * Decode a SourceDebugExtension which is in SourceMap format.
     * This is the entry point into the recursive descent parser.
     */
    private void decode() {
        /* check for "SMAP" - allow EOF if not ours */
        if (strlen(sourceDebugExtension) <= 4 ||
            (sdeRead() != 'S') ||
            (sdeRead() != 'M') ||
            (sdeRead() != 'A') ||
            (sdeRead() != 'P')) {
            return; /* not our info */
        }
        ignoreLine(); /* flush the rest */
        ignoreLine();  /* jplsFilename */
        defaultStratumId = readLine();
        createJavaStratum();
        while (true) {
            if (sdeRead() != '*') {
                syntax("expected '*'");
            }
            switch (sdeRead()) {
                case 'S': 
                    stratumSection();
                    break;
                case 'F': 
                    fileSection();
                    break;
                case 'L': 
                    lineSection();
                    break;
                case 'E': 
                    /* set end points */
                    storeStratum("*terminator*"); 
                    sourceMapIsValid = true;
                    return;
                default:
                    ignoreSection();
            }
        }
    }

    /***************** query functions ***********************/

    private int stiLineTableIndex(int sti, int jplsLine) {
        int i;
        int lineIndexStart;
        int lineIndexEnd;

        lineIndexStart = stratumTable[sti].lineIndex;
        /* one past end */
        lineIndexEnd = stratumTable[sti+1].lineIndex; 
        for (i = lineIndexStart; i < lineIndexEnd; ++i) {
            if ((jplsLine >= lineTable[i].jplsStart) &&
                            (jplsLine <= lineTable[i].jplsEnd)) {
                return i;
            }
        }
        return -1;
    }

    private int stiLineNumber(int sti, int lti, int jplsLine) {
        return lineTable[lti].njplsStart + 
                (((jplsLine - lineTable[lti].jplsStart) / 
                                   lineTable[lti].jplsLineInc));
    }

    private jboolean isValid() {
        return sourceMapIsValid;
    }
