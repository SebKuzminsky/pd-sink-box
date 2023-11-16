#include "pico/time.h"

#include "hmi.h"
#include "quadrature_encoder.pio.h"
#include "button.pio.h"


static hmi_window_t * hmi_windows;
static int hmi_active_window;


void hmi_init(hmi_window_t * windows) {
    hmi_windows = windows;

    uint const encoder_gpio_a = 0;
    pio_add_program_at_offset(pio1, &quadrature_encoder_program, 0);
    quadrature_encoder_program_init(pio1, encoder_gpio_a, 0);

    uint const button_gpio = 2;
    pio_add_program_at_offset(pio0, &button_program, 0);
    button_init(pio0, 0, button_gpio);

    for (int i = 0; hmi_windows[i].id != -1; ++i) {
        if (hmi_windows[i].init != nullptr) {
            hmi_windows[i].context = hmi_windows[i].init();
        }
    }

    hmi_set_active_window(0);
}


void hmi_set_active_window(int id) {
    hmi_active_window = id;
    if (hmi_windows[id].selected != nullptr) {
        hmi_windows[id].selected(hmi_windows[id].context);
    }
}


void hmi_run(void) {
    bool need_redraw = true;
    absolute_time_t next_redraw = at_the_end_of_time;

    int new_count, delta;
    int old_count = quadrature_encoder_get_count();

    while (true) {
        if (!is_at_the_end_of_time(next_redraw)) {
            if (absolute_time_diff_us(get_absolute_time(), next_redraw) < 0) {
                need_redraw = true;
            }
        }
        if (need_redraw) {
            uint32_t ms_until_redraw;
            need_redraw = false;
            ms_until_redraw = hmi_windows[hmi_active_window].draw(hmi_windows[hmi_active_window].context);
            if (ms_until_redraw == 0) {
                next_redraw = at_the_end_of_time;
            } else {
                next_redraw = make_timeout_time_ms(ms_until_redraw);
            }
        }

        new_count = quadrature_encoder_get_count();
        delta = new_count - old_count;
        if (delta >= 4) {
            if (hmi_windows[hmi_active_window].event_ccw != nullptr) {
                hmi_windows[hmi_active_window].event_ccw(hmi_windows[hmi_active_window].context);
                need_redraw = true;
            }
            old_count = new_count;
        } else if (delta <= -4) {
            if (hmi_windows[hmi_active_window].event_cw != nullptr) {
                hmi_windows[hmi_active_window].event_cw(hmi_windows[hmi_active_window].context);
                need_redraw = true;
            }
            old_count = new_count;
        }

        uint32_t button_state;
        if (button_get_state(button_state)) {
            if (button_state == 0) {
                if (hmi_windows[hmi_active_window].event_click != nullptr) {
                    hmi_windows[hmi_active_window].event_click(hmi_windows[hmi_active_window].context);
                    need_redraw = true;
                }
            }
        }
    }
}
