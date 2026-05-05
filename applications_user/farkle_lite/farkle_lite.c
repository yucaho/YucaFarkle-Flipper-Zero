#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>

#define DICE_COUNT 6U
#define FULL_MASK 0x3FU
#define ANIM_FRAMES 20U
#define ANIM_DURATION_MS 1000U
#define ANIM_FRAME_MS (ANIM_DURATION_MS / ANIM_FRAMES)
#define HOT_DICE_MS 900U
#define FARKLE_REVEAL_MS 450U
#define FARKLE_SCREEN_MS 950U

typedef enum {
    OverlayNone,
    OverlayHotDice,
    OverlayFarkle,
} OverlayMode;

typedef struct {
    uint8_t dice[DICE_COUNT];
    uint8_t anim_faces[DICE_COUNT][ANIM_FRAMES];
    uint8_t cursor;
    uint8_t selected_mask;
    uint8_t held_mask;
    int32_t turn_score;
    int32_t bank_score;
    bool must_score_after_roll;
    OverlayMode overlay;
} FarkleLiteApp;

static uint32_t app_now_ms(void) {
    return (furi_get_tick() * 1000U) / furi_kernel_get_tick_frequency();
}
static uint8_t bit_for_die(uint8_t idx) { return (uint8_t)(1U << idx); }
static uint8_t die_face_random(void) { return (uint8_t)(1U + (furi_hal_random_get() % 6U)); }

static void input_callback(InputEvent* input_event, void* ctx) {
    furi_message_queue_put((FuriMessageQueue*)ctx, input_event, FuriWaitForever);
}

static void app_roll_faces(FarkleLiteApp* app, uint8_t roll_mask) {
    for(uint8_t i = 0; i < DICE_COUNT; i++) if(roll_mask & bit_for_die(i)) app->dice[i] = die_face_random();
}

static int32_t score_mask_exact(const FarkleLiteApp* app, uint8_t mask) {
    uint8_t c[7] = {0};
    uint8_t n = 0;
    for(uint8_t i = 0; i < DICE_COUNT; i++) {
        if(mask & bit_for_die(i)) {
            c[app->dice[i]]++;
            n++;
        }
    }
    if(n == 0) return 0;
    if(n == 6) {
        bool s16 = true, s15 = true, s26 = true;
        for(uint8_t f = 1; f <= 6; f++) if(c[f] != 1) s16 = false;
        for(uint8_t f = 1; f <= 5; f++) if(c[f] != 1) s15 = false;
        for(uint8_t f = 2; f <= 6; f++) if(c[f] != 1) s26 = false;
        if(s16) return 1500;
        if(s15 && c[6] == 0) return 500;
        if(s26 && c[1] == 0) return 750;
        uint8_t pairs = 0, trips = 0;
        bool four = false;
        for(uint8_t f = 1; f <= 6; f++) {
            if(c[f] == 2) pairs++;
            if(c[f] == 3) trips++;
            if(c[f] == 4) four = true;
        }
        if(pairs == 3) return 1500;
        if(four && pairs == 1) return 1500;
        if(trips == 2) return 2500;
    }

    int32_t score = 0;
    uint8_t used = 0;
    for(uint8_t f = 1; f <= 6; f++) {
        if(c[f] >= 6) {
            score += 3000;
            c[f] -= 6;
            used += 6;
        }
        if(c[f] >= 5) {
            score += 2000;
            c[f] -= 5;
            used += 5;
        }
        if(c[f] >= 4) {
            score += 1000;
            c[f] -= 4;
            used += 4;
        }
        if(c[f] >= 3) {
            score += (f == 1) ? 1000 : (int32_t)f * 100;
            c[f] -= 3;
            used += 3;
        }
    }
    score += (int32_t)c[1] * 100;
    used += c[1];
    c[1] = 0;
    score += (int32_t)c[5] * 50;
    used += c[5];
    c[5] = 0;
    for(uint8_t f = 2; f <= 6; f++) if(c[f] != 0) return 0;
    return (used == n) ? score : 0;
}

static bool any_scoring_in_mask(const FarkleLiteApp* app, uint8_t mask) {
    for(uint8_t sub = mask; sub; sub = (uint8_t)((sub - 1U) & mask)) if(score_mask_exact(app, sub) > 0) return true;
    return false;
}

static void draw_die(Canvas* canvas, uint8_t x, uint8_t y, uint8_t face, bool selected, bool held, bool cursor) {
    canvas_draw_rframe(canvas, x, y, 22, 22, 2);
    if(selected) canvas_draw_rframe(canvas, x + 1, y + 1, 20, 20, 2);
    if(held) canvas_draw_frame(canvas, x + 3, y + 3, 16, 16);
    if(cursor) canvas_draw_frame(canvas, x - 2, y - 2, 26, 26);
    const uint8_t cx = x + 11, cy = y + 11;
    if(face == 1 || face == 3 || face == 5) canvas_draw_dot(canvas, cx, cy);
    if(face >= 2) { canvas_draw_dot(canvas, x + 6, y + 6); canvas_draw_dot(canvas, x + 16, y + 16); }
    if(face >= 4) { canvas_draw_dot(canvas, x + 16, y + 6); canvas_draw_dot(canvas, x + 6, y + 16); }
    if(face == 6) { canvas_draw_dot(canvas, x + 6, cy); canvas_draw_dot(canvas, x + 16, cy); }
}

static void draw_hot_dice_overlay(Canvas* canvas) {
    const uint8_t cx = 64, cy = 32;
    for(uint8_t a = 0; a < 24; a++) {
        int8_t dx = (int8_t)((a * 7U) % 17U) - 8;
        int8_t dy = (int8_t)((a * 11U) % 13U) - 6;
        canvas_draw_line(canvas, cx, cy, cx + dx * 8, cy + dy * 5);
    }
    for(uint8_t i = 0; i < 12; i++) {
        int8_t x1 = cx + (((int8_t)((i * 5U) % 13U) - 6) * 3);
        int8_t y1 = cy + (((int8_t)((i * 7U) % 11U) - 5) * 2);
        int8_t x2 = cx + (((int8_t)(((i + 1U) * 5U) % 13U) - 6) * 3);
        int8_t y2 = cy + (((int8_t)(((i + 1U) * 7U) % 11U) - 5) * 2);
        canvas_draw_line(canvas, x1, y1, x2, y2);
    }
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 27, 24, 74, 16);
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_str_aligned(canvas, 64, 35, AlignCenter, AlignBottom, "HOT DICE!");
    canvas_set_color(canvas, ColorBlack);
}

static void draw_farkle_screen(Canvas* canvas) {
    for(uint8_t x = 0; x < 128; x += 4) canvas_draw_vline(canvas, x, 0, 64);
    canvas_draw_str_aligned(canvas, 64, 38, AlignCenter, AlignCenter, "FARKLE...");
}

static void render_callback(Canvas* canvas, void* context) {
    FarkleLiteApp* app = context;
    canvas_clear(canvas);
    if(app->overlay == OverlayFarkle) {
        draw_farkle_screen(canvas);
        return;
    }
    canvas_draw_str(canvas, 2, 9, "Turn");
    canvas_draw_str(canvas, 2, 18, "Bank");
    char line[24];
    snprintf(line, sizeof(line), "%ld", app->turn_score);
    canvas_draw_str(canvas, 30, 9, line);
    snprintf(line, sizeof(line), "%ld", app->bank_score);
    canvas_draw_str(canvas, 30, 18, line);
    for(uint8_t i = 0; i < DICE_COUNT; i++) {
        draw_die(canvas, 58 + (i % 3U) * 23U, 2 + (i / 3U) * 24U, app->dice[i], app->selected_mask & bit_for_die(i), app->held_mask & bit_for_die(i), app->cursor == i);
    }
    if(app->must_score_after_roll) canvas_draw_str(canvas, 2, 31, "Score before roll");
    if(app->overlay == OverlayHotDice) draw_hot_dice_overlay(canvas);
}

static void animate_roll(FarkleLiteApp* app, ViewPort* vp, uint8_t roll_mask) {
    for(uint8_t i = 0; i < DICE_COUNT; i++) if(roll_mask & bit_for_die(i)) for(uint8_t f = 0; f < ANIM_FRAMES; f++) app->anim_faces[i][f] = die_face_random();
    for(uint8_t frame = 0; frame < ANIM_FRAMES; frame++) {
        for(uint8_t i = 0; i < DICE_COUNT; i++) if(roll_mask & bit_for_die(i)) app->dice[i] = app->anim_faces[i][frame];
        view_port_update(vp);
        furi_delay_ms(ANIM_FRAME_MS);
    }
    app_roll_faces(app, roll_mask);
}

int32_t farkle_lite_app(void* p) {
    UNUSED(p);
    FarkleLiteApp app = {.overlay = OverlayNone};
    app_roll_faces(&app, FULL_MASK);
    FuriMessageQueue* queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    ViewPort* vp = view_port_alloc();
    view_port_draw_callback_set(vp, render_callback, &app);
    view_port_input_callback_set(vp, input_callback, queue);
    Gui* gui = furi_record_open("gui");
    gui_add_view_port(gui, vp, GuiLayerFullscreen);

    bool running = true;
    InputEvent event;
    while(running) {
        if(furi_message_queue_get(queue, &event, 100) != FuriStatusOk) continue;
        if(event.type == InputTypeShort) {
            if(event.key == InputKeyBack) running = false;
            else if(event.key == InputKeyLeft && (app.cursor % 3U > 0U)) app.cursor--;
            else if(event.key == InputKeyRight && (app.cursor % 3U < 2U)) app.cursor++;
            else if(event.key == InputKeyUp && app.cursor >= 3U) app.cursor -= 3U;
            else if(event.key == InputKeyDown && app.cursor < 3U) app.cursor += 3U;
            else if(event.key == InputKeyOk) { uint8_t b = bit_for_die(app.cursor); if((app.held_mask & b) == 0U) app.selected_mask ^= b; }
        } else if(event.type == InputTypeLong) {
            if(event.key == InputKeyOk && !app.must_score_after_roll) {
                uint8_t roll_mask = (uint8_t)(FULL_MASK & ~app.held_mask);
                if(roll_mask == 0U) roll_mask = FULL_MASK;
                animate_roll(&app, vp, roll_mask);
                app.selected_mask = 0;
                if(!any_scoring_in_mask(&app, roll_mask)) {
                    view_port_update(vp);
                    furi_delay_ms(FARKLE_REVEAL_MS);
                    app.overlay = OverlayFarkle;
                    view_port_update(vp);
                    furi_delay_ms(FARKLE_SCREEN_MS);
                    app.overlay = OverlayNone;
                    app.turn_score = 0;
                    app.held_mask = 0;
                    app.selected_mask = 0;
                    animate_roll(&app, vp, FULL_MASK);
                } else app.must_score_after_roll = true;
            } else if(event.key == InputKeyRight && app.selected_mask != 0U) {
                int32_t score = score_mask_exact(&app, app.selected_mask);
                if(score > 0) {
                    app.turn_score += score;
                    app.held_mask |= app.selected_mask;
                    app.selected_mask = 0;
                    app.must_score_after_roll = false;
                    if(app.held_mask == FULL_MASK) {
                        app.overlay = OverlayHotDice;
                        uint32_t start = app_now_ms();
                        while((app_now_ms() - start) < HOT_DICE_MS) {
                            view_port_update(vp);
                            furi_delay_ms(50);
                        }
                        app.overlay = OverlayNone;
                        app.held_mask = 0;
                        animate_roll(&app, vp, FULL_MASK);
                        app.must_score_after_roll = true;
                    }
                }
            } else if(event.key == InputKeyDown) {
                app.bank_score += app.turn_score;
                app.turn_score = 0;
                app.held_mask = 0;
                app.selected_mask = 0;
                animate_roll(&app, vp, FULL_MASK);
                app.must_score_after_roll = true;
            }
        }
        view_port_update(vp);
    }

    gui_remove_view_port(gui, vp);
    furi_record_close("gui");
    view_port_free(vp);
    furi_message_queue_free(queue);
    return 0;
}
