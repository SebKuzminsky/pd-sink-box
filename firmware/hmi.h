#ifndef __HMI_H__
#define __HMI_H__

//
// This is a simple "human/machine interface" framework.  It does not
// manage a display/screen for output (that's up to the "windows"),
// but it does manage a rotary encoder/button for input from the human..
//
// There is a list of "windows".  Each window has a draw() function,
// and handlers for the clockwise/counter-clockwise/click events.
//

typedef struct {
    int id;
    void * context;

    // `init()` gets called at startup (from `hmi_init()`).  It may
    // return a window-specific context structure, which will get passed
    // to every future function call of this window.
    void * (*init)(void);

    // `draw()` draws the window.  Returns the number of milliseconds
    // until it wants to get called again to redraw the screen, or 0 for
    // "wait until there's a user interaction".
    uint32_t (*draw)(void * context);

    // `selected()` is called when the window becomes active (i.e. when
    // someone calls `hmi_set_active_window()` on it), before the first
    // call to `draw()`.
    void (*selected)(void * context);

    // Called when the user rotates the encoder knob one click clockwise,
    // while this window is active.
    void (*event_cw)(void * context);

    // Called when the user rotates the encoder knob one click
    // counter-clockwise, while this window is active.
    void (*event_ccw)(void * context);

    // Called when the user clicks the encoder knob, while this window
    // is active.
    void (*event_click)(void * context);
} hmi_window_t;


void hmi_init(hmi_window_t * windows);
void hmi_set_active_window(int id);
void hmi_run(void);


#endif // __HMI_H__
