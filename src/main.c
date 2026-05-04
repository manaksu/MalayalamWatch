/*
 * Keralam Watchface
 * Pebble Time Steel / Basalt (144x168)
 *
 * Clock face: 144x84 (top half), cream background
 * Center: (72, 42)
 *
 * Numeral positions (bitmap center):
 *   Top    y=7:   10(16) 11(40) 12(64) 01(92) 02(118)
 *   Right  x=130: 03(42)
 *   Bottom y=77:  08(16) 07(40) 06(64) 05(92) 04(118)
 *   Left   x=10:  09(42)
 *
 * Lower half (y=84..168): reserved for Chundan Vallam battery
 */

#include <pebble.h>

#define CX        72
#define CY        42
#define HOUR_LEN  26
#define HOUR_TAIL  5
#define HOUR_W     3
#define MIN_LEN   34
#define MIN_TAIL   6
#define MIN_W      2

typedef struct { int8_t x; int8_t y; } Pt;

static const Pt NUM_POS[13] = {
  {   0,  0 },
  {  92,  7 },  /* 01 */
  { 118,  7 },  /* 02 */
  { 130, 42 },  /* 03 */
  { 118, 77 },  /* 04 */
  {  92, 77 },  /* 05 */
  {  64, 77 },  /* 06 */
  {  40, 77 },  /* 07 */
  {  16, 77 },  /* 08 */
  {  10, 42 },  /* 09 */
  {  16,  7 },  /* 10 */
  {  40,  7 },  /* 11 */
  {  64,  7 },  /* 12 */
};

static const uint32_t NUM_RES[13] = {
  0,
  RESOURCE_ID_NUM_01, RESOURCE_ID_NUM_02, RESOURCE_ID_NUM_03,
  RESOURCE_ID_NUM_04, RESOURCE_ID_NUM_05, RESOURCE_ID_NUM_06,
  RESOURCE_ID_NUM_07, RESOURCE_ID_NUM_08, RESOURCE_ID_NUM_09,
  RESOURCE_ID_NUM_10, RESOURCE_ID_NUM_11, RESOURCE_ID_NUM_12,
};

static Window  *s_window;
static Layer   *s_canvas;
static GBitmap *s_bmp[13];
static int      s_hour, s_min;

static void draw_hand(GContext *ctx, int32_t angle,
                      int len, int tail, int width) {
  graphics_context_set_stroke_width(ctx, width);
  int32_t c = cos_lookup(angle);
  int32_t s = sin_lookup(angle);
  GPoint tip  = { CX + len  * c / TRIG_MAX_RATIO,
                  CY + len  * s / TRIG_MAX_RATIO };
  GPoint back = { CX - tail * c / TRIG_MAX_RATIO,
                  CY - tail * s / TRIG_MAX_RATIO };
  graphics_draw_line(ctx, back, tip);
}

static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  /* Cream background */
  graphics_context_set_fill_color(ctx, GColorFromRGB(240, 236, 224));
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* Numeral bitmaps — black ink on transparent */
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  for (int i = 1; i <= 12; i++) {
    if (!s_bmp[i]) continue;
    GRect bb = gbitmap_get_bounds(s_bmp[i]);
    int x = NUM_POS[i].x - bb.size.w / 2;
    int y = NUM_POS[i].y - bb.size.h / 2;
    graphics_draw_bitmap_in_rect(ctx, s_bmp[i],
                                 GRect(x, y, bb.size.w, bb.size.h));
  }

  /* Hands */
  GColor ink = GColorFromRGB(42, 42, 34);
  graphics_context_set_stroke_color(ctx, ink);

  int32_t h_angle =
    (TRIG_MAX_ANGLE * ((s_hour % 12) * 60 + s_min)) / (12 * 60)
    - TRIG_MAX_ANGLE / 4;
  int32_t m_angle =
    (TRIG_MAX_ANGLE * s_min) / 60
    - TRIG_MAX_ANGLE / 4;

  draw_hand(ctx, h_angle, HOUR_LEN, HOUR_TAIL, HOUR_W);
  draw_hand(ctx, m_angle, MIN_LEN,  MIN_TAIL,  MIN_W);

  /* Center cap */
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_circle(ctx, GPoint(CX, CY), 3);

  /* Divider line */
  graphics_context_set_stroke_color(ctx, GColorFromRGB(180, 176, 164));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(0, 84), GPoint(144, 84));
}

static void tick_handler(struct tm *t, TimeUnits changed) {
  s_hour = t->tm_hour;
  s_min  = t->tm_min;
  layer_mark_dirty(s_canvas);
}

static void window_load(Window *window) {
  for (int i = 1; i <= 12; i++)
    s_bmp[i] = gbitmap_create_with_resource(NUM_RES[i]);

  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_draw);
  layer_add_child(root, s_canvas);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hour = t->tm_hour;
  s_min  = t->tm_min;
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  for (int i = 1; i <= 12; i++)
    if (s_bmp[i]) gbitmap_destroy(s_bmp[i]);
}

static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
