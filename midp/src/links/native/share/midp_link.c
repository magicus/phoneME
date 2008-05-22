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

#include <jvm.h>
#include <kni.h>
#include <kni_globals.h>
#include <midp_libc_ext.h>
#include <midp_logging.h>
#include <midp_thread.h>
#include <midpError.h>
#include <midpServices.h>
#include <pcsl_memory.h>
#include <sni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* const midpClosedLinkException =
    "com/sun/midp/links/ClosedLinkException";
    /* IMPL_NOTE - move to midpError.h? */


/**
 * Native methods for the com.sun.midp.links.Link class.  Definitions for a
 * native data structure representing a rendezvous point that underlies a pair
 * of Link objects.
 *
 * While sending and receiving threads are blocked, they each put a reference
 * to a LinkMessage object into the rendezvous point. When awakening, the
 * reference is checked against the one that's a parameter to the native
 * method. If they are the same object, processing continues. If they are
 * different objects, this indicates that another thread is in the midst of an
 * operation on this Link, so the calling thread simply goes back to sleep.
 *
 * This works unambiguously for receivers, since the receive() method creates 
 * a unique LinkMessage object whenever it's called. It's possible for 
 * multiple threads to call send() on the same LinkMessage object. Since
 * they're sending the same object, it doesn't matter which thread is 
 * considered to have processed it -- the order of execution doesn't matter.
 *
 * IMPL_NOTE - use AddStrongReference or AddWeakReference?
 *
 * IMPL_NOTE - test for out-of-memory after AddStrongReference
 */

#define INVALID_REFERENCE_ID (-1)

/**
 * The current state of the rendezvous point.  The initial state is IDLE. 
 * There are two normal paths through the state machine, depending upon 
 * whether a receiver or sender thread arrives first.
 *
 * The state transitions are asymmetrical because of the need for the receiver
 * to allocate an object that will receive the message. It doesn't know what
 * to allocate until a sender arrives with a message. Therefore, there is an
 * extra set of context switches if the receiver arrives first.
 *
 * If the sender arrives first, the transitions are as follows:
 * 
 *   IDLE
 *      - sender calls send0() and blocks
 *   SENDING
 *      - receiver calls receive0(), transfers message, sets retcode,
 *        awakens sender, and continues
 *   DONE
 *      - sender awakens and cleans up
 *   IDLE
 * 
 * If the receiver blocks first, the transitions are as follows:
 *
 *   IDLE
 *      - receiver calls receive0() and blocks
 *   RECEIVING
 *      - sender calls send0(), awakens receiver, and blocks
 *   RENDEZVOUS
 *      - receiver awakens, transfers message, sets retcode,
 *        awakens sender, and continues
 *   DONE
 *      - sender awakens and cleans up
 *   IDLE
 *
 * The close() call will move the state to CLOSED and awaken any waiters.
 */
typedef enum {
    IDLE,           /* no operations pending */
    SENDING,        /* a thread has called send() and awaits a receiver */
    RECEIVING,      /* a thread has called receive() and awaits a sender */
    RENDEZVOUS,     /* receiver and sender threads have rendezvoused */
    DONE,           /* the receiver is done with the transfer */
    CLOSED          /* no further operations permitted */
} state_t;


/**
 * Return code set by the collaborating thread. That is, if a thread is 
 * blocked in receive(), a thread that calls send() is the collaborating 
 * thread, and vice-versa. This value is significant only when the rendezvous 
 * state is RECEIVED or SENT.
 */
typedef enum {
    OK,             /* collaborator transferred a message successfully */
    ERROR           /* collaborator failed for some reason */
} retcode_t;


/**
 * Implements the concept of a "rendezvous point" as defined in the JSR-121 
 * specification.
 */
typedef struct _rendezvous {
    state_t     state;      /* current state */
    retcode_t   retcode;    /* collaborator's return code */
    int         refcount;   /* num Java objs pointing to this struct */
    jint        msg;        /* refId for the sender's pending message */
    int         sender;     /* the isolate ID of the sender */
    int         receiver;   /* the isolate ID of the receiver */
} rendezvous;


/**
 * An entry in the array of portals.  Count is the number of rendezvous 
 * points. Its value is -1 if none have been set, 0 if set to an empty array, 
 * or >0 if there is an actual array. The rppa field points to the array of 
 * rendezvous points, or NULL if not allocated. Note that if count is 0, rppa 
 * will be NULL but the array is considered to have been set.
 */
typedef struct _portal {
    int         count;   /* the number of rendezvous points */
    rendezvous  **rppa;  /* array of pointers to rendezvous points */
} portal;


/**
 * Pointer to an array of portal entries.  Allocated lazily; will have size of
 * JVM_MaxIsolates().
 *
 * IMPL_NOTE: need to deal with initialization and finalization.
 */
static portal *portals = NULL;


#if ENABLE_I3_TEST
static void log_rp_free(rendezvous *);
#endif


/**
 * Creates a new rendezvous point with the given sender and receiver. Returns 
 * a pointer to the rendezvous point, otherwise NULL if out of memory.
 */
static rendezvous *
rp_create(int sender, int receiver) {
    rendezvous *rp;

    rp = (rendezvous *)pcsl_mem_malloc(sizeof(rendezvous));
    if (rp == NULL) {
        return NULL;
    }

    rp->state = IDLE;
    rp->retcode = OK;
    rp->refcount = 0;
    rp->msg = INVALID_REFERENCE_ID;
    rp->sender = sender;
    rp->receiver = receiver;

    return rp;
}


static void
rp_incref(rendezvous *rp)
{
    rp->refcount += 1;
}


static void
rp_decref(rendezvous *rp)
{
    rp->refcount -= 1;
    if (rp->refcount == 0) {
        if (rp->msg != INVALID_REFERENCE_ID) {
            /* IMPL_NOTE: really should be an assertion failure */
            KNI_FatalError("rp_decref refcount 0 with stale refid!");
        }
#if ENABLE_I3_TEST
        log_rp_free(rp);
#endif
        pcsl_mem_free(rp);
    }
}


/*
 * Checks refId for validity, gets its handle value, and stores it
 * into obj.  An error message incorporating msg is emitted if refId is 
 * invalid or if the handle returns is null.
 */
static void
getReference(int refId, char *msg, jobject obj)
{
    if (refId == INVALID_REFERENCE_ID) {
        midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
            "invalid reference ID in %s", msg);
        REPORT_CRIT(LC_CORE, gKNIBuffer);
        KNI_ReleaseHandle(obj);
    } else {
        SNI_GetReference(refId, obj);
        if (KNI_IsNullHandle(obj)) {
            midp_snprintf(gKNIBuffer, KNI_BUFFER_SIZE,
                "null reference from SNI_GetReference in %s", msg);
            REPORT_CRIT(LC_CORE, gKNIBuffer);
        }
    }
}


static void
setNativePointer(jobject linkObj, rendezvous *rp)
{
    jfieldID nativePointerField;

    KNI_StartHandles(1);
    KNI_DeclareHandle(linkClass);

    KNI_GetObjectClass(linkObj, linkClass);
    nativePointerField = KNI_GetFieldID(linkClass, "nativePointer", "I");
    KNI_SetIntField(linkObj, nativePointerField, (jint)rp);

    KNI_EndHandles();
}


static rendezvous *
getNativePointer(jobject linkObj)
{
    rendezvous *rp;
    jfieldID nativePointerField;

    KNI_StartHandles(1);
    KNI_DeclareHandle(linkClass);

    KNI_GetObjectClass(linkObj, linkClass);
    nativePointerField = KNI_GetFieldID(linkClass, "nativePointer", "I");
    rp = (rendezvous *)KNI_GetIntField(linkObj, nativePointerField);

    KNI_EndHandles();
    return rp;
}


static void
getContents(jobject linkMessageObj, jobject contentsObj)
{
    jfieldID contentsField;

    KNI_StartHandles(1);
    KNI_DeclareHandle(linkMessageClass);

    KNI_GetObjectClass(linkMessageObj, linkMessageClass);
    contentsField = KNI_GetFieldID(linkMessageClass, "contents",
        "Ljava/lang/Object;");
    KNI_GetObjectField(linkMessageObj, contentsField, contentsObj);

    KNI_EndHandles();
}


static void
setContents(jobject linkMessageObj, jobject contentsObj)
{
    jfieldID contentsField;

    KNI_StartHandles(1);
    KNI_DeclareHandle(linkMessageClass);

    KNI_GetObjectClass(linkMessageObj, linkMessageClass);
    contentsField = KNI_GetFieldID(linkMessageClass, "contents",
        "Ljava/lang/Object;");
    KNI_SetObjectField(linkMessageObj, contentsField, contentsObj);

    KNI_EndHandles();
}


static void
getRange(jobject linkMessageObj, int *offset, int *length)
{
    jfieldID offsetField;
    jfieldID lengthField;

    KNI_StartHandles(1);
    KNI_DeclareHandle(linkMessageClass);

    KNI_GetObjectClass(linkMessageObj, linkMessageClass);
    offsetField = KNI_GetFieldID(linkMessageClass, "offset", "I");
    lengthField = KNI_GetFieldID(linkMessageClass, "length", "I");
    *offset = KNI_GetIntField(linkMessageObj, offsetField);
    *length = KNI_GetIntField(linkMessageObj, lengthField);

    KNI_EndHandles();
}


static void
setRange(jobject linkMessageObj, int offset, int length)
{
    jfieldID offsetField;
    jfieldID lengthField;

    KNI_StartHandles(1);
    KNI_DeclareHandle(linkMessageClass);

    KNI_GetObjectClass(linkMessageObj, linkMessageClass);
    offsetField = KNI_GetFieldID(linkMessageClass, "offset", "I");
    lengthField = KNI_GetFieldID(linkMessageClass, "length", "I");
    KNI_SetIntField(linkMessageObj, offsetField, offset);
    KNI_SetIntField(linkMessageObj, lengthField, length);

    KNI_EndHandles();
}


/**
 * Copies the contents of fromMsg to the contents of toMsg. Both must be
 * instances of LinkMessage. The toLink object must be an instance of Link.
 * It's filled in if the contents of fromMsg are a Link.  Returns KNI_TRUE if
 * successful, otherwise KNI_FALSE.
 */
static jboolean
copy(jobject fromMsg, jobject toMsg, jobject toLink) {
    jboolean retval;

    KNI_StartHandles(6);
    KNI_DeclareHandle(byteArrayClass);
    KNI_DeclareHandle(stringClass);
    KNI_DeclareHandle(linkClass);
    KNI_DeclareHandle(fromContents);
    KNI_DeclareHandle(newString);
    KNI_DeclareHandle(newByteArray);

    KNI_FindClass("[B", byteArrayClass);
    KNI_FindClass("java/lang/String", stringClass);
    KNI_FindClass("com/sun/midp/links/Link", linkClass);
    getContents(fromMsg, fromContents);
    
    if (KNI_IsInstanceOf(fromContents, byteArrayClass)) {
        /* do a byte array copy */
        jint fromOffset;
        jint fromLength;

        getRange(fromMsg, &fromOffset, &fromLength);

        SNI_NewArray(SNI_BYTE_ARRAY, fromLength, newByteArray);
        if (KNI_IsNullHandle(newByteArray)) {
            retval = KNI_FALSE;
        } else {
            KNI_GetRawArrayRegion(fromContents, fromOffset, fromLength,
                SNI_GetRawArrayPointer(newByteArray));
            setContents(toMsg, newByteArray);
            setRange(toMsg, 0, fromLength);
            retval = KNI_TRUE;
        }
    } else if (KNI_IsInstanceOf(fromContents, stringClass)) {
        /* do a string copy */
        jchar *buf;
        jsize slen = KNI_GetStringLength(fromContents);

        SNI_NewArray(SNI_BYTE_ARRAY, slen*sizeof(jchar), newByteArray);

        if (KNI_IsNullHandle(newByteArray)) {
            retval = KNI_FALSE;
        } else {
            buf = SNI_GetRawArrayPointer(newByteArray);
            KNI_GetStringRegion(fromContents, 0, slen, buf);
            KNI_NewString(buf, slen, newString);
            setContents(toMsg, newString);
            retval = KNI_TRUE;
        }
    } else if (KNI_IsInstanceOf(fromContents, linkClass)) {
        /* copy the link */
        rendezvous *rp = getNativePointer(fromContents);
        setNativePointer(toLink, rp);
        rp_incref(rp);
        setContents(toMsg, toLink);
        retval = KNI_TRUE;
    } else {
        retval = KNI_FALSE;
    }

    KNI_EndHandles();
    return retval;
}


/**
 * public native void close();
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_links_Link_close(void)
{
    rendezvous *rp;
    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObj);

    KNI_GetThisPointer(thisObj);
    rp = getNativePointer(thisObj);

    /* ignore if closed twice */
    if (rp != NULL) {
        if (rp->sender == JVM_CurrentIsolateID()
                && rp->msg != INVALID_REFERENCE_ID) {
            /* we're the sender, make sure to clean out our message */
            SNI_DeleteReference(rp->msg);
            rp->msg = INVALID_REFERENCE_ID;
        }

        rp->state = CLOSED;
        midp_thread_signal(LINK_READY_SIGNAL, (int)rp, 0);
        setNativePointer(thisObj, NULL);
        rp_decref(rp);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * private native void finalize();
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_links_Link_finalize(void)
{
    Java_com_sun_midp_links_Link_close();
}


/**
 * private native void init0(int sender, int receiver);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_links_Link_init0(void)
{
    int sender;
    int receiver;
    rendezvous *rp;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObj);

    sender = KNI_GetParameterAsInt(1);
    receiver = KNI_GetParameterAsInt(2);
    KNI_GetThisPointer(thisObj);

    rp = rp_create(sender, receiver);
    if (rp == NULL) {
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    } else {
        setNativePointer(thisObj, rp);
        rp_incref(rp);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * public native boolean isOpen();
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_links_Link_isOpen(void)
{
    rendezvous *rp;
    jboolean retval;
    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObj);

    KNI_GetThisPointer(thisObj);
    rp = getNativePointer(thisObj);

    if (rp == NULL) {
        retval = KNI_FALSE;
    } else {
        retval = (rp->state != CLOSED);
    }

    KNI_EndHandles();
    KNI_ReturnBoolean(retval);
}


/**
 * private native void receive0(LinkMessage msg, Link link)
 *         throws ClosedLinkException,
 *                InterruptedIOException,
 *                IOException;
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_links_Link_receive0(void)
{
    rendezvous *rp;

    KNI_StartHandles(4);
    KNI_DeclareHandle(thisObj);
    KNI_DeclareHandle(recvMessageObj);
    KNI_DeclareHandle(sendMessageObj);
    KNI_DeclareHandle(linkObj);

    KNI_GetThisPointer(thisObj);
    KNI_GetParameterAsObject(1, recvMessageObj);
    KNI_GetParameterAsObject(2, linkObj);

    rp = getNativePointer(thisObj);

    if (rp == NULL) {
        if (SNI_GetReentryData(NULL) == NULL) {
            KNI_ThrowNew(midpClosedLinkException, NULL);
        } else {
            KNI_ThrowNew(midpInterruptedIOException, NULL);
        }
    } else if (JVM_CurrentIsolateID() != rp->receiver) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    } else {
        jboolean ok;

        switch (rp->state) {
            case IDLE:
                rp->state = RECEIVING;
                midp_thread_wait(LINK_READY_SIGNAL, (int)rp, NULL);
                break;

            case SENDING:
                getReference(rp->msg, "receive0/SENDING",
                    sendMessageObj);
                ok = copy(sendMessageObj, recvMessageObj, linkObj);
                if (ok) {
                    rp->retcode = OK;
                } else {
                    rp->retcode = ERROR;
                    KNI_ThrowNew(midpIOException, NULL);
                }
                rp->state = DONE;
                midp_thread_signal(LINK_READY_SIGNAL, (int)rp, 0);
                break;

            case RENDEZVOUS:
                getReference(rp->msg, "receive0/RENDEZVOUS", sendMessageObj);
                ok = copy(sendMessageObj, recvMessageObj, linkObj);
                if (ok) {
                    rp->retcode = OK;
                } else {
                    rp->retcode = ERROR;
                    KNI_ThrowNew(midpIOException, NULL);
                }
                rp->state = DONE;
                midp_thread_signal(LINK_READY_SIGNAL, (int)rp, 0);

                break;

            case RECEIVING:
            case DONE:
                midp_thread_wait(LINK_READY_SIGNAL, (int)rp, NULL);
                break;

            case CLOSED:
                setNativePointer(thisObj, NULL);
                rp_decref(rp);
                if (SNI_GetReentryData(NULL) == NULL) {
                    KNI_ThrowNew(midpClosedLinkException, NULL);
                } else {
                    KNI_ThrowNew(midpInterruptedIOException, NULL);
                }
                break;
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * private native void send0(LinkMessage msg)
 *     throws ClosedLinkException,
 *            InterruptedIOException,
 *            IOException;
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_links_Link_send0(void)
{
    rendezvous *rp;
    KNI_StartHandles(3);
    KNI_DeclareHandle(thisObj);
    KNI_DeclareHandle(messageObj);
    KNI_DeclareHandle(otherMessageObj);

    KNI_GetThisPointer(thisObj);
    KNI_GetParameterAsObject(1, messageObj);

    rp = getNativePointer(thisObj);

    if (rp == NULL) {
        if (SNI_GetReentryData(NULL) == NULL) {
            KNI_ThrowNew(midpClosedLinkException, NULL);
        } else {
            KNI_ThrowNew(midpInterruptedIOException, NULL);
        }
    } else if (JVM_CurrentIsolateID() != rp->sender) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    } else {
        switch (rp->state) {
            case IDLE:
                rp->msg = SNI_AddStrongReference(messageObj);
                rp->state = SENDING;
                midp_thread_wait(LINK_READY_SIGNAL, (int)rp, NULL);
                break;

            case RECEIVING:
                rp->msg = SNI_AddStrongReference(messageObj);
                rp->state = RENDEZVOUS;
                midp_thread_signal(LINK_READY_SIGNAL, (int)rp, 0);
                midp_thread_wait(LINK_READY_SIGNAL, (int)rp, NULL);
                break;

            case SENDING:
            case RENDEZVOUS:
                midp_thread_wait(LINK_READY_SIGNAL, (int)rp, NULL);
                break;

            case DONE:
                getReference(rp->msg, "send0/DONE", otherMessageObj);
                if (KNI_IsSameObject(messageObj, otherMessageObj)) {
                    /* it's our message, finish processing */
                    SNI_DeleteReference(rp->msg);
                    rp->msg = INVALID_REFERENCE_ID;
                    rp->state = IDLE;
                    midp_thread_signal(LINK_READY_SIGNAL, (int)rp, 0);
                    if (rp->retcode != OK) {
                        KNI_ThrowNew(midpIOException, NULL);
                    }
                } else {
                    /* somebody else's message, just go back to sleep */
                    midp_thread_wait(LINK_READY_SIGNAL, (int)rp, NULL);
                }

                break;
            case CLOSED:
                setNativePointer(thisObj, NULL);
                if (rp->msg != INVALID_REFERENCE_ID) {
                    /* a message was stranded in the link; clean it out */
                    SNI_DeleteReference(rp->msg);
                    rp->msg = INVALID_REFERENCE_ID;
                }
                rp_decref(rp);
                if (SNI_GetReentryData(NULL) == NULL) {
                    KNI_ThrowNew(midpClosedLinkException, NULL);
                } else {
                    KNI_ThrowNew(midpInterruptedIOException, NULL);
                }
                break;
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}


/**
 * Cleans up this portal entry. Frees the array of pointers to rendezvous 
 * points and sets the count to -1. If the count is already -1, does 
 * nothing. Note that this does not free the portal entry itself, since it's 
 * not separately allocated.
 */
void
portal_free(portal *pp) {
    int i;

    if (pp->count == -1) {
        return;
    }

    for (i = 0; i < pp->count; i++) {
        rp_decref(pp->rppa[i]);
    }

    if (pp->rppa != NULL) {
        pcsl_mem_free(pp->rppa);
        pp->rppa = NULL;
    }

    pp->count = -1;
}


/**
 * private static native void setLinks0(int isolateid, Link[] linkarray);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_links_LinkPortal_setLinks0(void)
{
    int targetIsolate;
    int len;
    int i;
    int ok = 1;
    rendezvous *rp;
    rendezvous **newrppa = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(linkArray);
    KNI_DeclareHandle(linkObj);

    targetIsolate = KNI_GetParameterAsInt(1);
    KNI_GetParameterAsObject(2, linkArray);

    if (KNI_IsNullHandle(linkArray)) {
        len = 0;
    } else {
        len = KNI_GetArrayLength(linkArray);
    }

    for (i = 0; i < len; i++) {
        KNI_GetObjectArrayElement(linkArray, i, linkObj);
        rp = getNativePointer(linkObj);
        if (rp == NULL || rp->state == CLOSED) {
            ok = 0;
            KNI_ThrowNew(midpIllegalArgumentException, NULL);
            break;
        }
    }

    if (ok && portals == NULL) {
        portals =
            (portal *)pcsl_mem_malloc(JVM_MaxIsolates() * sizeof(portal));
        if (portals == NULL) {
            ok = 0;
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
        } else {
            int i;
            for (i = 0; i < JVM_MaxIsolates(); i++) {
                portals[i].count = -1;
                portals[i].rppa = NULL;
            }
        }
    }

    if (ok && len > 0) {
        newrppa = (rendezvous **)pcsl_mem_malloc(len * sizeof(rendezvous *));
        if (newrppa == NULL) {
            KNI_ThrowNew(midpOutOfMemoryError, NULL);
            ok = 0;
        }
    }

    if (ok) {
        portal *pp = &portals[targetIsolate];

        portal_free(pp);

        /* at this point the portal's count is zero and rppa is null */

        if (len > 0) {
            for (i = 0; i < len; i++) {
                KNI_GetObjectArrayElement(linkArray, i, linkObj);
                rp = getNativePointer(linkObj);
                /* rp not null, checked above */
                newrppa[i] = rp;
                rp_incref(rp);
            }
            pp->count = len;
            pp->rppa = newrppa;
        } else if (KNI_IsNullHandle(linkArray)) {
            pp->count = -1;
            pp->rppa = NULL;
        } else {
            /* len == 0 */
            pp->count = 0;
            pp->rppa = NULL;
        }

        midp_thread_signal(LINK_PORTAL_SIGNAL, targetIsolate, 0);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * private static native int getLinkCount0();
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_links_LinkPortal_getLinkCount0(void)
{
    int retval = 0;
    int targetIsolate = JVM_CurrentIsolateID();

    if (portals == NULL || portals[targetIsolate].count == -1) {
        midp_thread_wait(LINK_PORTAL_SIGNAL, targetIsolate, NULL);
    } else {
        retval = portals[targetIsolate].count;
    }

    KNI_ReturnInt(retval);
}


/**
 * private static native void getLinks0(Link[] linkarray);
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_links_LinkPortal_getLinks0(void)
{
    int targetIsolate;
    jsize len;
    int i;
    KNI_StartHandles(2);
    KNI_DeclareHandle(linkArray);
    KNI_DeclareHandle(linkObj);

    targetIsolate = JVM_CurrentIsolateID();
    KNI_GetParameterAsObject(1, linkArray);
    len = KNI_GetArrayLength(linkArray);

    if (portals != NULL) {
        if (portals[targetIsolate].count > 0) {
            rendezvous **rpp = portals[targetIsolate].rppa;
            for (i = 0; i < len; i++) {
                KNI_GetObjectArrayElement(linkArray, i, linkObj);
                setNativePointer(linkObj, rpp[i]);
                rp_incref(rpp[i]);
            }
        }
        portal_free(&portals[targetIsolate]);
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Shutdowns Links subsystem.
 */
void midp_links_shutdown() {
    if (portals != NULL) {
        int i;
        for (i = 0; i < JVM_MaxIsolates(); i++) {
            portal_free(&portals[i]);       
        }

        pcsl_mem_free(portals);        
        portals = NULL;
    }
}


#if ENABLE_I3_TEST

#define LOGSIZE 100

/**
 * A list of rendezvous pointers that have been freed. When the list fills up,
 * appending to it fails silently.  This is for debugging purposes only. Note
 * that every pointer in this list is invalid, since it has already been
 * freed!
 */
static rendezvous *freelog[LOGSIZE];
static int freeptr = 0;

static void log_rp_free(rendezvous *rp) {
    /* fprintf(stderr, ">> freeing 0x%x\n", (unsigned int)rp); */
    if (freeptr < LOGSIZE) {
        freelog[freeptr++] = rp;
    }
}


/**
 * public static native int getRefCount(Link link);
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_com_sun_midp_links_Utils_getRefCount(void)
{
    int retval;
    KNI_StartHandles(1);
    KNI_DeclareHandle(linkObj);
    KNI_GetParameterAsObject(1, linkObj);

    if (KNI_IsNullHandle(linkObj)) {
        retval = 0;
    } else {
        rendezvous *rp = getNativePointer(linkObj);
        if (rp == NULL) {
            retval = 0;
        } else {
            retval = rp->refcount;
        }
    }

    KNI_EndHandles();
    KNI_ReturnInt(retval);
}


/**
 * public static native int[] getFreedRendezvousPoints();
 *
 * Returns an array of freed rendezvous points. Resets the list to empty after
 * it's called.
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
Java_com_sun_midp_links_Utils_getFreedRendezvousPoints(void)
{
    int i;
    KNI_StartHandles(1);
    KNI_DeclareHandle(arrayReturn);

    SNI_NewArray(SNI_INT_ARRAY, freeptr, arrayReturn);

    /*
     * Use SetRawArrayRegion? Maybe assumes too many things about
     * int sizes.
     */
    for (i = 0; i < freeptr; i++) {
        KNI_SetIntArrayElement(arrayReturn, i, (jint)freelog[i]);
    }
    freeptr = 0;

    KNI_EndHandlesAndReturnObject(arrayReturn);
}

/**
 * Forces a GC to occur right now, running finalizers.
 * 
 * public static native void forceGC();
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_links_Utils_forceGC(void)
{
    (void)JVM_GarbageCollect(0, 0);
    KNI_ReturnVoid();
}


#endif /* ENABLE_I3_TEST */
