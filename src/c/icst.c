#include "icst.h"

static void bluetooth_callback(bool connected){
	// Hide icon if connected
	layer_set_hidden(bitmap_layer_get_layer(s_bt_layer), connected);

	// Issue a vibrating alert
	if(!connected){
		vibes_double_pulse();
	}
}

// Update display elements
static void update_display(){
	// Invert colors if true
	if(settings.invert) layer_set_hidden(effect_layer_get_layer(s_invert_layer), false);
	else layer_set_hidden(effect_layer_get_layer(s_invert_layer), true);
}

// Read settings from persistent storage
static void load_settings(){
	// Load the default settings
	settings.invert = false;

	// Read settings from persistent storage, if they exists
	persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

// Save the settings to persistent storage
static void save_settings(){
	persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
	
	// Update the display based on new settings
	update_display();
}

// AppMessage receive handler
static void inbox_received_handler(DictionaryIterator *it, void *ctx){
	// Get settings
	Tuple *invert_t = dict_find(it, MESSAGE_KEY_invert);
	if(invert_t) settings.invert = invert_t->value->int32 == 1;

	save_settings();
}

static void battery_callback(BatteryChargeState state){
	// Record the new battery level
	s_battery_level = state.charge_percent;

	// Update meter based on percentage
	bitmap_layer_set_compositing_mode(s_bat_layer, GCompOpSet);

	if(s_battery_level <= 20)
		bitmap_layer_set_bitmap(s_bat_layer, s_bat20_bmap);
	else if(s_battery_level <= 40)
		bitmap_layer_set_bitmap(s_bat_layer, s_bat40_bmap);
	else if(s_battery_level <= 60)
		bitmap_layer_set_bitmap(s_bat_layer, s_bat60_bmap);
	else if(s_battery_level <= 75)
		bitmap_layer_set_bitmap(s_bat_layer, s_bat80_bmap);
	else if(s_battery_level <= 100)
		bitmap_layer_set_bitmap(s_bat_layer, s_bat100_bmap);

	// APP_LOG(APP_LOG_LEVEL_DEBUG, "BATLVL: %d", s_battery_level);
}

static void update_time(){
	// Get a tm structure
	time_t temp = time(NULL); 
	struct tm *tick_time = localtime(&temp);

	static char s_week_buffer[4];
	static char s_date_buffer[6];
	static char s_hour_buffer[3];
	static char s_minute_buffer[3];

	// Write the current info into a buffer
	strftime(s_week_buffer, sizeof(s_week_buffer),"%a", tick_time);
	strftime(s_date_buffer, sizeof(s_date_buffer),"%d/%m", tick_time);
	strftime(s_hour_buffer, sizeof(s_hour_buffer), clock_is_24h_style() ? "%H" : "%I", tick_time);
	strftime(s_minute_buffer, sizeof(s_minute_buffer),"%M", tick_time);

	// Display current info on the TextLayer
	text_layer_set_text(s_week_layer, s_week_buffer);
	text_layer_set_text(s_date_layer, s_date_buffer);
	text_layer_set_text(s_hour_layer, s_hour_buffer);
	text_layer_set_text(s_minute_layer, s_minute_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
	update_time();
}

static void set_bg_layer(BitmapLayer *layer, GBitmap *map){
	Layer *window = window_get_root_layer(s_main_window);

	bitmap_layer_set_compositing_mode(layer, GCompOpSet);
	bitmap_layer_set_bitmap(layer, map);
	
	layer_add_child(window, bitmap_layer_get_layer(layer));
}

static void set_time_layer(TextLayer *layer, GFont font){
	Layer *window = window_get_root_layer(s_main_window);
	
	text_layer_set_background_color(layer, GColorBlack);
	text_layer_set_text_color(layer, GColorWhite);
	text_layer_set_font(layer, font);
	text_layer_set_text_alignment(layer, GTextAlignmentCenter);
	
	layer_add_child(window, text_layer_get_layer(layer));
}

static void main_window_load(Window *window){
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	// Time layer coordinates
	int sizeX = 44;
	int sizeY = 48;
	int rightBorderX = bounds.size.w - sizeX;
	int rightMinuteBorderY = bounds.size.h / 2;
	int rightHourBorderY = bounds.size.h / 2 - sizeY;

	// Create GFont
	s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_48));
	s_week_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_28));

	// Create GBitmap
	s_bg_bmap = gbitmap_create_with_resource(RESOURCE_ID_LOGO_WHITE);
	s_bat100_bmap = gbitmap_create_with_resource(RESOURCE_ID_BAT100);
	s_bat80_bmap = gbitmap_create_with_resource(RESOURCE_ID_BAT80);
	s_bat60_bmap = gbitmap_create_with_resource(RESOURCE_ID_BAT60);
	s_bat40_bmap = gbitmap_create_with_resource(RESOURCE_ID_BAT40);
	s_bat20_bmap = gbitmap_create_with_resource(RESOURCE_ID_BAT20);
	s_bt_bmap = gbitmap_create_with_resource(RESOURCE_ID_BT);

	// Create BitmapLayer to display the GBitmap
	GRect bg = GRect(0, 0, bounds.size.w - sizeX, bounds.size.h);
	s_bg_layer = bitmap_layer_create(bg);
	s_bat_layer = bitmap_layer_create(bg);
	s_bt_layer = bitmap_layer_create(
		GRect(rightBorderX, rightMinuteBorderY + sizeY + 3, sizeX, 30));

	// Create the TextLayer with specific bounds
	s_date_layer = text_layer_create(
		GRect(0, -20, 100, sizeY));
	s_week_layer = text_layer_create(
		GRect(rightBorderX - 2, -3, sizeX, sizeY - 20));
	s_minute_layer = text_layer_create(
		GRect(rightBorderX, rightMinuteBorderY, sizeX, sizeY));
	s_hour_layer = text_layer_create(
		GRect(rightBorderX, rightHourBorderY, sizeX, sizeY));
	s_invert_layer = effect_layer_create(
		GRect(0, 0, 144, 168));

	// Set the bitmap onto the layer and add to the window
	set_bg_layer(s_bg_layer, s_bg_bmap);
	set_bg_layer(s_bat_layer, s_bat100_bmap);
	set_bg_layer(s_bt_layer, s_bt_bmap);

	// Improve the layout to be more like a watchface
	set_time_layer(s_week_layer, s_week_font);
	set_time_layer(s_date_layer, s_time_font);
	set_time_layer(s_hour_layer, s_time_font);
	set_time_layer(s_minute_layer, s_time_font);

	// Show the correct state of the BT connection from the start
	bluetooth_callback(connection_service_peek_pebble_app_connection());

	// Create layer to invert colors
	effect_layer_add_effect(s_invert_layer, effect_invert_bw_only, NULL);
	layer_add_child(window_layer, effect_layer_get_layer(s_invert_layer));
	
	// Update the display based on settings
	update_display();
}

static void main_window_unload(Window *window){
	// Destroy TextLayer
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_week_layer);
	text_layer_destroy(s_minute_layer);
	text_layer_destroy(s_hour_layer);

	// Unload GFont
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_week_font);

	// Destroy GBitmap
	gbitmap_destroy(s_bg_bmap);
	gbitmap_destroy(s_bat100_bmap);
	gbitmap_destroy(s_bat80_bmap);
	gbitmap_destroy(s_bat60_bmap);
	gbitmap_destroy(s_bat40_bmap);
	gbitmap_destroy(s_bat20_bmap);
	gbitmap_destroy(s_bt_bmap);

	// Destroy BitmapLayer
	bitmap_layer_destroy(s_bg_layer);
	bitmap_layer_destroy(s_bat_layer);
	bitmap_layer_destroy(s_bt_layer);

	// Destroy EffectLayer
	effect_layer_destroy(s_invert_layer);
}

static void init(){
	load_settings();

	// Create main Window element and assign to pointer
	s_main_window = window_create();
	window_set_background_color(s_main_window, GColorBlack);

	// Listen for AppMessages
	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(128, 128);

	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers){
		.load = main_window_load,
		.unload = main_window_unload
	});

	// Show the Window on the watch, with animated=true
	window_stack_push(s_main_window, true);

	// Make sure the time is displayed from the start
	update_time();

	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

	// Register for battery level updates
	battery_state_service_subscribe(battery_callback);

	// Ensure battery level is displayed from the start
	battery_callback(battery_state_service_peek());
 
	// Register for Bluetooth connection updates
	connection_service_subscribe((ConnectionHandlers){
		.pebble_app_connection_handler = bluetooth_callback
	});   
}

static void deinit(){
	// Destroy Window
	window_destroy(s_main_window);
}

int main(void){
	init();
	app_event_loop();
	deinit();
}