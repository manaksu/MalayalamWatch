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
#define HOUR_LEN    28
#define HOUR_TAIL    5
#define HOUR_W       3
#define MIN_LEN     38
#define MIN_TAIL     6
#define MIN_W        2

#define SP_CX       39   /* left side of clock face */
#define SP_CY       42   /* vertically centered in clock */
#define SP_MAX_R    17
#define SP_TURNS     3
#define SP_STEPS   360

static const uint32_t NUM_RES[13] = {
  0,
  RESOURCE_ID_NUM_01, RESOURCE_ID_NUM_02, RESOURCE_ID_NUM_03,
  RESOURCE_ID_NUM_04, RESOURCE_ID_NUM_05, RESOURCE_ID_NUM_06,
  RESOURCE_ID_NUM_07, RESOURCE_ID_NUM_08, RESOURCE_ID_NUM_09,
  RESOURCE_ID_NUM_10, RESOURCE_ID_NUM_11, RESOURCE_ID_NUM_12,
};

static const uint32_t NUM_BOLD_RES[13] = {
  0,
  RESOURCE_ID_NUM_B01, RESOURCE_ID_NUM_B02, RESOURCE_ID_NUM_B03,
  RESOURCE_ID_NUM_B04, RESOURCE_ID_NUM_B05, RESOURCE_ID_NUM_B06,
  RESOURCE_ID_NUM_B07, RESOURCE_ID_NUM_B08, RESOURCE_ID_NUM_B09,
  RESOURCE_ID_NUM_B10, RESOURCE_ID_NUM_B11, RESOURCE_ID_NUM_B12,
};

/* Rotated corner bitmaps: indices 0=10, 1=02, 2=08, 3=04 */
static const uint32_t NUM_ROT_RES[4]      = { RESOURCE_ID_NUM_10R,   RESOURCE_ID_NUM_02R,   RESOURCE_ID_NUM_08R,   RESOURCE_ID_NUM_04R };
static const uint32_t NUM_BOLD_ROT_RES[4] = { RESOURCE_ID_NUM_B10R,  RESOURCE_ID_NUM_B02R,  RESOURCE_ID_NUM_B08R,  RESOURCE_ID_NUM_B04R };

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

static const uint32_t GHOST_RES[13] = {
  0,
  RESOURCE_ID_GHOST_01, RESOURCE_ID_GHOST_02, RESOURCE_ID_GHOST_03,
  RESOURCE_ID_GHOST_04, RESOURCE_ID_GHOST_05, RESOURCE_ID_GHOST_06,
  RESOURCE_ID_GHOST_07, RESOURCE_ID_GHOST_08, RESOURCE_ID_GHOST_09,
  RESOURCE_ID_GHOST_10, RESOURCE_ID_GHOST_11, RESOURCE_ID_GHOST_12,
};

static const uint32_t GHOST_BOLD_RES[13] = {
  0,
  RESOURCE_ID_GHOST_B01, RESOURCE_ID_GHOST_B02, RESOURCE_ID_GHOST_B03,
  RESOURCE_ID_GHOST_B04, RESOURCE_ID_GHOST_B05, RESOURCE_ID_GHOST_B06,
  RESOURCE_ID_GHOST_B07, RESOURCE_ID_GHOST_B08, RESOURCE_ID_GHOST_B09,
  RESOURCE_ID_GHOST_B10, RESOURCE_ID_GHOST_B11, RESOURCE_ID_GHOST_B12,
};

static Window  *s_window;
static Layer   *s_canvas;
static GBitmap *s_bmp[13];
static GBitmap *s_bmp_bold[13];
static GBitmap *s_bmp_rot[4];       /* rotated corners: 10,02,08,04 */
static GBitmap *s_bmp_bold_rot[4];
static GBitmap *s_ghost_bmp[13];
static GBitmap *s_ghost_bold_bmp[13];
static GBitmap *s_month_bmp[13];
static GBitmap *s_day_bmp[7];
static GBitmap *s_inscription;
static int      s_hour, s_min;
static int      s_mday, s_mon, s_year, s_wday;
static int      s_battery_pct   = 100;
static int      s_battery_style = 0;
static int      s_hand_style    = 0;
static int      s_bold_style    = 0;
static int      s_bg_style      = 0;
static int      s_corner_rot    = 1;
static int      s_date_bold     = 0;   /* 0=regular, 1=bold date */
static int      s_grid_bold     = 0;   /* 0=regular, 1=bold grid */
static int      s_grid_shade    = 1;   /* 0=light, 1=medium, 2=strong */

#define SETTINGS_KEY 1
typedef struct {
  int battery_style;
  int hand_style;
  int bold_style;
  int bg_style;
  int corner_rot;
  int date_bold;
  int grid_bold;
  int grid_shade;
} Settings;

/* ── Seeded PRNG (xorshift) ───────────────────────────────── */
static uint32_t s_rng = 1;
static void rng_seed(uint32_t s) { s_rng = s ? s : 1; }
static uint32_t rng_next(void) {
  s_rng ^= s_rng << 13;
  s_rng ^= s_rng >> 17;
  s_rng ^= s_rng << 5;
  return s_rng;
}
/* Pick from: black(×3), dark grey(×2), mid grey(×1) */
static GColor pixel_col(void) {
  switch (rng_next() % 6) {
    case 0: case 1: case 2: return GColorFromRGB(26, 22, 16);
    case 3: case 4:         return GColorFromRGB(74, 72, 64);
    default:                return GColorFromRGB(122, 118, 104);
  }
}

/* ── Pixel block hand ─────────────────────────────────────── */
/*  Style 1: each step draws a filled N×N block of random grey pixels */
static void draw_pixel_blocky(GContext *ctx, int32_t angle,
                               int len, int tail, int block) {
  int32_t c = cos_lookup(angle);
  int32_t s = sin_lookup(angle);
  int half = block / 2;
  for (int i = -tail; i <= len; i++) {
    int px = CX + (int)(i * c / TRIG_MAX_RATIO);
    int py = CY + (int)(i * s / TRIG_MAX_RATIO);
    for (int dx = -half; dx <= half; dx++) {
      for (int dy = -half; dy <= half; dy++) {
        graphics_context_set_fill_color(ctx, pixel_col());
        graphics_fill_rect(ctx, GRect(px+dx, py+dy, 1, 1), 0, GCornerNone);
      }
    }
  }
}

/* ── Pixel tapered hand ───────────────────────────────────── */
/*  Style 2: 3px wide at base tapering to 1px at tip */
static void draw_pixel_tapered(GContext *ctx, int32_t angle,
                                int len, int tail) {
  int32_t c  = cos_lookup(angle);
  int32_t s  = sin_lookup(angle);
  int32_t pc = cos_lookup(angle + TRIG_MAX_ANGLE/4);
  int32_t ps = sin_lookup(angle + TRIG_MAX_ANGLE/4);
  for (int i = -tail; i <= len; i++) {
    int px = CX + (int)(i * c / TRIG_MAX_RATIO);
    int py = CY + (int)(i * s / TRIG_MAX_RATIO);
    /* width 3 at tail, 1 at tip */
    float t = (float)(i + tail) / (len + tail);
    int w = (t < 0.5f) ? 1 : (t < 0.8f) ? 2 : 3;
    int half = w / 2;
    for (int dw = -half; dw <= half; dw++) {
      int fx = px + (int)(dw * pc / TRIG_MAX_RATIO);
      int fy = py + (int)(dw * ps / TRIG_MAX_RATIO);
      graphics_context_set_fill_color(ctx, pixel_col());
      graphics_fill_rect(ctx, GRect(fx, fy, 1, 1), 0, GCornerNone);
    }
  }
}

/* ── Literary pixel hands (from Literary Watchface) ──────── */
/*
 * 2×2 pixel squares along the hand path, snapped to 3px grid.
 * Hour hand: green  (#2a7a2a)
 * Minute hand: red  (#aa2020)
 * Same clk_px grid-snap technique as the literary watch.
 */
#define LIT_PX_SZ   2
#define LIT_PX_STEP 3

static void lit_px(GContext *ctx, int x, int y) {
  int gx = ((x - CX + LIT_PX_STEP/2) / LIT_PX_STEP) * LIT_PX_STEP + CX;
  int gy = ((y - CY + LIT_PX_STEP/2) / LIT_PX_STEP) * LIT_PX_STEP + CY;
  graphics_fill_rect(ctx,
    GRect(gx - LIT_PX_SZ/2, gy - LIT_PX_SZ/2, LIT_PX_SZ, LIT_PX_SZ),
    0, GCornerNone);
}

static void draw_literary_hands(GContext *ctx, int32_t h_angle, int32_t m_angle) {
  GColor ink = GColorFromRGB(42, 42, 34);

  /* Minute hand — dark ink */
  graphics_context_set_fill_color(ctx, ink);
  for (int r = LIT_PX_STEP * 2; r <= MIN_LEN; r += LIT_PX_STEP) {
    int x = CX + (int)(sin_lookup(m_angle + TRIG_MAX_ANGLE/4) * r / TRIG_MAX_RATIO);
    int y = CY - (int)(cos_lookup(m_angle + TRIG_MAX_ANGLE/4) * r / TRIG_MAX_RATIO);
    lit_px(ctx, x, y);
  }

  /* Hour hand — dark ink */
  graphics_context_set_fill_color(ctx, ink);
  for (int r = LIT_PX_STEP * 2; r <= HOUR_LEN; r += LIT_PX_STEP) {
    int x = CX + (int)(sin_lookup(h_angle + TRIG_MAX_ANGLE/4) * r / TRIG_MAX_RATIO);
    int y = CY - (int)(cos_lookup(h_angle + TRIG_MAX_ANGLE/4) * r / TRIG_MAX_RATIO);
    lit_px(ctx, x, y);
  }

  /* Centre — solid ink dot */
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_circle(ctx, GPoint(CX, CY), 3);
}
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
 * Single line: <ഞായർ>, 04 <മേയ്>, 26
 * Left-aligned at x=6, y=88
 */
static void draw_date(GContext *ctx) {
  GColor ink = GColorFromRGB(42, 42, 34);
  GFont font = s_date_bold
    ? fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD)
    : fonts_get_system_font(FONT_KEY_GOTHIC_14);
  int x = 6;
  int y = 88;

  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  /* Day of week bitmap */
  GBitmap *dbmp = s_day_bmp[s_wday];
  if (dbmp) {
    GRect bb = gbitmap_get_bounds(dbmp);
    int by = y + (12 - bb.size.h) / 2;
    graphics_draw_bitmap_in_rect(ctx, dbmp, GRect(x, by, bb.size.w, bb.size.h));
    x += bb.size.w;
  }

  /* Comma + space after day */
  graphics_context_set_text_color(ctx, ink);
  graphics_draw_text(ctx, ", ", font, GRect(x, y-2, 14, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  x += 8;

  /* Day number e.g. "04 " */
  char day_str[5];
  snprintf(day_str, sizeof(day_str), "%02d ", s_mday);
  graphics_draw_text(ctx, day_str, font, GRect(x, y-2, 26, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  x += 14;

  /* Month bitmap */
  GBitmap *mbmp = s_month_bmp[s_mon];
  if (mbmp) {
    GRect bb = gbitmap_get_bounds(mbmp);
    int by = y + (12 - bb.size.h) / 2;
    graphics_draw_bitmap_in_rect(ctx, mbmp, GRect(x, by, bb.size.w, bb.size.h));
    x += bb.size.w;
  }

  /* ", YY" */
  char yr_str[6];
  snprintf(yr_str, sizeof(yr_str), ", %02d", s_year % 100);
  graphics_context_set_text_color(ctx, ink);
  graphics_draw_text(ctx, yr_str, font, GRect(x, y-2, 40, 16),
                     GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
}

/* ── Flower battery (Athapookkalam radial fill) ───────────── */
/*
 * Draws the flower in the lower-right of the bottom half.
 * Filled zone = charged (clockwise from top).
 * Drained zone = outlines only, fills removed.
 * Scaled to fit ~72×72px area centered at (108, 126).
 */

#define FL_CX    39   /* left side of clock face */
#define FL_CY    42
#define FL_R     17

/* ── Flower battery (Athapookkalam) — clean small version ─── */
/*
 * Simple 6-petal flower at (FL_CX, FL_CY), radius FL_R.
 * Radial sweep clockwise from top = charged zone (filled).
 * Drained zone = outlines only.
 */
static void draw_flower_battery(GContext *ctx, int pct) {
  GColor ink   = GColorFromRGB(90, 86, 76);
  GColor ghost = GColorFromRGB(200, 196, 184);

  /* Outer ring */
  graphics_context_set_stroke_color(ctx, ink);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, GPoint(FL_CX, FL_CY), FL_R);

  /* Inner ring */
  graphics_draw_circle(ctx, GPoint(FL_CX, FL_CY), FL_R - 3);

  /* 16 tick marks between rings */
  for (int i = 0; i < 16; i++) {
    int32_t a = TRIG_MAX_ANGLE * i / 16;
    int ox = (FL_R)     * cos_lookup(a) / TRIG_MAX_RATIO;
    int oy = (FL_R)     * sin_lookup(a) / TRIG_MAX_RATIO;
    int ix = (FL_R - 3) * cos_lookup(a) / TRIG_MAX_RATIO;
    int iy = (FL_R - 3) * sin_lookup(a) / TRIG_MAX_RATIO;
    graphics_draw_line(ctx,
      GPoint(FL_CX + ox, FL_CY + oy),
      GPoint(FL_CX + ix, FL_CY + iy));
  }

  /* 6 petals — lines from center to tip at r=FL_R-5 */
  int petal_r = FL_R - 5;
  for (int i = 0; i < 6; i++) {
    int32_t a = TRIG_MAX_ANGLE * i / 6 - TRIG_MAX_ANGLE / 4;
    int tx = FL_CX + petal_r * cos_lookup(a) / TRIG_MAX_RATIO;
    int ty = FL_CY + petal_r * sin_lookup(a) / TRIG_MAX_RATIO;

    /* Check if petal is in filled zone (clockwise from top) */
    int32_t norm_a = ((a + TRIG_MAX_ANGLE/4 + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;
    int32_t filled_angle = (int32_t)TRIG_MAX_ANGLE * pct / 100;
    bool in_filled = (norm_a <= filled_angle);

    graphics_context_set_stroke_color(ctx, in_filled ? ink : ghost);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, GPoint(FL_CX, FL_CY), GPoint(tx, ty));
  }

  /* 8 dots around the inner ring */
  for (int i = 0; i < 8; i++) {
    int32_t a = TRIG_MAX_ANGLE * i / 8 - TRIG_MAX_ANGLE / 4;
    int32_t norm_a = ((a + TRIG_MAX_ANGLE/4 + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;
    int32_t filled_angle = (int32_t)TRIG_MAX_ANGLE * pct / 100;
    bool in_filled = (norm_a <= filled_angle);

    int dot_r = FL_R - 6;
    int dx = FL_CX + dot_r * cos_lookup(a) / TRIG_MAX_RATIO;
    int dy = FL_CY + dot_r * sin_lookup(a) / TRIG_MAX_RATIO;
    graphics_context_set_fill_color(ctx, in_filled ? ink : ghost);
    graphics_fill_circle(ctx, GPoint(dx, dy), 1);
  }

  /* Center dot */
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_circle(ctx, GPoint(FL_CX, FL_CY), 2);
}

/* ── AppMessage handler ───────────────────────────────────── */
static void inbox_dropped(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Msg dropped: %d", (int)reason);
}

static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *t;
  /* BATTERY_STYLE=0, BG_STYLE=1, BOLD_STYLE=2, CORNER_ROT=3,
     DATE_BOLD=4, GRID_BOLD=5, GRID_SHADE=6, HAND_STYLE=7 */
  if ((t = dict_find(iter, 0))) s_battery_style = (int)t->value->int32;
  if ((t = dict_find(iter, 1))) s_bg_style      = (int)t->value->int32;
  if ((t = dict_find(iter, 2))) s_bold_style    = (int)t->value->int32;
  if ((t = dict_find(iter, 3))) s_corner_rot    = (int)t->value->int32;
  if ((t = dict_find(iter, 4))) s_date_bold     = (int)t->value->int32;
  if ((t = dict_find(iter, 5))) s_grid_bold     = (int)t->value->int32;
  if ((t = dict_find(iter, 6))) s_grid_shade    = (int)t->value->int32;
  if ((t = dict_find(iter, 7))) s_hand_style    = (int)t->value->int32;

  Settings s = { s_battery_style, s_hand_style, s_bold_style, s_bg_style,
                 s_corner_rot, s_date_bold, s_grid_bold, s_grid_shade };
  persist_write_data(SETTINGS_KEY, &s, sizeof(s));
  layer_mark_dirty(s_canvas);
}

/* ── Spiral drawing ───────────────────────────────────────── */
static void draw_spiral(GContext *ctx, int pct) {
  const int32_t total_trig  = (int32_t)SP_TURNS * TRIG_MAX_ANGLE;
  const int32_t filled_steps = (int32_t)SP_STEPS * pct / 100;
  const int32_t offset = -TRIG_MAX_ANGLE / 4;

  GColor ink   = GColorFromRGB(90, 86, 76);
  GColor ghost = GColorFromRGB(225, 221, 212);

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

/* ── Square spiral battery (Aldo style — exact copy) ──────── */
static const GPoint s_sq_spiral[] = {
  {6,6},{38,6},{38,38},{6,38},{6,10},
  {10,10},{34,10},{34,34},{10,34},{10,14},
  {14,14},{30,14},{30,30},{14,30},{14,18},
  {18,18},{26,18},{26,26},{18,26},{18,22},
  {22,22}
};
#define SQ_SPIRAL_LEN 21

static void draw_square_spiral(GContext *ctx, int pct) {
  GColor ink   = GColorFromRGB(90, 86, 76);
  GColor ghost = GColorFromRGB(225, 221, 212);

  /* Offset Aldo spiral (center ~22,22) to bottom-left watermark */
  int ox = SP_CX - 22;
  int oy = SP_CY - 22;

  /* Total path length */
  int total_len = 0;
  for (int i = 0; i < SQ_SPIRAL_LEN - 1; i++) {
    int dx = s_sq_spiral[i+1].x - s_sq_spiral[i].x;
    int dy = s_sq_spiral[i+1].y - s_sq_spiral[i].y;
    total_len += (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
  }

  int filled = total_len * pct / 100;
  int drawn  = 0;

  graphics_context_set_stroke_width(ctx, 2);

  for (int i = 0; i < SQ_SPIRAL_LEN - 1; i++) {
    GPoint p0 = GPoint(s_sq_spiral[i].x   + ox, s_sq_spiral[i].y   + oy);
    GPoint p1 = GPoint(s_sq_spiral[i+1].x + ox, s_sq_spiral[i+1].y + oy);
    int dx  = p1.x - p0.x;
    int dy  = p1.y - p0.y;
    int seg = (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);

    if (drawn + seg <= filled) {
      graphics_context_set_stroke_color(ctx, ink);
      graphics_draw_line(ctx, p0, p1);
      drawn += seg;
    } else if (drawn < filled) {
      int rem = filled - drawn;
      GPoint mid = GPoint(p0.x + dx * rem / seg, p0.y + dy * rem / seg);
      graphics_context_set_stroke_color(ctx, ink);
      graphics_draw_line(ctx, p0, mid);
      graphics_context_set_stroke_color(ctx, ghost);
      graphics_draw_line(ctx, mid, p1);
      drawn = filled;
    } else {
      graphics_context_set_stroke_color(ctx, ghost);
      graphics_draw_line(ctx, p0, p1);
    }
  }
}

/* ── Standard battery bar (no % text) ────────────────────── */
static void draw_pct_battery(GContext *ctx, int pct) {
  GColor ink = GColorFromRGB(90, 86, 76);

  /* Standard bar on clock face left — centered around SP_CX, SP_CY */
  const int BX = SP_CX - 14, BY = SP_CY + 16, BW = 28, BH = 7, TIP = 4;

  /* Outline */
  graphics_context_set_stroke_color(ctx, ink);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_rect(ctx, GRect(BX, BY, BW, BH));

  /* Tip */
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_rect(ctx, GRect(BX+BW+1, BY+BH/2-TIP/2, 3, TIP), 0, GCornerNone);

  /* Fill bar */
  int fill_w = (BW - 2) * pct / 100;
  if (fill_w > 0) {
    GColor fill = (pct <= 20) ? GColorFromRGB(120, 20, 20) : ink;
    graphics_context_set_fill_color(ctx, fill);
    graphics_fill_rect(ctx, GRect(BX+1, BY+1, fill_w, BH-2), 0, GCornerNone);
  }
}

/* ── Canvas draw ──────────────────────────────────────────── */

/*
 * Fixed numeral positions (bitmap centers) for 144×84 face with 5px padding.
 * Face: x=5..139, y=5..79  (FW=134, FH=74)
 * CX=72, CY=42
 * Top row    y=14:   10(18) 11(51) 12(72) 01(102) 02(126)  [evenly spaced]
 * Bottom row y=70:   08(18) 07(51) 06(72) 05(102) 04(126)
 * Left       x=17:   09  y=42
 * Right      x=127:  03  y=42
 */
static const int16_t NUM_X[13] = { 0, 104, 128, 130, 128, 104, 72, 50, 16, 14, 16, 46, 72 };
static const int16_t NUM_Y[13] = { 0,  12,  12,  42,  72,  72, 72, 72, 72, 42, 12, 12, 12 };

static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GColor cream;
  if      (s_bg_style == 1) cream = GColorWhite;
  else if (s_bg_style == 2) cream = GColorLightGray;
  else                      cream = GColorFromRGB(240, 236, 224);
  GColor ink = GColorFromRGB(42, 42, 34);

  /* Cream background */
  graphics_context_set_fill_color(ctx, cream);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  /* 60 tick marks along face perimeter (with 5px padding) */
  /* Face perimeter: FX=5,FY=5,FW=134,FH=74, PERIM=2*(134+74)=416 */
  #define FX2   5
  #define FY2   5
  #define FW2  134
  #define FH2   74
  #define PERIM2 416
  #define TCD2   67   /* FW2/2 = top center distance */

  /* 60 tick marks — corner hour ticks rotated 45° inward */
  /* Corner hour ticks: i=10(2), i=20(4), i=40(8), i=50(10) */
  for (int i = 0; i < 60; i++) {
    int32_t d = TCD2 + (int32_t)i * PERIM2 / 60;
    d = ((d % PERIM2) + PERIM2) % PERIM2;
    int16_t px, py; int8_t edge;
    if (d < FW2)                { px=FX2+d;              py=FY2;              edge=0; }
    else if (d < FW2+FH2)       { px=FX2+FW2;            py=FY2+d-FW2;        edge=1; }
    else if (d < 2*FW2+FH2)     { px=FX2+FW2-(d-FW2-FH2); py=FY2+FH2;        edge=2; }
    else                        { px=FX2;                py=FY2+FH2-(d-2*FW2-FH2); edge=3; }

    bool major = (i % 5 == 0);
    int tlen = major ? 5 : 2;
    GColor tc = major ? GColorFromRGB(80,76,64) : GColorFromRGB(160,154,136);
    graphics_context_set_stroke_color(ctx, tc);
    graphics_context_set_stroke_width(ctx, 1);

    /* Corner ticks: draw diagonally into the corner */
    if (i == 10 || i == 20 || i == 40 || i == 50) {
      /* Determine diagonal direction based on which corner */
      int ddx = (i == 10 || i == 50) ? 0 : 0; /* computed below */
      int ddy = 0;
      if      (i == 50) { ddx =  tlen; ddy =  tlen; } /* top-left → down-right */
      else if (i == 10) { ddx = -tlen; ddy =  tlen; } /* top-right → down-left */
      else if (i == 20) { ddx = -tlen; ddy = -tlen; } /* bottom-right → up-left */
      else              { ddx =  tlen; ddy = -tlen; } /* bottom-left → up-right */
      graphics_draw_line(ctx, GPoint(px, py), GPoint(px+ddx, py+ddy));
    } else {
      GPoint outer = GPoint(px, py);
      GPoint inner;
      if      (edge==0) inner = GPoint(px, py+tlen);
      else if (edge==1) inner = GPoint(px-tlen, py);
      else if (edge==2) inner = GPoint(px, py-tlen);
      else              inner = GPoint(px+tlen, py);
      graphics_draw_line(ctx, outer, inner);
    }
  }

  /* Numeral bitmaps — fixed positions, bold or regular per setting */
  /* Corner numerals (10,02,08,04) use pre-rotated 45° bitmaps */
  GBitmap **active_bmp     = (s_bold_style == 1) ? s_bmp_bold     : s_bmp;
  GBitmap **active_bmp_rot = (s_bold_style == 1) ? s_bmp_bold_rot : s_bmp_rot;
  graphics_context_set_compositing_mode(ctx, GCompOpSet);

  /* Map hour → rotated bitmap index: 10→0, 2→1, 8→2, 4→3 */
  for (int h = 1; h <= 12; h++) {
    GBitmap *bmp = NULL;
    if (s_corner_rot == 1) {
      if      (h == 10) bmp = active_bmp_rot[0];
      else if (h ==  2) bmp = active_bmp_rot[1];
      else if (h ==  8) bmp = active_bmp_rot[2];
      else if (h ==  4) bmp = active_bmp_rot[3];
      else              bmp = active_bmp[h];
    } else {
      bmp = active_bmp[h];
    }
    if (!bmp) continue;
    GRect bb = gbitmap_get_bounds(bmp);
    int x = NUM_X[h] - bb.size.w / 2;
    int y = NUM_Y[h] - bb.size.h / 2;
    graphics_draw_bitmap_in_rect(ctx, bmp, GRect(x, y, bb.size.w, bb.size.h));
  }

  /* Hands */
  int32_t h_angle =
    (TRIG_MAX_ANGLE * ((s_hour % 12) * 60 + s_min)) / (12 * 60)
    - TRIG_MAX_ANGLE / 4;
  int32_t m_angle =
    (TRIG_MAX_ANGLE * s_min) / 60
    - TRIG_MAX_ANGLE / 4;

  /* Seed RNG from time so pattern is stable per minute */
  rng_seed((uint32_t)(s_hour * 60 + s_min + 1));

  if (s_hand_style == 0) {
    /* Smooth line */
    graphics_context_set_stroke_color(ctx, ink);
    draw_hand(ctx, h_angle, HOUR_LEN, HOUR_TAIL, HOUR_W);
    draw_hand(ctx, m_angle, MIN_LEN,  MIN_TAIL,  MIN_W);
  } else if (s_hand_style == 1) {
    /* Pixel blocky — 3×3 hour, 2×2 minute */
    draw_pixel_blocky(ctx, h_angle, HOUR_LEN, HOUR_TAIL, 3);
    draw_pixel_blocky(ctx, m_angle, MIN_LEN,  MIN_TAIL,  2);
  } else if (s_hand_style == 2) {
    /* Pixel tapered */
    draw_pixel_tapered(ctx, h_angle, HOUR_LEN, HOUR_TAIL);
    draw_pixel_tapered(ctx, m_angle, MIN_LEN,  MIN_TAIL);
  } else {
    /* Style 3 — Literary pixel (green hour, red minute, grid-snapped) */
    draw_literary_hands(ctx, h_angle, m_angle);
  }

  /* Center cap */
  graphics_context_set_fill_color(ctx, ink);
  graphics_fill_circle(ctx, GPoint(CX, CY), 3);

  /* Divider */
  graphics_context_set_stroke_color(ctx, GColorFromRGB(180,174,158));
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_line(ctx, GPoint(0, 84), GPoint(144, 84));

  /* Date strip */
  draw_date(ctx);

  /* Battery — always bottom-left watermark */
  switch (s_battery_style) {
    case 1:  draw_flower_battery(ctx, s_battery_pct); break;
    case 2:  draw_square_spiral(ctx, s_battery_pct);  break;
    case 3:  draw_pct_battery(ctx, s_battery_pct);    break;
    default: draw_spiral(ctx, s_battery_pct);         break;
  }

  /* Ghost numeral grid 4×3 — regular or bold bitmaps, shade via passes */
  {
    GBitmap **gbmps = s_grid_bold ? s_ghost_bold_bmp : s_ghost_bmp;
    int passes = s_grid_shade + 1;
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    for (int i = 0; i < 12; i++) {
      if (!gbmps[i+1]) continue;
      int x = (i % 4) * 36;
      int y = 103 + (i / 4) * 14;
      GRect bb = gbitmap_get_bounds(gbmps[i+1]);
      for (int p = 0; p < passes; p++)
        graphics_draw_bitmap_in_rect(ctx, gbmps[i+1], GRect(x, y, bb.size.w, bb.size.h));
    }
  }

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
    s_bmp_bold[i] = gbitmap_create_with_resource(NUM_BOLD_RES[i]);
  for (int i = 0; i < 4; i++)
    s_bmp_rot[i] = gbitmap_create_with_resource(NUM_ROT_RES[i]);
  for (int i = 0; i < 4; i++)
    s_bmp_bold_rot[i] = gbitmap_create_with_resource(NUM_BOLD_ROT_RES[i]);
  for (int i = 1; i <= 12; i++)
    s_ghost_bmp[i] = gbitmap_create_with_resource(GHOST_RES[i]);
  for (int i = 1; i <= 12; i++)
    s_ghost_bold_bmp[i] = gbitmap_create_with_resource(GHOST_BOLD_RES[i]);
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
    if (s_bmp[i])            gbitmap_destroy(s_bmp[i]);
    if (s_bmp_bold[i])       gbitmap_destroy(s_bmp_bold[i]);
    if (s_ghost_bmp[i])      gbitmap_destroy(s_ghost_bmp[i]);
    if (s_ghost_bold_bmp[i]) gbitmap_destroy(s_ghost_bold_bmp[i]);
    if (s_month_bmp[i])      gbitmap_destroy(s_month_bmp[i]);
  }
  for (int i = 0; i < 4; i++) {
    if (s_bmp_rot[i])      gbitmap_destroy(s_bmp_rot[i]);
    if (s_bmp_bold_rot[i]) gbitmap_destroy(s_bmp_bold_rot[i]);
  }
  for (int i = 0; i < 7; i++)
    if (s_day_bmp[i]) gbitmap_destroy(s_day_bmp[i]);
  if (s_inscription) gbitmap_destroy(s_inscription);
}

/* ── App entry ────────────────────────────────────────────── */
static void init(void) {
  /* Load persisted settings */
  Settings s;
  if (persist_read_data(SETTINGS_KEY, &s, sizeof(s)) == sizeof(s)) {
    s_battery_style = s.battery_style;
    s_hand_style    = s.hand_style;
    s_bold_style    = s.bold_style;
    s_bg_style      = s.bg_style;
    s_corner_rot    = s.corner_rot;
    s_date_bold     = s.date_bold;
    s_grid_bold     = s.grid_bold;
    s_grid_shade    = s.grid_shade;
  }

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);

  /* AppMessage — open before register, matching noise watch pattern */
  app_message_open(64, 64);
  app_message_register_inbox_received(inbox_received);
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
