#include "fonts.h"

#define SMALLEST_FONT (40)
#define LARGEST_FONT (80)
// BRITTLE HACK
// Assuming fonts are assigned contiguously, starting from 1
#define FIRST_RESOURCE_ID (1)
#define FONT_SKIP (4)
#define NUM_FONTS ((LARGEST_FONT - SMALLEST_FONT) / FONT_SKIP + 1)  // +1 because largest and smallest are both inclusive bounds

static GFont s_fonts[NUM_FONTS];

void init_fonts() {
  for (int resource_id = FIRST_RESOURCE_ID; resource_id <= NUM_FONTS; resource_id++) {
    s_fonts[resource_id - FIRST_RESOURCE_ID] = fonts_load_custom_font(resource_get_handle(resource_id));
  }
}

GFont get_font(int size) {
  int idx = (size - SMALLEST_FONT) / FONT_SKIP;
  int actual_size = SMALLEST_FONT + FONT_SKIP * idx;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "size=%d, idx=%d, actual_size=%d", size, idx, actual_size);
  if (size < SMALLEST_FONT
      || size > LARGEST_FONT
      || idx < 0
      || idx >= NUM_FONTS
    ) {
    return NULL;
  }
  return s_fonts[idx];
}

void deinit_fonts() {
  for (int i = 0; i < NUM_FONTS; i++) {
    if (s_fonts[i] == NULL) {
      continue;
    }
    fonts_unload_custom_font(s_fonts[i]);
  }
}
