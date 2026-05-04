/*
 * Keralam Watchface
 * Pebble Time Steel / Basalt (144x168)
 *
 * Top half (144x84): Analog clock, cream bg, Malayalam numerals,
 *                    60 tick marks around perimeter
 * Bottom half (144x84): Archimedean spiral battery indicator,
 *                       bottom-left corner, fills outward from center
 *
 * Clock center: (72, 42)
 * Spiral center: (22, 155)
 */

#include <pebble.h>
#include <math.h>

#define CX          72
#define CY          42
#define HOUR_LEN    26
#define HOUR_TAIL    5
#define HOUR_W       3
#define MIN_LEN     34
#define MIN_TAIL     6
#define MIN_W        2

/* Spiral config */
#define SP_CX       22      /* spiral center x */
#define SP_CY      155      /* spiral center y */
#define SP_MAX_R    18      /* outermost radius px */
#define SP_TURNS     4      /* number of rings */
#define SP_STEPS   200      /* path resolution */

/* Perimeter layout */
#define PERIM       456
#define TOP_CTR_D    72

typedef struct { int16_t x; int16_t y; int8_t edge; } PerimPt;

static PerimPt perim_to_xy(int32_t d) {
  d = ((d % PERIM) + PERIM) % PERIM;
  PerimPt p;
  if (d < 144)              { p.x = d;       p.y = 0;       p.edge = 0; }
  else if (d < 144+84)      { p.x = 144;     p.y = d-144;   p.edge = 1; }
  else if (d < 144+84+144)  { p.x = 144-(d-144-84); p.y = 84; p.edge = 2; }
  else                      { p.x = 0; p.y = 84-(d-144-84-144); p.edge = 3; }
  return p;
}

static GPoint inward(PerimPt p, int dist) {
  switch(p.edge) {
    case 0: return GPoint(p.x, p.y + dist);
    case 1: return GPoint(p.x - dist, p.y);
    case 2: return GPoint(p.x, p.y - dist);
    default: return GPoint(p.x + dist, p.y);
  }
}

static const int8_t NUM_DY[13] = {
  0, -2,+2, 0,-2,+2,+2,+2,-2, 0,+2,-2,-2
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
static int      s_battery_pct = 100;

/* ── Hand drawing ─────────────────────────────────────────── */
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

/* ── Spiral drawing ───────────────────────────────────────── */
/*
 * Archimedean spiral: r = MAX_R - (MAX_R / total_angle) * a
 * Drawn as a GPath approximation using line segments.
 * We draw two passes:
 *   1. Full ghost (dim) outline
 *   2. Filled portion up to battery % (ink)
 */
static void draw_spiral(GContext *ctx, int pct) {
  const float total_angle = SP_TURNS * 2.0f * M_PI;
  const float filled_angle = total_angle * (pct / 100.0f);
  const float gap = (float)SP_MAX_R / total_angle;

  GColor ink   = GColorFromRGB(42, 42, 34);
  GColor ghost = GColorFromRGB(210, 206, 194);

  /* Ghost — full spiral */
  graphics_context_set_stroke_color(ctx, ghost);
  graphics_context_set_stroke_width(ctx, 2);
  GPoint prev, curr;
  bool first = true;
  for (int i = 0; i <= SP_STEPS; i++) {
    float a = (total_angle * i) / SP_STEPS;
    float r = SP_MAX_R - gap * a;
    if (r < 1.0f) break;
    float angle = a - M_PI / 2.0f;
    curr = GPoint(
      SP_CX + (int)(r * cos(angle)),
      SP_CY + (int)(r * sin(angle))
    );
    if (!first) graphics_draw_line(ctx, prev, curr);
    prev = curr;
    first = false;
  }

  /* Filled — up to battery % */
  if (pct <= 0) return;
  graphics_context_set_stroke_color(ctx, ink);
  graphics_context_set_stroke_width(ctx, 3);
  first = true;
  int filled_steps = (int)(SP_STEPS * (filled_angle / total_angle));
  for (int i = 0; i <= filled_steps; i++) {
    float a = (total_angle * i) / SP_STEPS;
    float r = SP_MAX_R - gap * a;
    if (r < 1.0f) break;
    float angle = a - M_PI / 2.0f;
    curr = GPoint(
      SP_CX + (int)(r * cos(angle)),
      SP_CY + (int)(r * sin(angle))
    );
    if (!first) graphics_draw_line(ctx, prev, curr);
    prev = curr;
    first = false;
  }

  /* Center dot */
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_circle(ctx, GPoint(SP_CX, SP_CY), 2);
}

/* ── Canvas draw ──────────────────────────────────────────── */
static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GColor cream = GColorFromRGB(240, 236, 224);
  GColor ink   = GColorFromRGB(42, 42, 34);

  /* Full cream background */
  graphics_context_set_fill_color(ctx, cream);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* ── 60 tick marks (clock face perimeter) ── */
  for (int i = 0; i < 60; i++) {
    int32_t d = TOP_CTR_D + (int32_t)i * PERIM / 60;
    PerimPt p = perim_to_xy(d);
    bool major = (i % 5 == 0);
    GPoint outer = GPoint(p.x, p.y);
    GPoint inner = inward(p, major ? 5 : 3);
    graphics_context_set_stroke_color(ctx,
      major ? GColorFromRGB(80,76,64) : GColorFromRGB(160,154,136));
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, outer, inner);
  }

  /* ── Numeral bitmaps ── */
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  for (int h = 1; h <= 12; h++) {
    if (!s_bmp[h]) continue;
    int32_t d = TOP_CTR_D + (int32_t)h * PERIM / 12;
    PerimPt p = perim_to_xy(d);
    GPoint np = inward(p, 11);
    np.y += NUM_DY[h];
    GRect bb = gbitmap_get_bounds(s_bmp[h]);
    int x = np.x - bb.size.w / 2;
    int y = np.y - bb.size.h / 2;
    graphics_draw_bitmap_in_rect(ctx, s_bmp[h],
                                 GRect(x, y, bb.size.w, bb.size.h));
  }

  /* ── Clock hands ── */
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

  /* Divider */
  graphics_context_set_stroke_color(ctx, GColorFromRGB(180,174,158));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(0, 84), GPoint(144, 84));

  /* ── Spiral battery ── */
  draw_spiral(ctx, s_battery_pct);
}

/* ── Battery handler ──────────────────────────────────────── */
static void battery_handler(BatteryChargeState state) {
  s_battery_pct = state.charge_percent;
  layer_mark_dirty(s_canvas);
}

/* ── Tick handler ─────────────────────────────────────────── */
static void tick_handler(struct tm *t, TimeUnits changed) {
  s_hour = t->tm_hour;
  s_min  = t->tm_min;
  layer_mark_dirty(s_canvas);
}

/* ── Window lifecycle ─────────────────────────────────────── */
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

  s_battery_pct = battery_state_service_peek().charge_percent;
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  for (int i = 1; i <= 12; i++)
    if (s_bmp[i]) gbitmap_destroy(s_bmp[i]);
}

/* ── App entry ────────────────────────────────────────────── */
static void init(void) {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
