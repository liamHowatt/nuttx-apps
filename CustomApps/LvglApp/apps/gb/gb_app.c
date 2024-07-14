#include "../../apps.h"

#include "lvgl/lvgl.h"
#include "Peanut-GB/peanut_gb.h"
#include <stdio.h>
#include "red.h"
#include <assert.h>
#include <math.h>

#define COLOR_R(h) ((h >> 16) & 0xff)
#define COLOR_G(h) ((h >> 8) & 0xff)
#define COLOR_B(h) (h & 0xff)
#define COLOR_HEX(h) {COLOR_B(h), COLOR_G(h), COLOR_R(h), 0xff}
#define COLOR_HEX_16(h) (((COLOR_R(h) & 0b11111000) << 8) | ((COLOR_G(h) & 0b11111100) << 3) | (COLOR_B(h) >> 3))

typedef struct {
    struct gb_s gb;
    lv_obj_t * canv;
    uint8_t tmp_buf[160 * 144];
    uint16_t * draw_buf_pixel_0;
    uint8_t * ram_0;
    uint8_t * rom_0;
    uint32_t initial_tick;
    uint32_t frame_count;
} ctx_t;

// static const uint8_t indexes[12] = {
//     0b000000,
//     0b000001,
//     0b000010,
//     0b000011,

//     0b010000,
//     0b010001,
//     0b010010,
//     0b010011,

//     0b100000,
//     0b100001,
//     0b100010,
//     0b100011,
// };

// static const lv_color32_t palettes[1][12] = {
//     {
//         COLOR_HEX(0xFFFFFF),
//         COLOR_HEX(0x7BFF31),
//         COLOR_HEX(0x008400),
//         COLOR_HEX(0x000000),

//         COLOR_HEX(0xFFFFFF),
//         COLOR_HEX(0xFF8484),
//         COLOR_HEX(0x943A3A),
//         COLOR_HEX(0x000000),

//         COLOR_HEX(0xFFFFFF),
//         COLOR_HEX(0xFF8484),
//         COLOR_HEX(0x943A3A),
//         COLOR_HEX(0x000000),
//     },
// };

static const uint16_t palettes_x[1][36] = {
    {
        [0b000000] = COLOR_HEX_16(0xFFFFFF),
        [0b000001] = COLOR_HEX_16(0x7BFF31),
        [0b000010] = COLOR_HEX_16(0x008400),
        [0b000011] = COLOR_HEX_16(0x000000),

        [0b010000] = COLOR_HEX_16(0xFFFFFF),
        [0b010001] = COLOR_HEX_16(0xFF8484),
        [0b010010] = COLOR_HEX_16(0x943A3A),
        [0b010011] = COLOR_HEX_16(0x000000),

        [0b100000] = COLOR_HEX_16(0xFFFFFF),
        [0b100001] = COLOR_HEX_16(0xFF8484),
        [0b100010] = COLOR_HEX_16(0x943A3A),
        [0b100011] = COLOR_HEX_16(0x000000),
    },
};

static void tim_cb(lv_timer_t * tim);

static void step(ctx_t * ctx) {
    uint32_t ms_since_start = lv_tick_get() - ctx->initial_tick;
    const double d_ms_since_start = ms_since_start;
    const double mspf = 1000.0 / VERTICAL_SYNC;

    double d_frames_should_have_run = d_ms_since_start / mspf;
    uint32_t frames_should_have_run = d_frames_should_have_run;
    uint32_t frames_to_run = frames_should_have_run - ctx->frame_count;
    ctx->frame_count = frames_should_have_run;

    if(frames_to_run) {
        while(frames_to_run--) {
            gb_run_frame(&ctx->gb);
        }

        for(uint32_t i = 0; i < (160 * 144); i++) {
            ctx->draw_buf_pixel_0[i] = palettes_x[0][ctx->tmp_buf[i]];
        }

        lv_obj_invalidate(ctx->canv);
    }

    double d_ms_until_next = fmod(d_ms_since_start, mspf);
    uint32_t ms_unitl_next = d_ms_until_next;
    lv_timer_t * tim = lv_timer_create(tim_cb, ms_unitl_next + 1, ctx);
    lv_timer_set_auto_delete(tim, true);
    lv_timer_set_repeat_count(tim, 1);
}

static void tim_cb(lv_timer_t * tim)
{
    ctx_t * ctx = lv_timer_get_user_data(tim);
    step(ctx);
}

static uint8_t rom_read(struct gb_s* gb, const uint_fast32_t addr) {
    ctx_t * ctx = gb->direct.priv;
    return ctx->rom_0[addr];
}

static uint8_t ram_read(struct gb_s* gb, const uint_fast32_t addr) {
    ctx_t * ctx = gb->direct.priv;
    return ctx->ram_0[addr];
}

static void ram_write(struct gb_s* gb, const uint_fast32_t addr, const uint8_t val) {
    ctx_t * ctx = gb->direct.priv;
    ctx->ram_0[addr] = val;
}

static void error(struct gb_s* gb, const enum gb_error_e e, const uint16_t addr) {
    (void)gb;
    fprintf(stderr, "ReanutGB FATAL error. code: %d. rom addr: %u", e, (unsigned) addr);
    exit(1);
}

static void draw_line(
    struct gb_s *gb,
    const uint8_t *pixels,
    const uint_fast8_t line
) {
    ctx_t * ctx = gb->direct.priv;
    memcpy(ctx->tmp_buf + (160 * line), pixels, 160);
}

void gb_app(void)
{
    lv_obj_t * scr = lv_screen_active();
    
    void * buf = calloc(1, 160 * 144 * 2);
    assert(buf);
    lv_obj_t * canv = lv_canvas_create(scr);
    lv_canvas_set_buffer(canv, buf, 160, 144, LV_COLOR_FORMAT_RGB565);
    lv_obj_center(canv);
    lv_image_set_scale(canv, 256 * 2);
    lv_image_set_antialias(canv, false);

    // for(uint32_t i = 0; i < 12; i++) {
    //     lv_canvas_set_palette(canv, indexes[i], palettes[0][i]);
    // }

    ctx_t * ctx = malloc(sizeof(ctx_t));
    assert(ctx);
    ctx->canv = canv;
    ctx->draw_buf_pixel_0 = buf;
    ctx->ram_0 = NULL;
    ctx->rom_0 = red_rom;

    enum gb_init_error_e err = gb_init(
        &ctx->gb,
        rom_read,
        ram_read,
        ram_write,
        error,
        ctx
    );
    assert(err == GB_INIT_NO_ERROR);

    ctx->ram_0 = calloc(1, gb_get_save_size(&ctx->gb));
    assert(ctx->ram_0);

    gb_init_lcd(&ctx->gb, draw_line);

    ctx->initial_tick = lv_tick_get();
    ctx->frame_count = 0;
    step(ctx);
}
