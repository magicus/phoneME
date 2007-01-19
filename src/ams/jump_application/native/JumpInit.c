
#include <kni.h>
#include <jvm.h>
#include <midpAMS.h>
#include <midpStorage.h>
#include <suitestore_common.h>
                                                                                   
/**
 * Initializes the native storage.
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_jump_JumpInit_initMidpNativeStates) {
    jchar jbuff[1024];
    char cbuff[1024];
    int max = sizeof(cbuff) - 1;
    int len, i, err;
    MIDPError status;

    KNI_StartHandles(1);
    KNI_DeclareHandle(home);
    KNI_GetParameterAsObject(1, home);
                                                                                   
    len = KNI_GetStringLength(home);
    if (len > max) {
        len = max;
    }
                                                                                   
    KNI_GetStringRegion(home, 0, len, jbuff);
    for (i=0; i<len; i++) {
        cbuff[i] = (char)jbuff[i];
    }
    cbuff[len] = 0;

    midpSetHomeDir(cbuff);
    err = storageInitialize(cbuff);

    if (err == 0) {
         status = midp_suite_storage_init();
    } else {
         status = OUT_OF_MEMORY;
    }

    KNI_EndHandles();

    switch(status) {
        case OUT_OF_MEMORY:
            KNI_ReturnInt(100);
            break;
        case ALL_OK:
            KNI_ReturnInt(1);
        default:
            break;
    }

    if (status != ALL_OK) {
       KNI_ReturnBoolean(KNI_FALSE);
    } else {
       KNI_ReturnBoolean(KNI_TRUE);
    }
}

