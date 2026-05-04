/*
 * Keralam Watchface
 * Pebble Time Steel / Basalt (144x168)
 *
 * Clock face: 144x84 (top half), cream background
 * 60 tick marks around the rectangular perimeter
 * Malayalam numerals placed geometrically, with fine-tuned offsets
 * Center: (72, 42)
 *
 * Lower half (y=84..168): reserved for Chundan Vallam battery
 */

#include <pebble.h>

#define CX         72
#define CY         42
#define W          144
#define H_CLOCK    84
#define HOUR_LEN   26
#define HOUR_TAIL   5
#define HOUR_W      3
#define MIN_LEN    34
#define MIN_TAIL    6
#define MIN_W       2

#define PERIM        456
#define TOP_CENTER_D  72

typedef struct { int16_t x; int16_t y; int8_t edge; } PerimPt;

static PerimPt perim_to_xy(int32_t d) {
  d = ((d % PERIM) + PERIM) % PERIM;
  PerimPt p;
  if (d < W) {
    p.x = d; p.y = 0; p.edge = 0;
  } else if (d < W + H_CLOCK) {
    p.x = W; p.y = d - W; p.edge = 1;
  } else if (d < 2*W + H_CLOCK) {
    p.x = W - (d - W - H_CLOCK); p.y = H_CLOCK; p.edge = 2;
  } else {
    p.x = 0; p.y = H_CLOCK - (d - 2*W - H_CLOCK); p.edge = 3;
  }
  return p;
}

static GPoint inward(PerimPt p, int dist) {
  switch (p.edge) {
    case 0: return GPoint(p.x, p.y + dist);
    case 1: return GPoint(p.x - dist, p.y);
    case 2: return GPoint(p.x, p.y - dist);
    default: return GPoint(p.x + dist, p.y);
  }
}

/*
 * Per-numeral fine-tune offsets (dx, dy):
 * Hours 1..12 in order.
 *
 * Top row (edge=0): corners 10,02 lower +2; middles 11,12,01 raise -2
 * Bottom row (edge=2): corners 08,04 raise -2; middles 07,06,05 lower +2
 * Left/right (edge 1,3): no change
 */
static const int8_t NUM_DX[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const int8_t NUM_DY[13] = {
  0,    /* unused */
  -2,   /* 01 — top middle → up */
  +2,   /* 02 — top right corner → down */
  0,    /* 03 — right edge */
  -2,   /* 04 — bottom right corner → up */
  +2,   /* 05 — bottom middle → down */
  +2,   /* 06 — bottom middle → down */
  +2,   /* 07 — bottom middle → down */
  -2,   /* 08 — bottom left corner → up */
  0,    /* 09 — left edge */
  +2,   /* 10 — top left corner → down */
  -2,   /* 11 — top middle → up */
  -2,   /* 12 — top middle → up */
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
  GColor ink       = GColorFromRGB(42, 42, 34);
  GColor cream     = GColorFromRGB(240, 236, 224);
  GColor tick_minor = GColorFromRGB(160, 156, 144);
  GColor tick_major = GColorFromRGB(80, 78, 70);

  /* Cream background */
  graphics_context_set_fill_color(ctx, cream);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* 60 tick marks */
  for (int i = 0; i < 60; i++) {
    int32_t d = TOP_CENTER_D + (int32_t)i * PERIM / 60;
    PerimPt p = perim_to_xy(d);
    bool major = (i % 5 == 0);
    int tick_len = major ? 5 : 3;
    GColor tc = major ? tick_major : tick_minor;
    GPoint outer = GPoint(p.x, p.y);
    GPoint inner = inward(p, tick_len);
    graphics_context_set_stroke_color(ctx, tc);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, outer, inner);
  }

  /* Numeral bitmaps */
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  for (int h = 1; h <= 12; h++) {
    if (!s_bmp[h]) continue;
    int32_t d = TOP_CENTER_D + (int32_t)h * PERIM / 12;
    PerimPt p = perim_to_xy(d);
    GPoint np = inward(p, 11);
    /* Apply fine-tune offset */
    np.x += NUM_DX[h];
    np.y += NUM_DY[h];
    GRect bb = gbitmap_get_bounds(s_bmp[h]);
    int x = np.x - bb.size.w / 2;
    int y = np.y - bb.size.h / 2;
    graphics_draw_bitmap_in_rect(ctx, s_bmp[h],
                                 GRect(x, y, bb.size.w, bb.size.h));
  }

  /* Hands */
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
