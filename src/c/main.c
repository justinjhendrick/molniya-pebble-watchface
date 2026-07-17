#include <pebble.h>
#include "utils.h"

#define BUFFER_LEN (10)
#define DEBUG_TIME (false)
#define DEBUG_BBOX (false)

static Window* s_window;
static Layer* s_layer;
static char s_buffer[BUFFER_LEN];
static GFont s_font_lg = NULL;
static GFont s_font_md = NULL;
static const GColor s_color_background = GColorBlack;
static const GColor s_color_dial = GColorWhite;
static const GColor s_color_major_tick = GColorBlack;
static const GColor s_color_minor_tick = GColorBlack;
static const GColor s_color_hour = GColorBlack;
static const GColor s_color_minute = GColorWhite;

static void debug_bbox(GContext* ctx, GRect bbox) {
  if (DEBUG_BBOX) {
    graphics_context_set_stroke_color(ctx, GColorRed);
    graphics_context_set_stroke_width(ctx, 1);
    graphics_draw_rect(ctx, bbox);
  }
}

static void draw_ticks(GContext* ctx, GPoint center, int dial_radius, struct tm* now) {
  graphics_context_set_fill_color(ctx, s_color_dial);
  graphics_fill_circle(ctx, center, dial_radius);
  for (int16_t tick_minute = 0; tick_minute < 60; tick_minute += 1) {
    int tick_deg = tick_minute * 360 / 60;
    GPoint minute_tick_outer = cartesian_from_polar(center, dial_radius, tick_deg);
    int tick_length = 1;
    if (tick_minute == now->tm_min) {
      graphics_context_set_stroke_color(ctx, s_color_major_tick);
      graphics_context_set_stroke_width(ctx, 9);
      tick_length = dial_radius * 7 / 20;
    } else if (tick_minute % 15 == 0) {
      graphics_context_set_stroke_color(ctx, s_color_major_tick);
      graphics_context_set_stroke_width(ctx, 5);
      tick_length = dial_radius * 2 / 10;
    } else if (tick_minute % 5 == 0) {
      graphics_context_set_stroke_color(ctx, s_color_minor_tick);
      graphics_context_set_stroke_width(ctx, 3);
      tick_length = dial_radius * 2 / 10;
    } else {
      graphics_context_set_stroke_color(ctx, s_color_minor_tick);
      graphics_context_set_stroke_width(ctx, 1);
      tick_length = dial_radius / 10;
    }
    GPoint minute_tick_inner = cartesian_from_polar(center, dial_radius - tick_length, tick_deg);
    graphics_draw_line(ctx, minute_tick_inner, minute_tick_outer);
  }
}

static void draw_hour(GContext* ctx, GPoint center, int dial_radius, struct tm* now) {
  GSize hour_bbox_size = GSize(100, 100);
  GFont hour_font = s_font_lg;
  GRect hour_bbox = rect_from_midpoint(center, hour_bbox_size);
  debug_bbox(ctx, hour_bbox);

  format_hour(now, s_buffer, BUFFER_LEN);
  graphics_context_set_text_color(ctx, s_color_hour);
  graphics_draw_text(ctx, s_buffer, hour_font, hour_bbox, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void draw_minute(GContext* ctx, GPoint center, int dial_radius, struct tm* now) {
  int minute_deg = 360 * now->tm_min / 60;
  GSize minute_bbox_size = GSize(100, 60);
  GFont minute_font = s_font_md;
  GPoint minute_bbox_midpoint = cartesian_from_polar(center, dial_radius * 27 / 20, minute_deg);
  GRect minute_bbox = rect_from_midpoint(minute_bbox_midpoint, minute_bbox_size);
  debug_bbox(ctx, minute_bbox);

  snprintf(s_buffer, BUFFER_LEN, "%d", now->tm_min);
  graphics_context_set_text_color(ctx, s_color_minute);
  graphics_draw_text(ctx, s_buffer, minute_font, minute_bbox, GTextOverflowModeFill, GTextAlignmentCenter, NULL);
}

static void update_layer(Layer* layer, GContext* ctx) {
  time_t temp = time(NULL);
  struct tm* now = localtime(&temp);
  if (DEBUG_TIME) {
    fast_forward_time(now);
  }
  GRect bounds = layer_get_unobstructed_bounds(layer);
  graphics_context_set_fill_color(ctx, s_color_background);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  int dial_radius = min(bounds.size.h, bounds.size.w) * 10 / 20;
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
  window_set_background_color(s_window, s_color_background);
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

static void init(void) {
  s_font_lg = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ANTONIO_90));
  s_font_md = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_ANTONIO_55));

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
  if (s_font_lg) fonts_unload_custom_font(s_font_lg);
  if (s_font_md) fonts_unload_custom_font(s_font_md);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
