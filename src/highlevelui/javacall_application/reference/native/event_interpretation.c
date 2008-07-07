/**
 */
#include <javacall_events.h>
#include <midp_jc_event_defs.h>
#include <event_interpretation.h>


static MidpEvent newMidpEvent;

jlong backup_time;
jlong current_time;
jlong click_timeout = 1000;

static jbyte evt_key = 0;

jboolean get_complicated_event(MidpEvent* pNewComplMidpEvent,
                                  int action, int x, int y) {

    jboolean res = KNI_FALSE;

    printf("get_complicated_event \n");

    switch (action) {
        case PENPRESSED:
            backup_time = JVM_JavaMilliSeconds();
            printf("case PENPRESSED backup_time = % d \n", backup_time);
            evt_key = EVT_DOWN_MASK;
            break;
        case PENRELEASED:
            current_time = JVM_JavaMilliSeconds();
            printf("case PENRELEASED current_time = % d \n", current_time);
            printf("case PENRELEASED evt_key = % d \n", evt_key);
            printf("case PENRELEASED evt_key & EVT_DOWN_MASK & EVT_MOVE_MASK = % d \n", evt_key & EVT_DOWN_MASK & EVT_MOVE_MASK);
            printf("case PENRELEASED evt_key & EVT_DOWNHOLD_MASK = % d \n", evt_key & EVT_DOWNHOLD_MASK);
            if ((evt_key & EVT_DOWN_MASK) && (evt_key & EVT_MOVE_MASK) && !(evt_key & EVT_DOWNHOLD_MASK)) {
                printf("get_complicated_event FLICKER \n");
                pNewComplMidpEvent->type    = MIDP_PEN_EVENT;
                pNewComplMidpEvent->ACTION  = FLICK;
                pNewComplMidpEvent->X_POS   = x;
                pNewComplMidpEvent->Y_POS   = y;
                res = KNI_TRUE;
            }
            if ((evt_key & EVT_DOWN_MASK) && (evt_key & EVT_DOWNHOLD_MASK) && (evt_key & EVT_MOVE_MASK)) {
                printf("get_complicated_event CLICK \n");
                pNewComplMidpEvent->type    = MIDP_PEN_EVENT;
                pNewComplMidpEvent->ACTION  = CLICK;
                pNewComplMidpEvent->X_POS   = x;
                pNewComplMidpEvent->Y_POS   = y;
                res = KNI_TRUE;
            }
            break;
        case PENDRAGGED:
            current_time = JVM_JavaMilliSeconds();
            if ((evt_key&EVT_DOWN_MASK) && (current_time - backup_time > click_timeout)) {
                evt_key = EVT_DOWNHOLD_MASK;
            }
            evt_key = evt_key | EVT_MOVE_MASK;
            backup_time = JVM_JavaMilliSeconds();
            break;
        default :
            ;// empty ??
    }
    
    return res;
                                      
}