#include "pebble.h"

// This is a custom defined key for saving our count field
#define NUM_AM_PKEY 8
#define NUM_PM_PKEY 8
#define NUM_WORKTIME_PKEY 25
#define NUM_SMALL_BREAK_PKEY 5
#define NUM_BIG_BREAK_PKEY 60
  
#define NUM_POMODORO_TIME 5
#define NUM_POMODORO_ITERATIONS 1

// You can define defaults for values in persistent storage
#define NUM_AM_DEFAULT 8
#define NUM_PM_DEFAULT 8
#define NUM_WORKTIME_DEFAULT 25
#define NUM_SMALL_BREAK_DEFAULT 5

static Window *window;
static Window *window_settings;

static GBitmap *action_icon_plus;
static GBitmap *action_icon_minus;

static ActionBarLayer *action_bar;
static ActionBarLayer *main_action_bar;

static TextLayer *header_text_layer;
static TextLayer *body_text_layer;
static TextLayer *label_text_layer;

static TextLayer *main_header_text_layer;
static TextLayer *main_body_text_layer;
static TextLayer *main_label_text_layer;

// We'll save the count in memory from persistent storage
static int wizard_pointer = 0;
static int num_worktime = NUM_WORKTIME_DEFAULT;
static int num_small_break = NUM_SMALL_BREAK_DEFAULT;

// COUNTDOWN CHIZZLE
static AppTimer *timer;
static const uint16_t timer_interval_ms = 100;
static const uint32_t HOURS_KEY = 1 << 1;
static const uint32_t MINUTES_KEY = 1 << 2;
static const uint32_t SECONDS_KEY = 1 << 3;
static const uint32_t LONG_VIBES_KEY = 1 << 4;
static const uint32_t SINGLE_VIBES_KEY = 1 << 5;
static const uint32_t DOUBLE_VIBES_KEY = 1 << 6;
int minutes;
int seconds;
char *single_vibes;
char *double_vibes;
char *long_vibes;
char *default_long_vibes;
char *default_single_vibes;
char *default_double_vibes;
char *time_key;
char *long_key;
char *single_key;
char *double_key;
int running;
int working = 1;

static void display_time(int minutes, int seconds) {
  static char body_text[6];
  snprintf(body_text, sizeof(body_text), "%02d:%02d", minutes, seconds);
  text_layer_set_text(main_body_text_layer, body_text);
}

static void show_time(void) {
  static char body_text[6];
  snprintf(body_text, sizeof(body_text), "%02d:%02d", minutes, seconds);
  text_layer_set_text(main_body_text_layer, body_text);
/*  text_layer_set_text(body_text_layer, body_text);
  if (strstr(long_vibes, time_test)) {
    vibes_long_pulse();
  }
  if (strstr(single_vibes, time_test)) {
    vibes_short_pulse();
  }
  if (strstr(double_vibes, time_test)) {
    vibes_double_pulse();
  }*/
}

static void updateLabel() {
  if (working) {
    text_layer_set_text(main_label_text_layer, "Working");
  } else {
    text_layer_set_text(main_label_text_layer, "Break");
  }
}

static void reset(void) {
  if (working) {
    working = 0;
    minutes = num_small_break;
    vibes_short_pulse();
  } else {
    working = 1;
    minutes = num_worktime;
    vibes_long_pulse();
  }
  seconds = 0;
  updateLabel();
  display_time(minutes, seconds);
}

static void timer_callback(void *data) {
  seconds--;
  if (seconds < 0) {
    minutes--;
    seconds = 59;
  }
  
  if (minutes < 0) {
    reset();
  }
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  display_time(minutes, seconds);
}

static void init_variables() {
  num_worktime = persist_exists(NUM_WORKTIME_PKEY) ? persist_read_int(NUM_WORKTIME_PKEY) : NUM_WORKTIME_DEFAULT;
  num_small_break = persist_exists(NUM_SMALL_BREAK_PKEY) ? persist_read_int(NUM_SMALL_BREAK_PKEY) : NUM_SMALL_BREAK_DEFAULT;
  minutes = num_worktime;
  seconds = 0;
}

static void update_text(int value) {
  static char body_text[5];
  snprintf(body_text, sizeof(body_text), "%u", value);
  text_layer_set_text(body_text_layer, body_text); 
}

static void window_unload(Window *window) {
  text_layer_destroy(header_text_layer);
  text_layer_destroy(body_text_layer);
  text_layer_destroy(label_text_layer);

  action_bar_layer_destroy(action_bar);
}

static void window_unload_main(Window *window) {
  text_layer_destroy(main_header_text_layer);
  text_layer_destroy(main_body_text_layer);
  text_layer_destroy(main_label_text_layer);

  action_bar_layer_destroy(main_action_bar);
}

/** SMALL BREAK SCREEN */
static void increment_ch_small_break(ClickRecognizerRef recognizer, void *context) {
  if (num_small_break >= 15) {
    return;
  }
  
  num_small_break += 1;
  update_text(num_small_break);
}

static void decrement_ch_small_break(ClickRecognizerRef recognizer, void *context) {
  if (num_small_break <= 1) {
    return;
  }

  num_small_break -= 1;
  update_text(num_small_break);
}

static void persist_ch_small_break(ClickRecognizerRef recognizer, void *context) {
  persist_write_int(NUM_SMALL_BREAK_PKEY, num_small_break);
  window_stack_pop(true);
  if (!running) {
    init_variables();
    display_time(num_worktime, 0);
  }
  
}

static void click_config_provider_small_break(void *context) {
  const uint16_t repeat_interval_ms = 50;
  window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, (ClickHandler) increment_ch_small_break);
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, repeat_interval_ms, (ClickHandler) persist_ch_small_break);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, (ClickHandler) decrement_ch_small_break);
}

static void window_load_small_break(Window *me) {
  action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar, me);
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider_small_break);

  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, action_icon_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, action_icon_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, action_icon_minus);

  Layer *layer = window_get_root_layer(me);
  const int16_t width = layer_get_frame(layer).size.w - ACTION_BAR_WIDTH - 3;

  header_text_layer = text_layer_create(GRect(4, 0, width, 60));
  text_layer_set_font(header_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(header_text_layer, GColorClear);
  text_layer_set_text(header_text_layer, "Set time small break");
  layer_add_child(layer, text_layer_get_layer(header_text_layer));

  body_text_layer = text_layer_create(GRect(4, 44, width, 60));
  text_layer_set_font(body_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(body_text_layer, GColorClear);
  layer_add_child(layer, text_layer_get_layer(body_text_layer));
  
  static char body_text[5];
  snprintf(body_text, sizeof(body_text), "%u", num_small_break);
  text_layer_set_text(body_text_layer, body_text);
}


static void window_init_small_break() { 
  window_stack_pop(true);
  window_destroy(window_settings);
  
  window_settings = window_create();
  window_set_window_handlers(window_settings, (WindowHandlers) {
    .load = window_load_small_break,
    .unload = window_unload,
  });
  
  window_stack_push(window_settings, true /* Animated */);
}

/** WORKTIME SCREEN */
static void increment_ch_worktime(ClickRecognizerRef recognizer, void *context) {
  if (num_worktime >= 60) {
    return;
  }
  num_worktime += 5;
  update_text(num_worktime);
}

static void decrement_ch_worktime(ClickRecognizerRef recognizer, void *context) {
  if (num_worktime <= 5) {
    return;
  }

  num_worktime -= 5;
  update_text(num_worktime);
}

static void persist_ch_worktime(ClickRecognizerRef recognizer, void *context) {
  persist_write_int(NUM_WORKTIME_PKEY, num_worktime);
  window_init_small_break();
}

static void click_config_provider_worktime(void *context) {
  const uint16_t repeat_interval_ms = 50;
  window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, (ClickHandler) increment_ch_worktime);
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, repeat_interval_ms, (ClickHandler) persist_ch_worktime);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, (ClickHandler) decrement_ch_worktime);
}

static void window_load_worktime(Window *me) {
  action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(action_bar, me);
  action_bar_layer_set_click_config_provider(action_bar, click_config_provider_worktime);

  action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, action_icon_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, action_icon_plus);
  action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, action_icon_minus);

  Layer *layer = window_get_root_layer(me);
  const int16_t width = layer_get_frame(layer).size.w - ACTION_BAR_WIDTH - 3;

  header_text_layer = text_layer_create(GRect(4, 0, width, 60));
  text_layer_set_font(header_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(header_text_layer, GColorClear);
  text_layer_set_text(header_text_layer, "Set worktime");
  layer_add_child(layer, text_layer_get_layer(header_text_layer));

  body_text_layer = text_layer_create(GRect(4, 44, width, 60));
  text_layer_set_font(body_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(body_text_layer, GColorClear);
  layer_add_child(layer, text_layer_get_layer(body_text_layer));
  
  static char body_text[5];
  snprintf(body_text, sizeof(body_text), "%u", num_worktime);
  text_layer_set_text(body_text_layer, body_text);
}


static void window_init_worktime() {
  window_settings = window_create();
  window_set_window_handlers(window_settings, (WindowHandlers) {
    .load = window_load_worktime,
    .unload = window_unload,
  });
  
  window_stack_push(window_settings, true /* Animated */);
}

/* MAIN SCREEN */
static void countdown(ClickRecognizerRef recognizer, void *context) {
  if ((running) || ((minutes == 0) && (seconds == 0))) {
   app_timer_cancel(timer);
   running = 0;
   return;
  }
  else {
    running = 1;
  }
  timer = app_timer_register(timer_interval_ms, timer_callback, NULL);
  updateLabel();
  show_time();
}

static void persist_ch_main(ClickRecognizerRef recognizer, void *context) {
  window_init_worktime();
}

static void resetPomodoro(ClickRecognizerRef recognizer, void *context) {
  if (running) {
    app_timer_cancel(timer);
  }
  running = 0;
  minutes = num_worktime;
  seconds = 0;
  working = 1;
  text_layer_set_text(main_label_text_layer, "");
  show_time();
}

static void click_config_provider_main(void *context) {
  const uint16_t repeat_interval_ms = 50;
  window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, (ClickHandler) countdown);
  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, repeat_interval_ms, (ClickHandler) persist_ch_main);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, (ClickHandler) resetPomodoro);
}

static void window_main(Window *me) {
  main_action_bar = action_bar_layer_create();
  action_bar_layer_add_to_window(main_action_bar, me);
  action_bar_layer_set_click_config_provider(main_action_bar, click_config_provider_main);

  action_bar_layer_set_icon(main_action_bar, BUTTON_ID_UP, action_icon_plus);
  action_bar_layer_set_icon(main_action_bar, BUTTON_ID_SELECT, action_icon_plus);
  action_bar_layer_set_icon(main_action_bar, BUTTON_ID_DOWN, action_icon_minus);

  Layer *layer = window_get_root_layer(me);
  const int16_t width = layer_get_frame(layer).size.w - ACTION_BAR_WIDTH - 3;

  header_text_layer = text_layer_create(GRect(4, 0, width, 60));
  text_layer_set_font(header_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_background_color(header_text_layer, GColorClear);
  text_layer_set_text(header_text_layer, "Foomodoro");
  layer_add_child(layer, text_layer_get_layer(header_text_layer));

  main_body_text_layer = text_layer_create(GRect(4, 44, width, 60));
  text_layer_set_font(main_body_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_background_color(main_body_text_layer, GColorClear);
  layer_add_child(layer, text_layer_get_layer(main_body_text_layer));
  
  main_label_text_layer = text_layer_create(GRect(4, 44 + 28, width, 60));
  text_layer_set_font(main_label_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_background_color(main_label_text_layer, GColorClear);
  layer_add_child(layer, text_layer_get_layer(main_label_text_layer));
  
  display_time(num_worktime, 0);
}

static void init(void) {
  action_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
  action_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);
  
  init_variables();
  
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_main,
    .unload = window_unload_main,
  });

  window_stack_push(window, true /* Animated */);
}

static void deinit(void) {
  // Save the count into persistent storage on app exit

  window_destroy(window);
  window_destroy(window_settings);

  gbitmap_destroy(action_icon_plus);
  gbitmap_destroy(action_icon_minus);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
