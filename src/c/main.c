#include <pebble.h>
#include "utils.h"
#include "fonts.h"

#define BUFFER_LEN (10)
#define DEBUG_TIME (false)
#define DEBUG_BBOX (false)

#if PBL_DISPLAY_WIDTH >= 260
  #define FULL_RADIUS_INSET (16)
#else
  #define FULL_RADIUS_INSET (0)
#endif

static Window* s_window;
static Layer* s_layer;
static char s_buffer[BUFFER_LEN];
static GColor s_color_background = GColorBlack;
static GColor s_color_15m_tick = GColorYellow;
static GColor s_color_5m_tick = GColorWhite;
static GColor s_color_1m_tick = GColorWhite;
static GColor s_color_hour_digit = GColorYellow;
static GColor s_color_minute_digit = GColorCyan;
static GColor s_color_minute_hand_outer = GColorCyan;
static GColor s_color_minute_hand_inner = GColorBlack;

static void debug_bbox(GContext* ctx, GRect bbox) {
  if (DEBUG_BBOX) {
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_rect(ctx, bbox);
  }
}

static void debug_time(struct tm* now) {
#if DEBUG_TIME
  now->tm_min = now->tm_sec;           /* Minutes. [0-59] */
  now->tm_hour = now->tm_sec % 24;     /* Hours.  [0-23] */
  now->tm_mday = now->tm_sec % 31 + 1; /* Day. [1-31] */
  now->tm_mon = now->tm_sec % 12;      /* Month. [0-11] */
  now->tm_wday = now->tm_sec % 7;      /* Day of week. [0-6] */
#endif
#ifdef HOUR_OVERRIDE
  now->tm_hour = HOUR_OVERRIDE;
#endif
#ifdef MINUTE_OVERRIDE
  now->tm_min = MINUTE_OVERRIDE;
#endif
}

static void draw_ticks(GContext* ctx, GPoint center, int dial_radius, struct tm* now) {
  // Lower end of range is with timeline-quick-view
  // Upper end of range is without
  //   Flint  dial_radius is 58-72
  //   Emery  dial_radius is 82-100
  //   Gabbro dial_radius is 72-122

  // All Flint sizes
  // Small size Gabbro
  int width_minute_hand = 7;
  int width_15m_ticks = 5;
  int width_5m_ticks = 3;
  int width_1m_ticks = 1;
  int width_minute_hand_inner = 1;
  if (dial_radius >= 110) {
    // Full size Gabbro only
    width_minute_hand = 11;
    width_15m_ticks = 7;
    width_5m_ticks = 5;
    width_1m_ticks = 3;
    width_minute_hand_inner = 3;
  } else if (dial_radius >= 80) {
    // All Emery sizes
    width_minute_hand = 9;
    width_15m_ticks = 5;
    width_5m_ticks = 3;
    width_1m_ticks = 1;
    width_minute_hand_inner = 1;
  }
  for (int16_t tick_minute = 0; tick_minute < 60; tick_minute += 1) {
    int tick_deg = tick_minute * 360 / 60;
    GPoint minute_tick_outer = cartesian_from_polar(center, dial_radius, tick_deg);
    int tick_length = 1;
    if (tick_minute == now->tm_min) {
      graphics_context_set_stroke_color(ctx, s_color_minute_hand_outer);
      graphics_context_set_stroke_width(ctx, width_minute_hand);
      tick_length = dial_radius * 9 / 20;
    } else if (tick_minute % 15 == 0) {
      graphics_context_set_stroke_color(ctx, s_color_15m_tick);
      graphics_context_set_stroke_width(ctx, width_15m_ticks);
      tick_length = dial_radius * 7 / 20;
    } else if (tick_minute % 5 == 0) {
      graphics_context_set_stroke_color(ctx, s_color_5m_tick);
      graphics_context_set_stroke_width(ctx, width_5m_ticks);
      tick_length = dial_radius * 6 / 20;
    } else {
      graphics_context_set_stroke_color(ctx, s_color_1m_tick);
      graphics_context_set_stroke_width(ctx, width_1m_ticks);
      tick_length = dial_radius * 3 / 20;
    }
    GPoint minute_tick_inner = cartesian_from_polar(center, dial_radius - tick_length, tick_deg);
    graphics_draw_line(ctx, minute_tick_inner, minute_tick_outer);
    if (tick_minute == now->tm_min) {
      graphics_context_set_stroke_color(ctx, s_color_minute_hand_inner);
      graphics_context_set_stroke_width(ctx, width_minute_hand_inner);
      graphics_draw_line(ctx, minute_tick_inner, minute_tick_outer);
    }
  }
}

static void draw_hour(GContext* ctx, GPoint center, int dial_radius, struct tm* now) {
  GSize hour_bbox_size = GSize(dial_radius, dial_radius * 9 / 10);
  GFont hour_font = get_font(dial_radius * 8 / 10);
  GRect hour_bbox = rect_from_midpoint(center, hour_bbox_size);
  debug_bbox(ctx, hour_bbox);

  format_hour(now, s_buffer, BUFFER_LEN);
  graphics_context_set_text_color(ctx, s_color_hour_digit);
  graphics_draw_text(ctx, s_buffer, hour_font, hour_bbox, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void draw_minute(GContext* ctx, GPoint center, int dial_radius, struct tm* now) {
  int minute_deg = 360 * now->tm_min / 60;
  GSize minute_bbox_size = GSize(dial_radius, dial_radius * 6 / 10);
  GFont minute_font = get_font(dial_radius * 52 / 100);
  GPoint minute_bbox_midpoint = cartesian_from_polar(center, dial_radius * 27 / 20, minute_deg);
  GRect minute_bbox = rect_from_midpoint(minute_bbox_midpoint, minute_bbox_size);
  debug_bbox(ctx, minute_bbox);

  format_minute(now, s_buffer, BUFFER_LEN);
  graphics_context_set_text_color(ctx, s_color_minute_digit);
  graphics_draw_text(ctx, s_buffer, minute_font, minute_bbox, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void update_layer(Layer* layer, GContext* ctx) {
  time_t temp = time(NULL);
  struct tm* now = localtime(&temp);
  debug_time(now);
  GRect bounds = layer_get_unobstructed_bounds(layer);
  graphics_context_set_fill_color(ctx, s_color_background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int full_radius = min(bounds.size.h, bounds.size.w) - FULL_RADIUS_INSET;
  int dial_radius = full_radius / 2;
  int minute_deg = 360 * now->tm_min / 60;
  int inverted_minute_deg = 180 + minute_deg;
  GPoint bounds_center = grect_center_point(&bounds);
  GPoint dial_center = cartesian_from_polar(bounds_center, dial_radius * 12 / 20, inverted_minute_deg);

  draw_ticks(ctx, dial_center, dial_radius, now);
  draw_hour(ctx, dial_center, dial_radius, now);
  draw_minute(ctx, dial_center, dial_radius, now);
}

static void window_load(Window* window) {
  Layer* window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  s_layer = layer_create(bounds);
  layer_set_update_proc(s_layer, update_layer);
  layer_add_child(window_layer, s_layer);
}

static void window_unload(Window* window) {
  if (s_layer) layer_destroy(s_layer);
}

static void tick_handler(struct tm* now, TimeUnits units_changed) {
  if (s_layer) layer_mark_dirty(s_layer);
}

static void load_settings() {
  if (persist_exists(MESSAGE_KEY_color_background)) { s_color_background.argb = persist_read_int(MESSAGE_KEY_color_background); }
  if (persist_exists(MESSAGE_KEY_color_15m_tick)) { s_color_15m_tick.argb = persist_read_int(MESSAGE_KEY_color_15m_tick); }
  if (persist_exists(MESSAGE_KEY_color_5m_tick)) { s_color_5m_tick.argb = persist_read_int(MESSAGE_KEY_color_5m_tick); }
  if (persist_exists(MESSAGE_KEY_color_1m_tick)) { s_color_1m_tick.argb = persist_read_int(MESSAGE_KEY_color_1m_tick); }
  if (persist_exists(MESSAGE_KEY_color_hour_digit)) { s_color_hour_digit.argb = persist_read_int(MESSAGE_KEY_color_hour_digit); }
  if (persist_exists(MESSAGE_KEY_color_minute_digit)) { s_color_minute_digit.argb = persist_read_int(MESSAGE_KEY_color_minute_digit); }
  if (persist_exists(MESSAGE_KEY_color_minute_hand_outer)) { s_color_minute_hand_outer.argb = persist_read_int(MESSAGE_KEY_color_minute_hand_outer); }
  if (persist_exists(MESSAGE_KEY_color_minute_hand_inner)) { s_color_minute_hand_inner.argb = persist_read_int(MESSAGE_KEY_color_minute_hand_inner); }
}

static void save_settings() {
  const int version_key = 555;
  // bump this if we need a backwards incompatible settings change.
  const int version_value = 1;
  persist_write_int(version_key, version_value);
  // We depend on the order of the message keys to be stable
  persist_write_int(MESSAGE_KEY_color_background, s_color_background.argb);
  persist_write_int(MESSAGE_KEY_color_15m_tick, s_color_15m_tick.argb);
  persist_write_int(MESSAGE_KEY_color_5m_tick, s_color_5m_tick.argb);
  persist_write_int(MESSAGE_KEY_color_1m_tick, s_color_1m_tick.argb);
  persist_write_int(MESSAGE_KEY_color_hour_digit, s_color_hour_digit.argb);
  persist_write_int(MESSAGE_KEY_color_minute_digit, s_color_minute_digit.argb);
  persist_write_int(MESSAGE_KEY_color_minute_hand_outer, s_color_minute_hand_outer.argb);
  persist_write_int(MESSAGE_KEY_color_minute_hand_inner, s_color_minute_hand_inner.argb);
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *t;
  if ((t = dict_find(iter, MESSAGE_KEY_color_background))) { s_color_background = GColorFromHEX(t->value->int32); }
  if ((t = dict_find(iter, MESSAGE_KEY_color_15m_tick))) { s_color_15m_tick = GColorFromHEX(t->value->int32); }
  if ((t = dict_find(iter, MESSAGE_KEY_color_5m_tick))) { s_color_5m_tick = GColorFromHEX(t->value->int32); }
  if ((t = dict_find(iter, MESSAGE_KEY_color_1m_tick))) { s_color_1m_tick = GColorFromHEX(t->value->int32); }
  if ((t = dict_find(iter, MESSAGE_KEY_color_hour_digit))) { s_color_hour_digit = GColorFromHEX(t->value->int32); }
  if ((t = dict_find(iter, MESSAGE_KEY_color_minute_digit))) { s_color_minute_digit = GColorFromHEX(t->value->int32); }
  if ((t = dict_find(iter, MESSAGE_KEY_color_minute_hand_outer))) { s_color_minute_hand_outer = GColorFromHEX(t->value->int32); }
  if ((t = dict_find(iter, MESSAGE_KEY_color_minute_hand_inner))) { s_color_minute_hand_inner = GColorFromHEX(t->value->int32); }
  save_settings();
  if (s_layer) layer_mark_dirty(s_layer);
}

static void init(void) {
  init_fonts();
  load_settings();
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(1024, 64);

  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(DEBUG_TIME ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  if (s_window) window_destroy(s_window);
  deinit_fonts();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
