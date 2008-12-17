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

#if ENABLE_NUTS_FRAMEWORK

#include <fileInstallerTest.h>
#include <midp_logging.h>

char* test_jad_1 = "testReadJadFile_1";
char* test_jad_2 = "testReadJadFile_2";
char* fileInstallerTest = "fileInstallerTest";
char* fileInstallerTestJar = "fileInstallerTestJar";
char* fileInstallerTestBadJad = "fileInstallerTestBadJad";

char* testMidpGetVersion_1m = "testMidpGetVersion_1";
char* testMidpGetVersion_2m = "testMidpGetVersion_2";
char* testMidpGetVersion_3m = "testMidpGetVersion_3";
char* testMidpGetVersion_4m = "testMidpGetVersion_4";
char* testMidpCompareVersion_1m = "testMidpCompareVersion_1";
char* testMidpCompareVersion_2m = "testMidpCompareVersion_2";
char* testMidpCompareVersion_3m = "testMidpCompareVersion_3";
char* testCreateRelativeURL_1m = "testCreateRelativeURL_1";
char* testCreateRelativeURL_2m = "testCreateRelativeURL_2";

char* testParseJad_1m = "testParseJad_1";
char* testParseJad_2m = "testParseJad_2";
char* testParseManifest_1m = "testParseManifest_1";
char* testParseManifest_2m = "testParseManifest_2";

char* myArgv[2] = {"test","games.jad"};
char* mrgv[2] = {"test","games.jar"};
char* mrgvbad[2] = {"test","gamesbad.jad"};

int registerFileInstallerTests() {

    if (!register_test(test_jad_1,testReadJadFile_1)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n", test_jad_1);
    }

    if (!register_test(test_jad_2,testReadJadFile_2)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",test_jad_2);
    }

    if (!register_test(testCreateRelativeURL_1m,testCreateRelativeURL_1)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testCreateRelativeURL_1m);
    }

    if (!register_test(testCreateRelativeURL_2m,testCreateRelativeURL_2)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testCreateRelativeURL_2m);
    }

    if (!register_test(testMidpGetVersion_1m,testMidpGetVersion_1)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testMidpGetVersion_1m);
    }

    if (!register_test(testMidpGetVersion_2m,testMidpGetVersion_2)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testMidpGetVersion_2m);
    }

    if (!register_test(testMidpGetVersion_3m,testMidpGetVersion_3)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testMidpGetVersion_3m);
    }

    if (!register_test(testMidpGetVersion_4m,testMidpGetVersion_4)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testMidpGetVersion_4m);
    }

    if (!register_test(testMidpCompareVersion_1m,testMidpCompareVersion_1)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testMidpCompareVersion_1m);
    }

    if (!register_test(testMidpCompareVersion_2m,testMidpCompareVersion_2)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testMidpCompareVersion_2m);
    }

    if (!register_test(testMidpCompareVersion_3m,testMidpCompareVersion_3)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testMidpCompareVersion_3m);
    }

        /*
    if (!register_test(fileInstallerTest,testInstalFileUsingJad)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",fileInstallerTest);
}

    if (!register_test(fileInstallerTestJar,testInstalFileUsingJar)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",fileInstallerTestJar);
}

    if (!register_test(fileInstallerTestBadJad,testInstalFileUsingBadJad)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",fileInstallerTestBadJad);
}
    */

    /*
    if (!register_test(testParseJad_1m,testParseJad_1)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testParseJad_1m);
}

    if (!register_test(testParseJad_2m,testParseJad_2)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testParseJad_2m);
}

    if (!register_test(testParseManifest_1m,testParseManifest_1)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testParseManifest_1m);
}

    if (!register_test(testParseManifest_2m,testParseManifest_2)) {
        REPORT_WARN1(LC_AMS, "Registration of test %s failed.\n",testParseManifest_2);
}
    */
  return 1;

} /* end of registerFileInstallerTests */

int testReadJadFile_1(void) {
    int    jadsize   =  0;
    char*  jad_buf   =  NULL;
    char*  jad       =  "../../../src/common/native/share/unittests/jad1.jad";
    int    res       =  ALL_OK;
    MidpString jadURL = {0,NULL};

    REPORT_INFO(LC_AMS, "############# This test should pass.");
    jadURL = midpCharsToJchars(jad);
    jadsize = (int)readJadFile (jadURL, &jad_buf);
    if ((jadsize <= 0) || (!jad_buf)) {
        REPORT_WARN1(LC_AMS, "Can't open JAD file %s", jad);
        res = NO_JAD_FILE;
    } else {
        REPORT_INFO1(LC_AMS, "JAD content is:\n%s\n", jad_buf);
    }

    midpFreeString(jadURL);

    if (jad_buf) {
        midpFree(jad_buf);
    } /* end of if */

    return res;
} /* testReadJadFile_1 */

int testReadJadFile_2(void) {
    int    jadsize   =  0;
    char*  jad_buf   =  NULL;
    char*  jad       =  "../../../src/common/native/share/unittests/no_such_a_file.jad";
    int    res       =  NO_JAD_FILE;
    MidpString jadURL = {0,NULL};

    REPORT_INFO(LC_AMS, "#############  This test should fail. Trying to load file that doesn't exist.\n");
    jadURL = midpCharsToJchars(jad);
    jadsize = (int)readJadFile (jadURL, &jad_buf);
    if ((jadsize <= 0) || (!jad_buf)) {
        REPORT_WARN1(LC_AMS, "\nCan't open JAD file %s\n", jad);
        res = ALL_OK;
    } else {
        REPORT_INFO1(LC_AMS, "JAD content is:\n%s\n", jad_buf);
    }

    midpFreeString(jadURL);

    if (jad_buf) {
        midpFree(jad_buf);
    } /* end of if */

    return res;
} /* testReadJadFile_2 */

extern int fileInstaller(int, char** );

int testInstalFileUsingJad(void) {
    int    res    =  ALL_OK;
    int    myArgc = 2;

    REPORT_INFO(LC_AMS, "#############  This test should pass.\n");
    res = fileInstaller(myArgc, myArgv);

    if (res != ALL_OK) {
        REPORT_WARN2(LC_AMS, "\n%s failed. Result code is %d\n", 
		    fileInstallerTest, res);
    } else {
        REPORT_INFO1(LC_AMS, "%s passed\n", fileInstallerTest);
    }

    return res;
} /* testInstalFileUsingJad */

int testInstalFileUsingJar(void) {
    int    res    =  ALL_OK;
    int    myArgc = 2;

    REPORT_INFO(LC_AMS, "#############  This test should pass.\n");
    res = fileInstaller(myArgc, mrgv);

    if (res != ALL_OK) {
        REPORT_WARN2(LC_AMS, "\n%s failed. Result code is %d\n",
		     fileInstallerTestJar, res);
    } else {
        REPORT_INFO1(LC_AMS, "%s passed\n", fileInstallerTestJar);
    }
    return res;
} /* testInstalFileUsingJad */

int testInstalFileUsingBadJad(void) {
    int    res    =  -1;
    int    myArgc = 2;

    REPORT_INFO(LC_AMS, "#############  This test should fail. Using Jad without JAR-Size\n");
    res = fileInstaller(myArgc, mrgvbad);

    if (res >= 0) {
        REPORT_WARN2(LC_AMS, "\n%s failed. Result code is %d\n",
		     fileInstallerTestBadJad, res);
        res = -1;
    } else {
        REPORT_INFO1(LC_AMS, "%s passed\n", fileInstallerTestBadJad);
        res = ALL_OK;
    }
    return res;
}

/* negative test */
int testMidpGetVersion_1(void) {
    int major;
    int minor;
    int micro;
    int res = 0;
    jchar ver[] = {'1','2','3','.','0','.','2','3'};
    MidpString in = {sizeof (ver) / sizeof (jchar),ver};
    res = midpGetVersion(in, &major, &minor, &micro);
    REPORT_INFO1(LC_AMS, "testMidpGetVersion_1 res = %d\n", res);
    if(res == 0) {
        return 1;
    } /* end of if */
    return -1;
}

/* negative test */
int testMidpGetVersion_2(void) {
    int major;
    int minor;
    int micro;
    int res = 0;
    jchar ver[] = {'1','.','0','.','0','.','2','.','3'};
    MidpString in = {sizeof (ver) / sizeof (jchar),ver};
    res = midpGetVersion(in, &major, &minor, &micro);
    REPORT_INFO1(LC_AMS, "testMidpGetVersion_2 res = %d\n", res);
    if(res == 0) {
        return 1;
    } /* end of if */
    return -1;
}

/* positive test */
int testMidpGetVersion_3(void) {
    int major;
    int minor;
    int micro;
    int res = 0;
    jchar ver[] = {'1','.','0','.','2','3'};
    MidpString in = {sizeof (ver) / sizeof (jchar),ver};
    res = midpGetVersion(in, &major, &minor, &micro);
    REPORT_INFO1(LC_AMS, "testMidpGetVersion_3 res = %d\n", res);
    if(res != 1) {
        return -1;
    } /* end of if */
    return 1;
}

/* positive test */
int testMidpGetVersion_4(void) {
    int major;
    int minor;
    int micro;
    int res = 0;
    jchar ver[] = {'1','1','.','0','1','.','2','3'};
    MidpString in = {sizeof (ver) / sizeof (jchar),ver};
    res = midpGetVersion(in, &major, &minor, &micro);
    REPORT_INFO1(LC_AMS, "testMidpGetVersion_4 res = %d\n", res);
    if(res != 1) {
        return -1;
    } /* end of if */
    return 1;
}

int testMidpCompareVersion_1(void) {
    jchar ver1[] = {'1','.','1','.','2'};
    MidpString in1 = {sizeof (ver1) / sizeof (jchar),ver1};
    jchar ver2[] = {'1','.','0','.','2'};
    MidpString in2 = {sizeof (ver2) / sizeof (jchar),ver2};
    int res = 0;
    res = midpCompareVersion(in1,in2);
    REPORT_INFO1(LC_AMS, "testMidpCompareVersion_1 res = %d\n", res);
    if(res > 0 ) {
        return 1;
    }
    return -1;
}

int testMidpCompareVersion_2(void) {
    jchar ver1[] = {'1','.','0','.','2','3'};
    MidpString in1 = {sizeof (ver1) / sizeof (jchar),ver1};
    jchar ver2[] = {'1','.','0','.','2','3'};
    MidpString in2 = {sizeof (ver2) / sizeof (jchar),ver2};
    int res = 0;
    res = midpCompareVersion(in1,in2);
    REPORT_INFO1(LC_AMS, "testMidpCompareVersion_2 res = %d\n", res);
    if(res == 0 ) {
        return 1;
    }
    return -1;
}

int testMidpCompareVersion_3(void) {
    jchar ver1[] = {'1','.','0','.','2','3'};
    MidpString in1 = {sizeof (ver1) / sizeof (jchar),ver1};
    jchar ver2[] = {'1','.','0','.','2','4'};
    MidpString in2 = {sizeof (ver2) / sizeof (jchar),ver2};
    int res = 0;
    res = midpCompareVersion(in1,in2);
    REPORT_INFO1(LC_AMS, "testMidpCompareVersion_3 res = %d\n", res);
    if(res < 0 ) {
        return 1;
    }
    return -1;
}


int testCreateRelativeURL_1(void) {
    jchar URL[] = {'h','t','t','p',':','/','/','a','b','/','.','.','/','.','.','/','k','.','j','a','d'};
    MidpString url = {sizeof (URL) / sizeof (jchar),URL};
    MidpString relativeURL = {0,NULL};
    int res = ALL_OK;
    relativeURL = createRelativeURL(url);
    if (relativeURL.len > 0) {
        printMidpStringWithMessage("testCreateRelativeURL_1", relativeURL);
        midpFreeString(relativeURL);
        return res;
    } else {
        res = -1;
    }
    return res;
}

int testCreateRelativeURL_2(void) {
    jchar URL[] = {'.','/','a','b','/','.','.','/','.','.','/','k','.','j','a','d'};
    MidpString url = {sizeof (URL) / sizeof (jchar),URL};
    MidpString relativeURL = {0,NULL};
    int res = ALL_OK;
    relativeURL = createRelativeURL(url);
    if (relativeURL.len > 0) {
        printMidpStringWithMessage("testCreateRelativeURL_2", relativeURL);
        midpFreeString(relativeURL);
        return res;
    } else {
        res = -1;
    }
    return res;
}

int testParseJad_1(void) {
    return 1;
}

int testParseJad_2(void) {
    return 1;
}

int testParseManifest_1(void) {
    return 1;
}

int testParseManifest_2(void) {
    return 1;
}

#endif /* ENABLE_NUTS_FRAMEWORK */
