#include <pebble.h>
#include "app_config.h"

enum {
  KEY_TEMPERATURE = 0,
  KEY_CONDITIONS = 1
};

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer, *s_weather_layer;
static GFont s_time_font, s_date_font, s_weather_font;
static BitmapLayer *s_background_layer, *s_bt_icon_layer;
static GBitmap *s_background_bitmap, *s_bt_icon_bitmap;
static Layer *s_battery_layer;
static int s_battery_level;

/* Function prototype */
static void bluetooth_callback(bool connected);

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "00:00";

  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);

  // Copy date into buffer from tm structure
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);

  // Show the date
  text_layer_set_text(s_date_layer, date_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 114.0F);

  // Draw the background
  graphics_context_set_fill_color(ctx, STYLE_BATTERY_BG_COL);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, STYLE_BATTERY_COL);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void main_window_load(Window *window) {
  // Create GBitmap, then set to created BitmapLayer
  s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
  s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
  bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));

  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);

  // Create the BitmapLayer to display the GBitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(STYLE_BT_POS_X, STYLE_BT_POS_Y,
                                              STYLE_BT_SIZE_X, STYLE_BT_SIZE_Y));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
  // Show the correct state of the BT connection from the start
  bluetooth_callback(bluetooth_connection_service_peek());

  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(STYLE_TIME_POS_X, STYLE_TIME_POS_Y,
                                         STYLE_TIME_SIZE_X, STYLE_TIME_SIZE_Y));
  text_layer_set_background_color(s_time_layer, STYLE_TIME_BG_COL);
  text_layer_set_text_color(s_time_layer, STYLE_TIME_TXT_COL);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  s_time_font = fonts_load_custom_font(resource_get_handle(STYLE_TIME_FONT));
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text(s_time_layer, "00:00");
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));

  // Create date TextLayer
  s_date_layer = text_layer_create(GRect(STYLE_DATE_POS_X, STYLE_DATE_POS_Y,
                                         STYLE_DATE_SIZE_X, STYLE_DATE_SIZE_Y));
  text_layer_set_background_color(s_date_layer, STYLE_DATE_BG_COL);
  text_layer_set_text_color(s_date_layer, STYLE_DATE_TXT_COL);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  s_date_font = fonts_load_custom_font(resource_get_handle(STYLE_DATE_FONT));
  text_layer_set_font(s_date_layer, s_date_font);
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_date_layer));

  // Create weather Layer
  s_weather_layer = text_layer_create(GRect(STYLE_WEATHER_POS_X, STYLE_WEATHER_POS_Y,
                                           STYLE_WEATHER_SIZE_X, STYLE_WEATHER_SIZE_Y));
  text_layer_set_background_color(s_weather_layer, STYLE_WEATHER_BG_COL);
  text_layer_set_text_color(s_weather_layer, STYLE_WEATHER_TXT_COL);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");
  s_weather_font = fonts_load_custom_font(resource_get_handle(STYLE_WEATHER_FONT));
  text_layer_set_font(s_weather_layer, s_weather_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));

  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(STYLE_BATTERY_POS_X, STYLE_BATTERY_POS_Y,
                                       STYLE_BATTERY_SIZE_X, STYLE_BATTERY_SIZE_Y));
  layer_set_update_proc(s_battery_layer, battery_update_proc);

  // Add to Window
  layer_add_child(window_get_root_layer(window), s_battery_layer);

  // Make sure the time is displayed from the start
  update_time();
}

static void main_window_unload(Window *window) {
  // Destroy time elements
  fonts_unload_custom_font(s_time_font);
  text_layer_destroy(s_time_layer);
  // Destroy date elements
  fonts_unload_custom_font(s_date_font);
  text_layer_destroy(s_date_layer);
  // Destroy weather elements
  fonts_unload_custom_font(s_weather_font);
  text_layer_destroy(s_weather_layer);
  // Destroy Battery
  layer_destroy(s_battery_layer);
  // Destroy Bluetooth
  gbitmap_destroy(s_bt_icon_bitmap);
  bitmap_layer_destroy(s_bt_icon_layer);
  // Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
  // Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();

  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];

  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);

  if(!connected) {
    // Issue a vibrating alert
    vibes_double_pulse();
  }
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Register for battery level updates
  battery_state_service_subscribe(battery_callback);
  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  // Register for Bluetooth connection updates
  bluetooth_connection_service_subscribe(bluetooth_callback);

  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
