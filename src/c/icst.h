#pragma once
#include <pebble.h>
#include <pebble-effect-layer/pebble-effect-layer.h>

// Persistent storage key
#define SETTINGS_KEY 1

static Window *s_main_window;
static TextLayer *s_date_layer, *s_week_layer, *s_hour_layer, *s_minute_layer;
static GFont s_time_font, s_week_font;
static BitmapLayer *s_bg_layer, *s_bat_layer, *s_bt_layer;
static EffectLayer *s_invert_layer;
static GBitmap *s_bg_bmap, *s_bat100_bmap, *s_bat80_bmap, *s_bat60_bmap, *s_bat40_bmap, *s_bat20_bmap, *s_bt_bmap;
static int s_battery_level;

typedef struct ClaySettings{
	bool invert;
} ClaySettings;

static ClaySettings settings;

static void bluetooth_callback(bool connected);
static void update_display();
static void load_settings();
static void save_settings();
static void inbox_received_handler(DictionaryIterator *it, void *ctx);
static void battery_callback(BatteryChargeState state);
static void update_time();
static void tick_handler(struct tm *tick_time, TimeUnits units_changed);
static void set_bg_layer(BitmapLayer *layer, GBitmap *map);
static void set_time_layer(TextLayer *layer, GFont font);
static void main_window_load(Window *window);
static void main_window_unload(Window *window);
static void init();
static void deinit();