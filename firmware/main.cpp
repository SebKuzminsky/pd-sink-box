#include <cstdio>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <hardware/pwm.h>
#include <hardware/flash.h>
#include <hardware/sync.h>

#include <pico/stdlib.h>

#include <hagl_hal.h>
#include <hagl.h>
#include <font6x9.h>

#include <mipi_display.h>
#include <mipi_dcs.h>

#include "hagl_char_scaled.h"

#include "hmi.h"
#include "husb238.h"
#include "version-info.h"

#ifdef RASPBERRYPI_PICO_W
#include "pico/cyw43_arch.h"
#endif


//
// We store some config variables in flash and load them back in at
// boot time.
//
// The flash is memory-mapped into the RP2040 address space at XIP_BASE.
// We store our stuff at offset 256kB in the flash.
//

#define FLASH_OFFSET ((1024 + 512) * 1024)

// This is the data structure we store in flash.
static struct {
    uint8_t cookie;
    int backlight_duty_cycle;
    int screen_rotation_index;
    uint8_t checksum;
} flash_data;

static void write_flash(void) {
    // Cheesiest checksum ever.
    uint8_t checksum = 0;
    flash_data.cookie = 0x55;
    flash_data.checksum = 0;
    for (uint i = 0; i < sizeof(flash_data); ++i) {
        checksum ^= ((uint8_t *)(&flash_data))[i];
    }
    flash_data.checksum = checksum;

    uint32_t ints = save_and_disable_interrupts();

    // Erase one sector of the flash.
    flash_range_erase(FLASH_OFFSET, FLASH_SECTOR_SIZE);

    // Write our data to the flash.
    flash_range_program(FLASH_OFFSET, (uint8_t const *)(&flash_data), FLASH_PAGE_SIZE);

    restore_interrupts(ints);
}

static bool read_flash(void) {
    uint8_t const * flash_mem_ptr = (uint8_t const *)(XIP_BASE + FLASH_OFFSET);

    memcpy(&flash_data, flash_mem_ptr, sizeof(flash_data));

    uint8_t checksum = 0;
    for (uint i = 0; i < sizeof(flash_data); ++i) {
        checksum ^= ((uint8_t *)(&flash_data))[i];
    }

    if ((flash_data.cookie == 0x55) && (checksum == 0)) {
        // Valid flash.
        return true;
    }

    // Invalid flash, caller will initialize flash_data to sane defaults.
    return false;
}


static i2c_inst_t * i2c;

static hagl_backend_t *display;

static uint16_t display_width = MIPI_DISPLAY_WIDTH;
static uint16_t display_height = MIPI_DISPLAY_HEIGHT;

static int backlight_duty_cycle_max = 10*1000; // Highest possible value for duty cycle.
static int backlight_duty_cycle = 10*1000;     // Active value for duty cycle.
static int backlight_duty_cycle_delta = 1000;  // Duty cycle changes by this much for each knob detent.
static uint16_t backlight_pwm_slice;


typedef enum {
    WINDOW_MAIN,
    WINDOW_MENU,
    WINDOW_ROTATE,
    WINDOW_BACKLIGHT,
    WINDOW_INFO
} hmi_window_id_t;


//
// Main window
//

void window_main_draw(void * void_context) {
    int r;

    int volts;
    float max_current;

    wchar_t str[40];
    int16_t x, y;

    uint8_t const * font = font6x9;
    int w=6, h=9;
    int scale=4;

    hagl_color_t text_color;

    hagl_clear(display);

    if (!husb238_connected(i2c)) {
        text_color = hagl_color(display, 255, 0, 0);

        r = swprintf(str, sizeof(str), L"No");
        x = (display->width - (r * w * scale))/2;
        y = (display->height / 2) - (1.5 * h * scale);
        hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

        r = swprintf(str, sizeof(str), L"input");
        x = (display->width - (r * w * scale))/2;
        y = (display->height / 2) - (0.5 * h * scale);
        hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

        r = swprintf(str, sizeof(str), L"power");
        x = (display->width - (r * w * scale))/2;
        y = (display->height / 2) + (0.5 * h * scale);
        hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

        hagl_flush(display);
        return;
    }

    r = husb238_get_contract(i2c, volts, max_current);
    if (r != PICO_OK) {
        printf("error reading PD contract from HUSB238\n");
        volts = -1;
        max_current = -1.0;
    }
    printf("PD contract: %dV %4.2fA\n", volts, max_current);

    if (volts > 0) {
        // Got a PD contract, happy green text.
        text_color = hagl_color(display, 0, 255, 0);
    } else {
        // No PD contract established, sad grayish text.
        text_color = hagl_color(display, 150, 150, 150);
    }

    r = swprintf(str, sizeof(str), L"%dV", volts);
    x = (display->width - (r * w * scale))/2;
    y = (display->height / 2) - h * scale;
    hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

    r = swprintf(str, sizeof(str), L"%04.2fA", max_current);
    x = (display->width - (r * w * scale))/2;
    y = (display->height / 2) + 2;
    hagl_put_text_scaled(display, str, x, y, text_color, scale, font6x9);

    hagl_flush(display);
}

void window_main_any_interaction(void * void_context) {
    hmi_set_active_window(WINDOW_MENU);
}


//
// Main Menu window
//

typedef struct {
    wchar_t text[16];
    bool enabled;  // menu items that are !enabled are shown but not selectable
} menu_item_t;

typedef struct {
    menu_item_t * items;
    int selected_item;
    int num_items;
} menu_t;


typedef struct {
    husb238_pdo_t pdos[6];
    int current_pdo;       // this is the SRC_PDO identifier
    menu_t menu;
} window_menu_context_t;


static void * window_menu_init(void) {
    window_menu_context_t * context = (window_menu_context_t*)calloc(1, sizeof(window_menu_context_t));
    if (context == nullptr) {
        printf("out of memory\n");
        return nullptr;
    }

    context->menu.num_items = 10;  // 6 PDOs, Rotate, Backlight, Info, Back
    context->menu.items = (menu_item_t*)calloc(context->menu.num_items, sizeof(menu_item_t));
    if (context->menu.items == nullptr) {
        printf("out of memory\n");
        return nullptr;
    }

    // The first 6 Main Menu items are the PDOs.  Because you can run
    // this device without USB-PD connected (by powering the Pico via
    // its own USB Micro-B connector) we re-read the PDOs each time the
    // user selects the Main Menu, and we re-sprintf the menu items
    // based on the max currents detected at that time (and set the
    // enabled/disabled state of each PDO).

    // The 7th Main Menu item is the Rotate screen.
    swprintf(
        context->menu.items[6].text,
        sizeof(context->menu.items[6].text),
        L"Rotate"
    );
    context->menu.items[6].enabled = true;

    // The 8th Main Menu item is the Backlight screen.
    swprintf(
        context->menu.items[7].text,
        sizeof(context->menu.items[7].text),
        L"Backlight"
    );
    context->menu.items[7].enabled = true;

    // The 9th Main Menu item is the Info screen.
    swprintf(
        context->menu.items[8].text,
        sizeof(context->menu.items[8].text),
        L"Info"
    );
    context->menu.items[8].enabled = true;

    // The 10th and final Main Menu item is Back, to go back to the main window.
    swprintf(
        context->menu.items[9].text,
        sizeof(context->menu.items[9].text),
        L"Back"
    );
    context->menu.items[9].enabled = true;

    return context;
}


static void window_menu_draw(void * void_context) {
    window_menu_context_t * context = (window_menu_context_t*)void_context;

    int x_pos = 5;
    int y_pos = 5;

    hagl_clear(display);

    context->current_pdo = husb238_get_current_pdo(i2c);

    for (int i = 0; i < context->menu.num_items; ++i) {
        hagl_color_t white = hagl_color(display, 255, 255, 255);
        hagl_color_t gray = hagl_color(display, 150, 150, 150);
        hagl_color_t green = hagl_color(display, 0, 255, 0);

        hagl_color_t text_color;

        uint8_t const * font = font6x9;
        int w=6, h=9;
        int scale=2;

        if (context->menu.items[i].enabled) {
            text_color = white;
        } else {
            text_color = gray;
        }
        if ((i < 6) && (context->pdos[i].id == context->current_pdo)) {
            text_color = green;
        }

        hagl_put_text_scaled(display, context->menu.items[i].text, x_pos, y_pos, text_color, scale, font);

        if (i == context->menu.selected_item) {
            hagl_color_t red = hagl_color(display, 255, 0, 0);
            wchar_t cursor[] = L"<<<";
            size_t len = wcslen(context->menu.items[i].text);
            hagl_put_text_scaled(display, cursor, x_pos+(len*w*scale), y_pos, red, scale, font);
        }

        y_pos += h * scale;
    }

    hagl_flush(display);
}


// The Main Menu window was selected, re-read PDOs and regenerate the
// menu items enabled/disabled state.
void window_menu_selected(void * void_context) {
    window_menu_context_t * context = (window_menu_context_t *)void_context;
    int r;

    r = husb238_get_pdos(i2c, context->pdos);
    if (r != PICO_OK) {
        printf("error reading PDOs\n");
    }

    // Update the first 6 menu items based on the PDOs offered by this
    // USB-PD Source.  All PDOs are displayed, but the ones not available
    // are disabled (grayed out and not selectable).
    for (int i = 0; i < 6; ++i) {
        r = swprintf(
            context->menu.items[i].text,
            sizeof(context->menu.items[i].text),
            L"%dV/%dA",
            (int)context->pdos[i].volts,
            (int)context->pdos[i].max_current
        );
        if (context->pdos[i].max_current > 0) {
            context->menu.items[i].enabled = true;
        } else {
            context->menu.items[i].enabled = false;
        }
    }

    // By default we select the lowest-voltage PDO.  If there are no
    // PDOs available, select the first non-PDO menu item.
    for (int i = 0; i < context->menu.num_items; ++i) {
        if (context->menu.items[i].enabled) {
            context->menu.selected_item = i;
            break;
        }
    }
}


void window_menu_cw(void * void_context) {
    window_menu_context_t * context = (window_menu_context_t*)void_context;

    context->menu.selected_item = (context->menu.selected_item + 1) % context->menu.num_items;
    while (! context->menu.items[context->menu.selected_item].enabled) {
        context->menu.selected_item = (context->menu.selected_item + 1) % context->menu.num_items;
    }
}


void window_menu_ccw(void * void_context) {
    window_menu_context_t * context = (window_menu_context_t*)void_context;

    context->menu.selected_item -= 1;
    if (context->menu.selected_item == -1) {
        context->menu.selected_item = context->menu.num_items-1;
    }
    while (! context->menu.items[context->menu.selected_item].enabled) {
        context->menu.selected_item -= 1;
        if (context->menu.selected_item == -1) {
            context->menu.selected_item = context->menu.num_items-1;
        }
    }
}

void window_menu_click(void * void_context) {
    window_menu_context_t * context = (window_menu_context_t*)void_context;

    if (context->menu.selected_item == 6) {
        hmi_set_active_window(WINDOW_ROTATE);
        return;
    } else if (context->menu.selected_item == 7) {
        hmi_set_active_window(WINDOW_BACKLIGHT);
        return;
    } else if (context->menu.selected_item == 8) {
        hmi_set_active_window(WINDOW_INFO);
        return;
    } else if (context->menu.selected_item == 9) {
        hmi_set_active_window(WINDOW_MAIN);
        return;
    }

    husb238_select_pdo(i2c, context->pdos[context->menu.selected_item].id);
    husb238_dump_registers(i2c);
    context->current_pdo = husb238_get_current_pdo(i2c);
    hmi_set_active_window(WINDOW_MAIN);
}


//
// Rotate window
//

typedef struct {
    struct {
        uint8_t dcs_address_mode;
        uint16_t width, height;
        int16_t x_offset, y_offset;
    } rotation_info[4];
    int rotation_index;
} window_rotate_context_t;

void set_screen_rotation(window_rotate_context_t * c) {
    uint8_t mode = c->rotation_info[c->rotation_index].dcs_address_mode;

    hagl_clear(display);
    hagl_flush(display);

    mipi_display_ioctl(MIPI_DCS_SET_ADDRESS_MODE, &mode, 1);

    hagl_set_resolution(
        display,
        c->rotation_info[c->rotation_index].width,
        c->rotation_info[c->rotation_index].height
    );

    mipi_display_set_xy_offset(
        c->rotation_info[c->rotation_index].x_offset,
        c->rotation_info[c->rotation_index].y_offset
    );

    display_width = c->rotation_info[c->rotation_index].width;
    display_height = c->rotation_info[c->rotation_index].height;
}

void * window_rotate_init(void) {
    window_rotate_context_t * c = (window_rotate_context_t *)calloc(1, sizeof(window_rotate_context_t));
    if (c == nullptr) {
        printf("out of memory\n");
        return nullptr;
    }

    // available constants
    // MIPI_DCS_ADDRESS_MODE_MIRROR_Y      0x80
    // MIPI_DCS_ADDRESS_MODE_MIRROR_X      0x40
    // MIPI_DCS_ADDRESS_MODE_SWAP_XY       0x20
    // MIPI_DCS_ADDRESS_MODE_BGR           0x08
    // MIPI_DCS_ADDRESS_MODE_RGB           0x00
    // MIPI_DCS_ADDRESS_MODE_FLIP_X        0x02
    // MIPI_DCS_ADDRESS_MODE_FLIP_Y        0x01

    // 0°, the native orientation of the screen
    c->rotation_info[0] = {
        .dcs_address_mode = 0x00,
        .width = MIPI_DISPLAY_WIDTH,
        .height = MIPI_DISPLAY_HEIGHT,
        .x_offset = MIPI_DISPLAY_OFFSET_X,
        .y_offset = MIPI_DISPLAY_OFFSET_Y,
    };

    // 90°
    c->rotation_info[1] = {
        .dcs_address_mode = MIPI_DCS_ADDRESS_MODE_SWAP_XY | MIPI_DCS_ADDRESS_MODE_MIRROR_X,
        .width = MIPI_DISPLAY_HEIGHT,
        .height = MIPI_DISPLAY_WIDTH,
        .x_offset = MIPI_DISPLAY_OFFSET_Y,
        .y_offset = MIPI_DISPLAY_OFFSET_X,
    };

    // 180°
    c->rotation_info[2] = {
        .dcs_address_mode = MIPI_DCS_ADDRESS_MODE_MIRROR_X | MIPI_DCS_ADDRESS_MODE_MIRROR_Y,
        .width = MIPI_DISPLAY_WIDTH,
        .height = MIPI_DISPLAY_HEIGHT,
        .x_offset = MIPI_DISPLAY_OFFSET_X,
        .y_offset = MIPI_DISPLAY_OFFSET_Y,
    };

    // 270°
    c->rotation_info[3] = {
        .dcs_address_mode = MIPI_DCS_ADDRESS_MODE_SWAP_XY | MIPI_DCS_ADDRESS_MODE_MIRROR_Y,
        .width = MIPI_DISPLAY_HEIGHT,
        .height = MIPI_DISPLAY_WIDTH,
        .x_offset = MIPI_DISPLAY_OFFSET_Y,
        .y_offset = MIPI_DISPLAY_OFFSET_X,
    };

    c->rotation_index = flash_data.screen_rotation_index;
    if (c->rotation_index < 0) {
        c->rotation_index = 0;
    } else if (c->rotation_index > 3) {
        c->rotation_index = 3;
    }

    set_screen_rotation(c);

    return c;
}

void window_rotate_draw(void * void_context) {
    hagl_color_t white = hagl_color(display, 255, 255, 255);
    hagl_color_t red = hagl_color(display, 255, 0, 0);

    hagl_clear(display);

    hagl_draw_rectangle_xyxy(display, 0, 0, display_width-1, display_height-1, white);
    hagl_draw_rectangle_xyxy(display, 1, 1, display_width-2, display_height-2, white);
    hagl_draw_rectangle_xyxy(display, 2, 2, display_width-3, display_height-3, white);

    int r;
    wchar_t str[40];
    int16_t x, y;

    uint8_t const * font = font6x9;
    int w=6;
    int scale=4;

    r = swprintf(str, sizeof(str), L"Top");
    x = (display_width - (r * w * scale))/2;
    y = 5;
    hagl_put_text_scaled(display, str, x, y, red, scale, font);

    hagl_flush(display);
}

void window_rotate_cw(void * void_context) {
    window_rotate_context_t * c = (window_rotate_context_t *)void_context;
    c->rotation_index = (c->rotation_index + 1) % 4;
    set_screen_rotation(c);
}

void window_rotate_ccw(void * void_context) {
    window_rotate_context_t * c = (window_rotate_context_t *)void_context;
    c->rotation_index = c->rotation_index - 1;
    if (c->rotation_index == -1) c->rotation_index = 3;
    set_screen_rotation(c);
}

void window_rotate_click(void * void_context) {
    window_rotate_context_t * c = (window_rotate_context_t *)void_context;
    flash_data.screen_rotation_index = c->rotation_index;
    write_flash();  // remember which screen orientation the user likes
    hmi_set_active_window(WINDOW_MAIN);
}


//
// Backlight window
//

void window_backlight_draw(void * void_context) {
    int r;

    wchar_t str[40];
    int16_t x, y;

    uint8_t const * font = font6x9;
    int w=6, h=9;
    int scale=2;

    hagl_color_t text_color = hagl_color(display, 255, 255, 255);

    hagl_clear(display);

    r = swprintf(str, sizeof(str), L"Backlight");
    x = (display->width - (r * w * scale))/2;
    y = (display->height / 2) - (1 * h * scale);
    hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

    r = swprintf(str, sizeof(str), L"%d%%", (100 * backlight_duty_cycle)/backlight_duty_cycle_max);
    x = (display->width - (r * w * scale))/2;
    y = (display->height / 2) + (1 * h * scale);
    hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

    hagl_flush(display);
}


void window_backlight_cw(void * void_context) {
    backlight_duty_cycle += backlight_duty_cycle_delta;
    if (backlight_duty_cycle > backlight_duty_cycle_max) {
        backlight_duty_cycle = backlight_duty_cycle_max;
    }
    pwm_set_chan_level(backlight_pwm_slice, PWM_CHAN_B, backlight_duty_cycle);
}


void window_backlight_ccw(void * void_context) {
    backlight_duty_cycle -= backlight_duty_cycle_delta;
    if (backlight_duty_cycle < 0) {
        backlight_duty_cycle = 0;
    }
    pwm_set_chan_level(backlight_pwm_slice, PWM_CHAN_B, backlight_duty_cycle);
}


void window_backlight_click(void * void_context) {
    flash_data.backlight_duty_cycle = backlight_duty_cycle;
    write_flash();  // remember which backlight brightness the user likes
    hmi_set_active_window(WINDOW_MAIN);
}


//
// Info window
//

void window_info_draw(void * void_context) {
    int r;

    wchar_t str[40];
    int16_t x, y;

    uint8_t const * font = font6x9;
    int w=6, h=9;
    int scale=2;

    hagl_color_t text_color = hagl_color(display, 255, 255, 255);

    hagl_clear(display);

    r = swprintf(str, sizeof(str), L"Build");
    x = (display->width - (r * w * scale))/2;
    y = (display->height / 2) - (2 * h * scale);
    hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

    r = swprintf(str, sizeof(str), L"version");
    x = (display->width - (r * w * scale))/2;
    y = (display->height / 2) - (1 * h * scale);
    hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

    // `version` is from version-info.c, generated at build time.
    r = swprintf(str, sizeof(str), L"%s", version_info_commit);
    x = (display->width - (r * w * scale))/2;
    y = (display->height / 2) + (0 * h * scale);
    hagl_put_text_scaled(display, str, x, y, text_color, scale, font);

    // `dirty` is from version-info.c, generated at build time.
    if (strlen(version_info_dirty) > 0) {
        r = swprintf(str, sizeof(str), L"%s", version_info_dirty);
        x = (display->width - (r * w * scale))/2;
        y = (display->height / 2) + (1 * h * scale);
        hagl_put_text_scaled(display, str, x, y, text_color, scale, font);
    }

    hagl_flush(display);
}

void window_info_any_interaction(void * void_context) {
    hmi_set_active_window(WINDOW_MAIN);
}


static hmi_window_t windows[] = {
    {
        .id = WINDOW_MAIN,
        .init = nullptr,
        .draw = &window_main_draw,
        .selected = nullptr,
        .event_cw = &window_main_any_interaction,
        .event_ccw = &window_main_any_interaction,
        .event_click = &window_main_any_interaction
    },

    {
        .id = WINDOW_MENU,
        .init = &window_menu_init,
        .draw = &window_menu_draw,
        .selected = &window_menu_selected,
        .event_cw = &window_menu_cw,
        .event_ccw = &window_menu_ccw,
        .event_click = &window_menu_click
    },

    {
        .id = WINDOW_ROTATE,
        .init = &window_rotate_init,
        .draw = &window_rotate_draw,
        .selected = nullptr,
        .event_cw = &window_rotate_cw,
        .event_ccw = &window_rotate_ccw,
        .event_click = &window_rotate_click
    },

    {
        .id = WINDOW_BACKLIGHT,
        .init = nullptr,
        .draw = &window_backlight_draw,
        .selected = nullptr,
        .event_cw = &window_backlight_cw,
        .event_ccw = &window_backlight_ccw,
        .event_click = &window_backlight_click
    },

    {
        .id = WINDOW_INFO,
        .init = nullptr,
        .draw = &window_info_draw,
        .selected = nullptr,
        .event_cw = &window_info_any_interaction,
        .event_ccw = &window_info_any_interaction,
        .event_click = &window_info_any_interaction
    },

    {
        .id = -1  // secret handshake that means "end of window list"
    }
};


int main() {
    stdio_init_all();
    // sleep_ms(3000);

    if (!read_flash()) {
        // Invalid flash, initialize to sane defaults.
        flash_data.backlight_duty_cycle = backlight_duty_cycle_max;
        flash_data.screen_rotation_index = 0;
    }

    backlight_duty_cycle = flash_data.backlight_duty_cycle;
    if (backlight_duty_cycle > backlight_duty_cycle_max) {
        backlight_duty_cycle = backlight_duty_cycle_max;
    } else if (backlight_duty_cycle < 0) {
        backlight_duty_cycle = 0;
    }

    display = hagl_init();

    // PWM control of backlight.
    gpio_set_function(MIPI_DISPLAY_PIN_BL, GPIO_FUNC_PWM);
    backlight_pwm_slice = pwm_gpio_to_slice_num(MIPI_DISPLAY_PIN_BL);
    pwm_set_wrap(backlight_pwm_slice, backlight_duty_cycle_max);
    // PWM channel A is not used.
    pwm_set_chan_level(backlight_pwm_slice, PWM_CHAN_B, backlight_duty_cycle);
    pwm_set_enabled(backlight_pwm_slice, true);

#if defined RASPBERRYPI_PICO_W
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return 1;
    }
    printf("cyw43_arch_init\n");
#endif

    // printf("\x1b[2J");  // VT100: clear screen
    printf("booted\n");

    //
    // Initialize i2c.
    //

    const uint sda_gpio = 16;  // pin 21
    const uint scl_gpio = 17;  // pin 22

    i2c = i2c0;
    i2c_init(i2c, 100*1000);  // run i2c at 100 kHz

    // Initialize I2C pins
    gpio_set_function(sda_gpio, GPIO_FUNC_I2C);
    gpio_set_function(scl_gpio, GPIO_FUNC_I2C);

    gpio_pull_up(sda_gpio);
    gpio_pull_up(scl_gpio);


    //
    // Initialize the HMI.
    //

    hmi_init(windows);


    //
    // And go!
    //

    hmi_run();
}
