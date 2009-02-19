/*
 * @(#)QtImageDecoder.cc	1.6 06/10/25
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
#include <qt.h>
#include <qwidget.h>
#include "jni.h"
#include "awt.h"
#include "sun_awt_qt_QtImageDecoder.h"

/*
 * Class:     sun_awt_qt_QtImageDecoder
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtImageDecoder_initIDs (JNIEnv *env, jclass cls)
{
    QtCachedIDs.QtImageDecoderClass = (jclass)env->NewGlobalRef (cls);

    GET_FIELD_ID(QtImageDecoder_widthFID,"width", "I");
    GET_FIELD_ID(QtImageDecoder_heightFID,"height", "I");
    GET_FIELD_ID (QtImageDecoder_colorModelFID, "colorModel", 
                  "Ljava/awt/image/ColorModel;");
    GET_STATIC_FIELD_ID (QtImageDecoder_RGB24DCMFID, 
                         "RGB24_DCM", 
                         "Ljava/awt/image/DirectColorModel;");
    GET_STATIC_FIELD_ID (QtImageDecoder_RGB32DCMFID, 
                         "RGB32_DCM", 
                         "Ljava/awt/image/DirectColorModel;");

    /* created in QtToolkit and is initialized before QtImageDecoderr */
    cls = QtCachedIDs.java_awt_image_IndexColorModelClass ;
    GET_METHOD_ID (java_awt_image_IndexColorModel_constructor2MID, 
                   "<init>", 
                   "(II[BIZ)V");
}

/*
 * Class:     sun_awt_qt_QtImageDecoder
 * Method:    getPixels
 * Signature: (II[B)V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtImageDecoder_getPixels__II_3B(JNIEnv *env, 
                                                jclass cls, 
                                                jint imageHandle, 
                                                jint line, 
                                                jbyteArray pixelArray){
    QImage *image = (QImage *)imageHandle ;

    /* get the byte array elements */
    jbyte *pixels = env->GetByteArrayElements(pixelArray, NULL);

    /*
     * The caller is responsible for making sure that "pixels" can hold
     * atleast a single scanline of data
     */
    memcpy(pixels, image->scanLine(line), image->bytesPerLine()) ;

    /* commit the changes made to the array elements */
    env->ReleaseByteArrayElements(pixelArray, pixels, JNI_COMMIT);
}

/*
 * Class:     sun_awt_qt_QtImageDecoder
 * Method:    getPixels
 * Signature: (II[I)V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtImageDecoder_getPixels__II_3I(JNIEnv *env, 
                                                jclass cls, 
                                                jint imageHandle, 
                                                jint line, 
                                                jintArray pixelArray) {
    QImage *image = (QImage *)imageHandle ;

    /* get the byte array elements */
    jint *pixels = env->GetIntArrayElements(pixelArray, NULL);

    /*
     * The caller is responsible for making sure that "pixels" can hold
     * atleast a single scanline of data
     */
    memcpy(pixels, image->scanLine(line), image->bytesPerLine()) ;

    /* commit the changes made to the array elements */
    env->ReleaseIntArrayElements(pixelArray, pixels, JNI_COMMIT);
}

/*
 * Class:     sun_awt_qt_QtImageDecoder
 * Method:    decodeImage
 * Signature: ([BI)I
 */
JNIEXPORT jint JNICALL 
Java_sun_awt_qt_QtImageDecoder_decodeImage(JNIEnv *env, 
                                           jobject thisObj, 
                                           jbyteArray dataBuffer,
                                           jint size) {
    jbyte *data ;
    data = env->GetByteArrayElements ( dataBuffer, NULL);

    QImage *image = new QImage() ;
    if ( image == NULL ) 
        return (jint)0 ;

    bool status = image->loadFromData((uchar *)data,size) ;
    if ( status == 0 ) {
        delete image ;
        return (jint)0 ;
    }

#if 0
    printf("image: (%d %d) depth %d numColors %d numBytes %d bytesPerLine %d "
           "ct=%x isGrayScale=%s hasAlpha=%s\n",
           image->width(),
           image->height(),
           image->depth(),
           image->numColors(),
           image->numBytes(),
           image->bytesPerLine(),
           image->colorTable(),
           (image->isGrayscale()?"true":"false"),
           (image->hasAlphaBuffer()?"true":"false")) ;
#endif

    int depth = image->depth() ;
    QRgb *colorTable = image->colorTable() ;
    jobject colorModel = NULL ;
    if ( colorTable != NULL ) { /* IndexColorModel */
        int numColorsInColorTable = image->numColors() ;
        jobject rgbaArray ;
        int i = 0 ;

        /* create a byte[] depending on the alpha of the image */
        int ct_size = sizeof(QRgb)*numColorsInColorTable ;
        if ( image->hasAlphaBuffer() ) {
            rgbaArray = env->NewByteArray(ct_size) ;
        }
        else {
            /* we dont need to send alpha byte so we decrement the size
             * of color map by "sizeof(byte) * numColorsInColorTable"
             */
            rgbaArray = env->NewByteArray(ct_size-numColorsInColorTable) ;
        }

        if (rgbaArray == NULL) {
            goto exit_on_failure ;
        }

        jbyte *rgbaElem = env->GetByteArrayElements((jbyteArray)rgbaArray, 0);

        if ( rgbaElem == NULL) {
            goto exit_on_failure ;
        }

        if ( image->hasAlphaBuffer() ) {
            /* copy the color table RGB data into java byte array */
            memcpy(rgbaElem,colorTable,sizeof(QRgb)*numColorsInColorTable) ;

            /* QRgb when treated as byte pointer contains the RGBA as follows
             * bytePointer[0] = Blue 
             * bytePointer[1] = Green
             * bytePointer[2] = Red
             * bytePointer[3] = Alpha
             *
             * Since we need to create a byte[] with RGBA interleaved, 
             * we should swap contents of bytePointer[0] and bytePointer[2] 
             */
            uchar *rgbaPtr ;
            int tmp ;
            for ( i = 0 ; i < numColorsInColorTable ; i++ ) {
                rgbaPtr = (uchar *)(rgbaElem+(sizeof(QRgb)*i));
                tmp = rgbaPtr[0] ;
                rgbaPtr[0] = rgbaPtr[2] ;
                rgbaPtr[2] = tmp ;
            }
        }
        else {
            /* no alpha in color map */
            int j = 0 ;
            for ( i = 0 ; i < numColorsInColorTable ; i++ ) {
                rgbaElem[j++] = qRed(colorTable[i]) ;
                rgbaElem[j++] = qGreen(colorTable[i]) ;
                rgbaElem[j++] = qBlue(colorTable[i]) ;
            }
        }

        /* commit the changes made to the array elements */
        env->ReleaseByteArrayElements((jbyteArray)rgbaArray,
                                      rgbaElem, 
                                      JNI_COMMIT);

        /* create a IndexColorModel with the above information */
        colorModel =
        env->NewObject(QtCachedIDs.java_awt_image_IndexColorModelClass,
             QtCachedIDs.java_awt_image_IndexColorModel_constructor2MID,
                       depth,
                       numColorsInColorTable,
                       rgbaArray,
                       0,
                       ((jboolean)(image->hasAlphaBuffer()))) ;
    }
    else { /* DirectColorModel */
        switch (depth) {
        case 32 :
        case 24 :
            if ( image->hasAlphaBuffer() ) 
                colorModel = env->GetStaticObjectField(
                             QtCachedIDs.QtImageDecoderClass,
                             QtCachedIDs.QtImageDecoder_RGB32DCMFID) ;
            else
                colorModel = env->GetStaticObjectField(
                             QtCachedIDs.QtImageDecoderClass,
                             QtCachedIDs.QtImageDecoder_RGB24DCMFID) ;
            break ;

        case 16 :
        default :
            goto exit_on_failure ;
            break ;
        }
    }

    /*
     * set the dimension of the image
     */
    env->SetIntField (thisObj, 
                      QtCachedIDs.QtImageDecoder_widthFID,
                      (jint)image->width()) ;
    env->SetIntField (thisObj, 
                      QtCachedIDs.QtImageDecoder_heightFID,
                      (jint)image->height()) ;

    /*
     * set the color model of the image
     */
    env->SetObjectField (thisObj, 
                         QtCachedIDs.QtImageDecoder_colorModelFID,
                         colorModel) ;

    return (jint)image ;

exit_on_failure :
    delete image ;
    return (jint)0 ;
}

/*
 * Class:     sun_awt_qt_QtImageDecoder
 * Method:    disposeImage
 * Signature: (I)V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_qt_QtImageDecoder_disposeImage(JNIEnv *env, 
                                            jobject thisObj, 
                                            jint qimage) {
    delete (QImage *)qimage ;
}


