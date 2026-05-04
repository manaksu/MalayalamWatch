/*
 * Keralam Watchface
 * Pebble Time Steel / Basalt (144x168)
 *
 * Cream e-paper background with Malayalam numeral bitmaps
 * positioned around the screen perimeter. Analog pixel hands.
 *
 * Clock center: (72, 84) — center of full 144x168 screen
 *
 * Numeral positions (bitmap center):
 *   Top    y=8:   10(18) 11(45) 12(72) 01(99) 02(126)
 *   Right  x=136: 03(84)
 *   Bottom y=160: 08(18) 07(45) 06(72) 05(99) 04(126)
 *   Left   x=8:   09(84)
 */

#include <pebble.h>

#define CX          72
#define CY          84
#define HOUR_LEN    42
#define HOUR_TAIL   7
#define HOUR_W      3
#define MIN_LEN     60
#define MIN_TAIL    10
#define MIN_W       2

typedef struct { int8_t x; int8_t y; } Pt;

static const Pt NUM_POS[13] = {
  {   0,   0 },
  {  99,   8 },   /* 01 */
  { 126,   8 },   /* 02 */
  { 136,  84 },   /* 03 */
  { 126, 160 },   /* 04 */
  {  99, 160 },   /* 05 */
  {  72, 160 },   /* 06 */
  {  45, 160 },   /* 07 */
  {  18, 160 },   /* 08 */
  {   8,  84 },   /* 09 */
  {  18,   8 },   /* 10 */
  {  45,   8 },   /* 11 */
  {  72,   8 },   /* 12 */
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
  /* Cream background */
  graphics_context_set_fill_color(ctx, GColorFromRGB(240, 236, 224));
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);

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

  int32_t h_angle = (TRIG_MAX_ANGLE *
    ((s_hour % 12) * 60 + s_min)) / (12 * 60) - TRIG_MAX_ANGLE / 4;
  int32_t m_angle = (TRIG_MAX_ANGLE * s_min) / 60
    - TRIG_MAX_ANGLE / 4;

  draw_hand(ctx, h_angle, HOUR_LEN, HOUR_TAIL, HOUR_W);
  draw_hand(ctx, m_angle, MIN_LEN,  MIN_TAIL,  MIN_W);

  /* Center cap */
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_circle(ctx, GPoint(CX, CY), 3);
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
