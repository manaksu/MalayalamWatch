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

#define SP_CX       39
#define SP_CY       42
#define SP_MAX_R    17
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

static const uint32_t GHOST_RES[13] = {
  0,
  RESOURCE_ID_GHOST_01, RESOURCE_ID_GHOST_02, RESOURCE_ID_GHOST_03,
  RESOURCE_ID_GHOST_04, RESOURCE_ID_GHOST_05, RESOURCE_ID_GHOST_06,
  RESOURCE_ID_GHOST_07, RESOURCE_ID_GHOST_08, RESOURCE_ID_GHOST_09,
  RESOURCE_ID_GHOST_10, RESOURCE_ID_GHOST_11, RESOURCE_ID_GHOST_12,
};

static Window  *s_window;
static Layer   *s_canvas;
static GBitmap *s_bmp[13];
static GBitmap *s_ghost_bmp[13];
static GBitmap *s_month_bmp[13];
static GBitmap *s_day_bmp[7];
static GBitmap *s_inscription;
static int      s_hour, s_min;
static int      s_mday, s_mon, s_year, s_wday;
static int      s_battery_pct   = 100;
static int      s_battery_style = 0;   /* 0=spiral, 1=flower radial */
static int      s_hand_style    = 0;   /* 0=smooth, 1=blocky, 2=tapered */

#define SETTINGS_KEY 1
typedef struct {
  int battery_style;
  int hand_style;
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

/* ── Hand drawing (smooth) ────────────────────────────────── */
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
  GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14);
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

#define FL_CX    108   /* flower center x */
#define FL_CY    126   /* flower center y */
#define FL_R      34   /* outer radius px */
#define FL_STEPS  60   /* arc resolution  */

/* Scale a reference value (designed at r=300) to FL_R */
#define FLS(v) ((v) * FL_R / 300)

static void draw_flower_segment(GContext *ctx, bool filled,
                                int32_t a_start, int32_t a_end) {
  /* Draw flower elements, clipped to the arc sector a_start..a_end
   * (Pebble trig angles, TRIG_MAX_ANGLE = full circle)
   * filled=true  → draw with fills (charged zone)
   * filled=false → outlines only (drained zone)
   */

  /* We approximate the clip by drawing individual elements
   * and checking if their center angle falls in range.
   * For continuous shapes (rings, petals) we use GPath clipping
   * via the graphics clip rect — simplified: draw all, rely on
   * the layer clip rect set by the caller. */

  GColor ink    = GColorFromRGB(42, 42, 34);
  GColor c_dark = GColorFromRGB(42, 42, 32);
  GColor c_mid  = GColorFromRGB(122, 118, 104);
  GColor c_band = GColorFromRGB(216, 212, 200);
  GColor cream  = GColorFromRGB(240, 236, 224);

  /* Outer ring */
  graphics_context_set_stroke_color(ctx, ink);
  graphics_context_set_stroke_width(ctx, FLS(5) < 1 ? 1 : FLS(5));
  if (filled) {
    graphics_context_set_fill_color(ctx, GColorFromRGB(74, 72, 64));
    graphics_fill_circle(ctx, GPoint(FL_CX, FL_CY), FLS(300));
  }
  graphics_draw_circle(ctx, GPoint(FL_CX, FL_CY), FLS(300));

  /* Inner ring — cream fill */
  if (filled) {
    graphics_context_set_fill_color(ctx, cream);
    graphics_fill_circle(ctx, GPoint(FL_CX, FL_CY), FLS(276));
  }
  graphics_context_set_stroke_width(ctx, FLS(3) < 1 ? 1 : FLS(3));
  graphics_draw_circle(ctx, GPoint(FL_CX, FL_CY), FLS(276));

  /* Dot band */
  if (filled) {
    graphics_context_set_fill_color(ctx, c_band);
    graphics_fill_circle(ctx, GPoint(FL_CX, FL_CY), FLS(230));
    graphics_context_set_fill_color(ctx, cream);
    graphics_fill_circle(ctx, GPoint(FL_CX, FL_CY), FLS(158));
  }

  /* 6 petals */
  for (int i = 0; i < 6; i++) {
    int32_t pangle = (TRIG_MAX_ANGLE * i) / 6;
    int32_t pc = cos_lookup(pangle - TRIG_MAX_ANGLE/4);
    int32_t ps = sin_lookup(pangle - TRIG_MAX_ANGLE/4);
    GPoint tip = {
      FL_CX + FLS(118) * pc / TRIG_MAX_RATIO,
      FL_CY + FLS(118) * ps / TRIG_MAX_RATIO
    };
    /* Draw petal as ellipse approximation — just a line for Pebble simplicity */
    GColor pc_col = (i % 2 == 0) ? c_dark : c_mid;
    if (filled) {
      graphics_context_set_stroke_color(ctx, pc_col);
    } else {
      graphics_context_set_stroke_color(ctx, ink);
    }
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_line(ctx, GPoint(FL_CX, FL_CY), tip);
  }

  /* 16 dots */
  const GColor dot_cols[4] = {
    GColorFromRGB(42,42,32), GColorFromRGB(122,118,104),
    GColorFromRGB(184,180,168), GColorFromRGB(232,228,216)
  };
  for (int i = 0; i < 16; i++) {
    int32_t dangle = (TRIG_MAX_ANGLE * i) / 16 - TRIG_MAX_ANGLE/4;
    int32_t dc = cos_lookup(dangle);
    int32_t ds = sin_lookup(dangle);
    GPoint dp = {
      FL_CX + FLS(195) * dc / TRIG_MAX_RATIO,
      FL_CY + FLS(195) * ds / TRIG_MAX_RATIO
    };
    if (filled) {
      graphics_context_set_fill_color(ctx, dot_cols[i % 4]);
      graphics_fill_circle(ctx, dp, FLS(16));
    }
    graphics_context_set_stroke_color(ctx, ink);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_circle(ctx, dp, FLS(16));
  }

  /* Ticks */
  for (int i = 0; i < 16; i++) {
    int32_t tangle = (TRIG_MAX_ANGLE * i) / 16 - TRIG_MAX_ANGLE/4;
    int32_t tc2 = cos_lookup(tangle);
    int32_t ts  = sin_lookup(tangle);
    GPoint outer = {
      FL_CX + FLS(276) * tc2 / TRIG_MAX_RATIO,
      FL_CY + FLS(276) * ts  / TRIG_MAX_RATIO
    };
    GPoint inner = {
      FL_CX + FLS(252) * tc2 / TRIG_MAX_RATIO,
      FL_CY + FLS(252) * ts  / TRIG_MAX_RATIO
    };
    graphics_context_set_stroke_color(ctx, ink);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, outer, inner);
  }

  /* Center */
  if (filled) {
    graphics_context_set_fill_color(ctx, c_dark);
    graphics_fill_circle(ctx, GPoint(FL_CX, FL_CY), FLS(18));
  }
  graphics_context_set_stroke_color(ctx, ink);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, GPoint(FL_CX, FL_CY), FLS(18));
}

static void draw_flower_battery(GContext *ctx, int pct) {
  /* Clip to filled sector (top, clockwise = charged) */
  /* Filled angle in trig units */
  int32_t filled_trig = (int32_t)TRIG_MAX_ANGLE * pct / 100;
  int32_t start_trig  = -TRIG_MAX_ANGLE / 4;  /* 12 o'clock */

  /* Step through FL_STEPS arc segments.
   * For each segment, check if it falls in filled or drained zone.
   * We approximate by drawing the whole flower twice with GRect clip. */

  /* Pass 1: filled zone — clip rect approximation via full draw + mask */
  /* Pass 2: outline zone */
  /* Pebble doesn't support arbitrary clip paths, so we use a simpler
   * approach: draw filled, then overdraw outline-only in the drained sector
   * using cream fills to erase the existing fills. */

  /* Draw full filled flower first */
  draw_flower_segment(ctx, true, 0, TRIG_MAX_ANGLE);

  if (pct < 100) {
    /* Overdraw drained sector: fill shapes with cream, then redraw outlines */
    GColor cream = GColorFromRGB(240, 236, 224);
    GColor ink   = GColorFromRGB(42, 42, 34);
    GColor c_band= GColorFromRGB(216, 212, 200);

    /* We simulate clip by drawing covering rects in the drained sector.
     * Using a polygon approximation of the pie sector. */
    int32_t drain_start = start_trig + filled_trig;
    int32_t drain_end   = start_trig + TRIG_MAX_ANGLE;

    /* Fill the drained pie sector with cream to erase fills */
    graphics_context_set_fill_color(ctx, cream);
    /* Draw pie sector as triangle fan */
    for (int s = 0; s < FL_STEPS; s++) {
      int32_t a0 = drain_start + (drain_end - drain_start) * s     / FL_STEPS;
      int32_t a1 = drain_start + (drain_end - drain_start) * (s+1) / FL_STEPS;
      GPoint p0 = {
        FL_CX + (FL_R+4) * cos_lookup(a0) / TRIG_MAX_RATIO,
        FL_CY + (FL_R+4) * sin_lookup(a0) / TRIG_MAX_RATIO
      };
      GPoint p1 = {
        FL_CX + (FL_R+4) * cos_lookup(a1) / TRIG_MAX_RATIO,
        FL_CY + (FL_R+4) * sin_lookup(a1) / TRIG_MAX_RATIO
      };
      /* Fill triangle: center, p0, p1 */
      GPoint tri[3] = { GPoint(FL_CX, FL_CY), p0, p1 };
      GPath *path = gpath_create(&(GPathInfo){ .num_points=3, .points=tri });
      gpath_draw_filled(ctx, path);
      gpath_destroy(path);
    }

    /* Redraw outlines in the drained sector without fills */
    for (int i = 0; i < 16; i++) {
      int32_t dangle = (TRIG_MAX_ANGLE * i) / 16 - TRIG_MAX_ANGLE/4;
      /* Check if dot is in drained zone */
      int32_t norm = ((dangle - drain_start) % TRIG_MAX_ANGLE + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;
      int32_t range= ((drain_end - drain_start) % TRIG_MAX_ANGLE + TRIG_MAX_ANGLE) % TRIG_MAX_ANGLE;
      if (norm <= range) {
        int32_t dc = cos_lookup(dangle), ds = sin_lookup(dangle);
        GPoint dp = {
          FL_CX + FLS(195)*dc/TRIG_MAX_RATIO,
          FL_CY + FLS(195)*ds/TRIG_MAX_RATIO
        };
        graphics_context_set_stroke_color(ctx, ink);
        graphics_context_set_stroke_width(ctx, 1);
        graphics_draw_circle(ctx, dp, FLS(16));
      }
    }

    /* Redraw ring outlines */
    graphics_context_set_stroke_color(ctx, ink);
    graphics_context_set_stroke_width(ctx, 2);
    graphics_draw_circle(ctx, GPoint(FL_CX, FL_CY), FLS(300));
    graphics_draw_circle(ctx, GPoint(FL_CX, FL_CY), FLS(276));
    graphics_draw_circle(ctx, GPoint(FL_CX, FL_CY), FLS(18));
  }
}

/* ── AppMessage handler ───────────────────────────────────── */
static void inbox_received(DictionaryIterator *iter, void *context) {
  Tuple *batt_t = dict_find(iter, MESSAGE_KEY_BATTERY_STYLE);
  if (batt_t) s_battery_style = (int)batt_t->value->int32;

  Tuple *hand_t = dict_find(iter, MESSAGE_KEY_HAND_STYLE);
  if (hand_t) s_hand_style = (int)hand_t->value->int32;

  Settings s = { .battery_style = s_battery_style,
                 .hand_style    = s_hand_style };
  persist_write_data(SETTINGS_KEY, &s, sizeof(s));
  layer_mark_dirty(s_canvas);
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

/*
 * Fixed numeral positions (bitmap centers) for 144×84 face with 5px padding.
 * Face: x=5..139, y=5..79  (FW=134, FH=74)
 * CX=72, CY=42
 * Top row    y=14:   10(18) 11(51) 12(72) 01(102) 02(126)  [evenly spaced]
 * Bottom row y=70:   08(18) 07(51) 06(72) 05(102) 04(126)
 * Left       x=17:   09  y=42
 * Right      x=127:  03  y=42
 */
static const int16_t NUM_X[13] = { 0, 102, 126, 127, 126, 102, 72, 51, 18, 17, 18, 51, 72 };
static const int16_t NUM_Y[13] = { 0,  14,  14,  42,  70,  70, 70, 70, 70, 42, 14, 14, 14 };

static void canvas_draw(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GColor cream = GColorFromRGB(240, 236, 224);
  GColor ink   = GColorFromRGB(42, 42, 34);

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

  for (int i = 0; i < 60; i++) {
    int32_t d = TCD2 + (int32_t)i * PERIM2 / 60;
    d = ((d % PERIM2) + PERIM2) % PERIM2;
    int16_t px, py; int8_t edge;
    if (d < FW2)                { px=FX2+d;       py=FY2;       edge=0; }
    else if (d < FW2+FH2)       { px=FX2+FW2;     py=FY2+d-FW2; edge=1; }
    else if (d < 2*FW2+FH2)     { px=FX2+FW2-(d-FW2-FH2); py=FY2+FH2; edge=2; }
    else                        { px=FX2; py=FY2+FH2-(d-2*FW2-FH2); edge=3; }

    int tlen = (i % 5 == 0) ? 4 : 2;
    GPoint outer = GPoint(px, py);
    GPoint inner;
    if      (edge==0) inner = GPoint(px, py+tlen);
    else if (edge==1) inner = GPoint(px-tlen, py);
    else if (edge==2) inner = GPoint(px, py-tlen);
    else              inner = GPoint(px+tlen, py);

    graphics_context_set_stroke_color(ctx,
      (i%5==0) ? GColorFromRGB(80,76,64) : GColorFromRGB(160,154,136));
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_line(ctx, outer, inner);
  }

  /* Numeral bitmaps — fixed positions */
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  for (int h = 1; h <= 12; h++) {
    if (!s_bmp[h]) continue;
    GRect bb = gbitmap_get_bounds(s_bmp[h]);
    int x = NUM_X[h] - bb.size.w / 2;
    int y = NUM_Y[h] - bb.size.h / 2;
    graphics_draw_bitmap_in_rect(ctx, s_bmp[h],
                                 GRect(x, y, bb.size.w, bb.size.h));
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
  } else {
    /* Pixel tapered */
    draw_pixel_tapered(ctx, h_angle, HOUR_LEN, HOUR_TAIL);
    draw_pixel_tapered(ctx, m_angle, MIN_LEN,  MIN_TAIL);
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

  /* Battery indicator — style chosen by user */
  if (s_battery_style == 1) {
    draw_flower_battery(ctx, s_battery_pct);
  } else {
    draw_spiral(ctx, s_battery_pct);
  }

  /* Ghost numeral grid 4×3 — y=100..148
   * Each ghost bitmap is exactly 36×16px (cell sized)
   * Contains ML numeral + Arabic reference, pre-rendered light grey
   */
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  for (int i = 0; i < 12; i++) {
    if (!s_ghost_bmp[i+1]) continue;
    int col = i % 4;
    int row = i / 4;
    int x = col * 36;
    int y = 103 + row * 14;
    GRect bb = gbitmap_get_bounds(s_ghost_bmp[i+1]);
    graphics_draw_bitmap_in_rect(ctx, s_ghost_bmp[i+1], GRect(x, y, bb.size.w, bb.size.h));
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
    s_ghost_bmp[i] = gbitmap_create_with_resource(GHOST_RES[i]);
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
    if (s_ghost_bmp[i]) gbitmap_destroy(s_ghost_bmp[i]);
    if (s_month_bmp[i]) gbitmap_destroy(s_month_bmp[i]);
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
  }

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers){
    .load   = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);

  /* AppMessage for settings */
  app_message_register_inbox_received(inbox_received);
  app_message_open(64, 64);
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
