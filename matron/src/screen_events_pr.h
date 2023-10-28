// screen events: private data types
// (separate from screen_events.c only for readability)

#ifndef _SCREEN_EVENTS_PR_H_
#define _SCREEN_EVENTS_PR_H_

typedef enum {
	      SCREEN_EVENT_NONE = 0, // unused / init
	      SCREEN_EVENT_UPDATE = 1,
	      SCREEN_EVENT_SAVE = 2,
	      SCREEN_EVENT_RESTORE = 3,
	      SCREEN_EVENT_FONT_FACE = 4,
	      SCREEN_EVENT_FONT_SIZE = 5,
	      SCREEN_EVENT_AA = 6,
	      SCREEN_EVENT_LEVEL = 7,
	      SCREEN_EVENT_LINE_WIDTH = 8,
	      SCREEN_EVENT_LINE_CAP = 9,
	      SCREEN_EVENT_LINE_JOIN = 10,
	      SCREEN_EVENT_MITER_LIMIT = 11,
	      SCREEN_EVENT_MOVE = 12,
	      SCREEN_EVENT_LINE = 13,
	      SCREEN_EVENT_MOVE_REL = 14,
	      SCREEN_EVENT_LINE_REL = 15,
	      SCREEN_EVENT_CURVE = 16,
	      SCREEN_EVENT_CURVE_REL = 17,
	      SCREEN_EVENT_ARC = 18,
	      SCREEN_EVENT_RECT = 19,
	      SCREEN_EVENT_STROKE = 20,
	      SCREEN_EVENT_FILL = 21,
	      SCREEN_EVENT_TEXT = 22,
	      SCREEN_EVENT_TEXT_RIGHT = 23,
	      SCREEN_EVENT_TEXT_CENTER = 24,
	      SCREEN_EVENT_TEXT_EXTENTS = 25,
	      SCREEN_EVENT_TEXT_TRIM = 26,
	      SCREEN_EVENT_CLEAR = 27,
	      SCREEN_EVENT_CLOSE_PATH = 28,
	      SCREEN_EVENT_EXPORT_PNG = 29,
	      SCREEN_EVENT_DISPLAY_PNG = 30,
	      SCREEN_EVENT_ROTATE = 31,
	      SCREEN_EVENT_TRANSLATE = 32,
	      SCREEN_EVENT_SET_OPERATOR = 33,
	      SCREEN_EVENT_PEEK = 34,
	      SCREEN_EVENT_POKE = 35,
	      SCREEN_EVENT_CURRENT_POINT = 36,
		  SCREEN_EVENT_HARDWARE_REFRESH = 37,
	      
} screen_event_id_t;

//----------------
struct se_int {
    int i1;
};

struct se_doubles {
    double d1;
    double d2;
    double d3;
    double d4;
    double d5;
    double d6;
};

// string and binary types use the same buffer
struct se_buf {
    size_t nb;
};
    
struct se_buf_doubles {
    size_t nb;
    double d1;
    double d2;
    double d3;
    double d4;
};

struct se_buf_ints {
    size_t nb;
    int i1;
    int i2;
    int i3;
    int i4;
};
union screen_event_payload {
    struct se_int i;
    struct se_doubles d;
    struct se_buf b;
    struct se_buf_doubles bd;
    struct se_buf_ints bi;
};

struct screen_event_data {
    int type;
    void *buf;
    union screen_event_payload payload;
};

#endif
