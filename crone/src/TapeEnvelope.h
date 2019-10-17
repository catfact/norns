#ifndef _TAPE_ENVELOPE_H_
#define _TAPE_ENVELOPE_H_

#include "Window.h"

namespace crone {
    struct TapeEnvelope {
        typedef enum {
            Opening, Closing, Open, Closed
        } State;
        State state;
        int idx;

        void open() {
            state = Opening;
        }

        void close() {
            state = Closing;
        }

        bool increment() {
            idx++;
            if (idx >= static_cast<int>(Window::raisedCosShortLen)) {
                idx = Window::raisedCosShortLen - 1;
                state = Open;
                return true;
            }
            return false;
        }

        bool decrement() {
            idx--;
            if (idx < 0) {
                idx = 0;
                state = Closed;
                return true;
            }
            return false;
        }

        float value() {
            switch (state) {
                case Closed:
                    return 0.f;
                case Open:
                    return 1.f;
                case Opening:
                case Closing:
                    return Window::raisedCosShort(idx);
                default:
                    return 0.f;
            }
        }
    };

}

#endif
