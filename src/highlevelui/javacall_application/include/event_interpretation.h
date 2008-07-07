#ifndef _EVENT_INTERPRETATION_H_
#define _EVENT_INTERPRETATION_H_

#include <kni.h>
#include <jvm.h>
#include <midpEvents.h>

#define EVT_DOWN_MASK           0x1  //0x0001 : Touch Down
#define EVT_DOWNHOLD_MASK  0x2  //0x0010 :  Touch Down and Hold
#define EVT_MOVE_MASK            0x4  //0x0100 : Move

typedef enum {
    PENPRESSED = 1,
    PENRELEASED = 2,
    PENDRAGGED = 3,
    CLICK = 4,
    FLICK = 5
} penevent;

extern jboolean get_complicated_event(MidpEvent*,
                                  int, int, int);

#endif /* _EVENT_INTERPRETATION_H_ */