#pragma once
#include <pebble.h>

static int min(int a, int b) {
  if (a < b) {
    return a;
  }
  return b;
}

static GPoint cartesian_from_polar(GPoint center, int radius, int angle_deg) {
  GPoint ret = {
    .x = (int16_t)(sin_lookup(DEG_TO_TRIGANGLE(angle_deg)) * radius / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(DEG_TO_TRIGANGLE(angle_deg)) * radius / TRIG_MAX_RATIO) + center.y,
  };
  return ret;
}

static GRect rect_from_midpoint(GPoint midpoint, GSize size) {
  GRect ret;
  ret.origin.x = midpoint.x - size.w / 2;
  ret.origin.y = midpoint.y - size.h / 2;
  ret.size = size;
  return ret;
}

static void format_hour(struct tm* now, char* buf, int len) {
  int hour = now->tm_hour;
  if (!clock_is_24h_style()) {
    hour = now->tm_hour % 12;
    if (hour == 0) {
      hour = 12;
    }
  }
  snprintf(buf, len, "%d", hour);
}

static void format_minute(struct tm* now, char* buf, int len) {
  strftime(buf, len, "%M", now);
}
