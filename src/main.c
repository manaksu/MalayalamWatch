/*
 * Keralam Watchface
 * Pebble Time Steel / Basalt (144x168)
 *
 * Top half (144x84)  : Analog clock, cream bg, Malayalam numerals, 60 ticks
 * Just below divider : Date as "26 മേയ്, 26" — Arabic day/year + Malayalam month bitmap
 * Bottom half        : Archimedean spiral battery indicator, bottom-left
 *
 * Clock center: (72, 42)
 * Spiral center: (26, 148)
 */

#include <pebble.h>

#define CX          72
#define CY          42
#define HOUR_LEN    26
#define HOUR_TAIL    5
#define HOUR_W       3
#define MIN_LEN     34
#define MIN_TAIL     6
#define MIN_W        2

#define SP_CX       32
#define SP_CY       42
#define SP_MAX_R    12
#define SP_TURNS     3
#define SP_STEPS   180

#define PERIM       456
#define TOP_CTR_D    72

typedef struct { int16_t x; int16_t y; int8_t edge; } PerimPt;

static PerimPt perim_to_xy(int32_t d) {
  d = ((d % PERIM) + PERIM) % PERIM;
  PerimPt p;
  if (d < 144)             { p.x = d;             p.y = 0;            p.edge = 0; }
  else if (d < 144+84)     { p.x = 144;            p.y = d-144;        p.edge = 1; }
  else if (d < 144+84+144) { p.x = 144-(d-144-84); p.y = 84;           p.edge = 2; }
  else                     { p.x = 0;              p.y = 84-(d-144-84-144); p.edge = 3; }
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

static const uint32_t MONTH_RES[13] = {
  0,
  RESOURCE_ID_MONTH_01, RESOURCE_ID_MONTH_02, RESOURCE_ID_MONTH_03,
  RESOURCE_ID_MONTH_04, RESOURCE_ID_MONTH_05, RESOURCE_ID_MONTH_06,
  RESOURCE_ID_MONTH_07, RESOURCE_ID_MONTH_08, RESOURCE_ID_MONTH_09,
  RESOURCE_ID_MONTH_10, RESOURCE_ID_MONTH_11, RESOURCE_ID_MONTH_12,
};

static const uint32_t DAY_RES[7] = {
  RESOURCE_ID_DAY_0, RESOURCE_ID_DAY_1, RESOURCE_ID_DAY_2,
  RESOURCE_ID_DAY_3, RESOURCE_ID_DAY_4, RESOURCE_ID_DAY_5,
  RESOURCE_ID_DAY_6,
};

static Window  *s_window;
static Layer   *s_canvas;
static GBitmap *s_bmp[13];
static GBitmap *s_month_bmp[13];
static GBitmap *s_day_bmp[7];
static GBitmap *s_inscription;
static int      s_hour, s_min;
static int      s_mday, s_mon, s_year, s_wday;
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

/* ── Date drawing ─────────────────────────────────────────── */
/*
 * Line 1 (y=88): Day name bitmap  e.g. <ഞായർ>
 * Line 2 (y=102): "26 <മേയ്>, 26"
 * Both left-aligned at x=6
 */
static void draw_date(GContext *ctx) {
  GColor ink = GColorFromRGB(42, 42, 34);
  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  /* Line 1 — "26 <month>, 26" */
  int x = 6;
  int y = 88;

  char day_str[6];
  snprintf(day_str, sizeof(day_str), "%02d ", s_mday);
  char yr_str[6];
  snprintf(yr_str, sizeof(yr_str), ", %02d", s_year % 100);

  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);

  /* Day number */
  graphics_context_set_text_color(ctx, ink);
  graphics_draw_text(ctx, day_str, font, GRect(x, y-2, 30, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  /* GOTHIC_14: each digit ~7px, space ~4px, so "04 " = 18px */
  x += 14;

  /* Month bitmap */
  GBitmap *mbmp = s_month_bmp[s_mon];
  if (mbmp) {
    GRect bb = gbitmap_get_bounds(mbmp);
    int by = y + (12 - bb.size.h) / 2;
    graphics_draw_bitmap_in_rect(ctx, mbmp, GRect(x, by, bb.size.w, bb.size.h));
    x += bb.size.w;
  }

  /* Year */
  graphics_context_set_text_color(ctx, ink);
  graphics_draw_text(ctx, yr_str, font, GRect(x, y-2, 40, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

  /* Line 2 — day of week */
  GBitmap *dbmp = s_day_bmp[s_wday];
  if (dbmp) {
    GRect bb = gbitmap_get_bounds(dbmp);
    graphics_draw_bitmap_in_rect(ctx, dbmp, GRect(6, 102, bb.size.w, bb.size.h));
  }
}

/* ── Spiral drawing ───────────────────────────────────────── */
static void draw_spiral(GContext *ctx, int pct) {
  const int32_t total_trig  = (int32_t)SP_TURNS * TRIG_MAX_ANGLE;
  const int32_t filled_steps = (int32_t)SP_STEPS * pct / 100;
  const int32_t offset = -TRIG_MAX_ANGLE / 4;

  GColor ink   = GColorFromRGB(42, 42, 34);
  GColor ghost = GColorFromRGB(210, 206, 194);

  /* Ghost — full spiral */
  graphics_context_set_stroke_color(ctx, ghost);
  graphics_context_set_stroke_width(ctx, 2);
  GPoint prev = GPoint(SP_CX, SP_CY);
  bool first = true;
  for (int i = 0; i <= SP_STEPS; i++) {
    int32_t trig_angle = offset + total_trig * i / SP_STEPS;
    int32_t r = SP_MAX_R - (int32_t)SP_MAX_R * i / SP_STEPS;
    if (r < 1) break;
    GPoint curr = GPoint(
      SP_CX + (int)(r * cos_lookup(trig_angle) / TRIG_MAX_RATIO),
      SP_CY + (int)(r * sin_lookup(trig_angle) / TRIG_MAX_RATIO)
    );
    if (!first) graphics_draw_line(ctx, prev, curr);
    prev = curr;
    first = false;
  }

  /* Filled — up to battery % */
  if (pct <= 0) {
    graphics_context_set_fill_color(ctx, ink);
    graphics_fill_circle(ctx, GPoint(SP_CX, SP_CY), 2);
    return;
  }
  graphics_context_set_stroke_color(ctx, ink);
  graphics_context_set_stroke_width(ctx, 3);
  first = true;
  for (int i = 0; i <= filled_steps; i++) {
    int32_t trig_angle = offset + total_trig * i / SP_STEPS;
    int32_t r = SP_MAX_R - (int32_t)SP_MAX_R * i / SP_STEPS;
    if (r < 1) break;
    GPoint curr = GPoint(
      SP_CX + (int)(r * cos_lookup(trig_angle) / TRIG_MAX_RATIO),
      SP_CY + (int)(r * sin_lookup(trig_angle) / TRIG_MAX_RATIO)
    );
    if (!first) graphics_draw_line(ctx, prev, curr);
    prev = curr;
    first = false;
  }

  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_circle(ctx, GPoint(SP_CX, SP_CY), 2);
}

/* ── Canvas draw ──────────────────────────────────────────── */
static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GColor cream = GColorFromRGB(240, 236, 224);
  GColor ink   = GColorFromRGB(42, 42, 34);

  /* Cream background */
  graphics_context_set_fill_color(ctx, cream);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* 60 tick marks */
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

  /* Numeral bitmaps */
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

  /* Divider */
  graphics_context_set_stroke_color(ctx, GColorFromRGB(180,174,158));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(0, 84), GPoint(144, 84));

  /* Date strip */
  draw_date(ctx);

  /* Spiral battery — inside clock face, left side */
  draw_spiral(ctx, s_battery_pct);

  /* Inscription — bottom of lower half, centered */
  if (s_inscription) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    GRect bb = gbitmap_get_bounds(s_inscription);
    int x = (144 - bb.size.w) / 2;
    int y = 168 - bb.size.h - 4;
    graphics_draw_bitmap_in_rect(ctx, s_inscription, GRect(x, y, bb.size.w, bb.size.h));
  }
}

/* ── Handlers ─────────────────────────────────────────────── */
static void battery_handler(BatteryChargeState state) {
  s_battery_pct = state.charge_percent;
  layer_mark_dirty(s_canvas);
}

static void tick_handler(struct tm *t, TimeUnits changed) {
  s_hour = t->tm_hour;
  s_min  = t->tm_min;
  s_mday = t->tm_mday;
  s_mon  = t->tm_mon + 1;
  s_year = t->tm_year + 1900;
  s_wday = t->tm_wday;
  layer_mark_dirty(s_canvas);
}

/* ── Window lifecycle ─────────────────────────────────────── */
static void window_load(Window *window) {
  for (int i = 1; i <= 12; i++)
    s_bmp[i] = gbitmap_create_with_resource(NUM_RES[i]);
  for (int i = 1; i <= 12; i++)
    s_month_bmp[i] = gbitmap_create_with_resource(MONTH_RES[i]);
  for (int i = 0; i < 7; i++)
    s_day_bmp[i] = gbitmap_create_with_resource(DAY_RES[i]);
  s_inscription = gbitmap_create_with_resource(RESOURCE_ID_INSCRIPTION);

  Layer *root = window_get_root_layer(window);
  s_canvas = layer_create(layer_get_bounds(root));
  layer_set_update_proc(s_canvas, canvas_draw);
  layer_add_child(root, s_canvas);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  s_hour = t->tm_hour;
  s_min  = t->tm_min;
  s_mday = t->tm_mday;
  s_mon  = t->tm_mon + 1;
  s_year = t->tm_year + 1900;
  s_wday = t->tm_wday;

  s_battery_pct = battery_state_service_peek().charge_percent;
}

static void window_unload(Window *window) {
  layer_destroy(s_canvas);
  for (int i = 1; i <= 12; i++) {
    if (s_bmp[i])       gbitmap_destroy(s_bmp[i]);
    if (s_month_bmp[i]) gbitmap_destroy(s_month_bmp[i]);
  }
  for (int i = 0; i < 7; i++)
    if (s_day_bmp[i]) gbitmap_destroy(s_day_bmp[i]);
  if (s_inscription) gbitmap_destroy(s_inscription);
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
