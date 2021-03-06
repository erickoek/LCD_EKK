#include "pebble.h"
//#include "gbitmap_colour_manipulator.h"

static Window *window;
static Layer *window_layer;
static uint8_t batteryPercent;
static AppSync sync;
static uint8_t sync_buffer[128];

#define SETTINGS_KEY 99
#define PERSIST_KEY_TEMPERATURE 201
#define PERSIST_KEY_CONDITION 202

int TEMPERATURE, CONDITION, COLOR_SET;
bool Celsius;

typedef struct persist {
    int Blink;
    int Invert;
    int BluetoothVibe;
    int HourlyVibe;

} __attribute__((__packed__)) persist;

persist settings = {
  .Blink = 0,
  .Invert = 0,
  .BluetoothVibe = 1,
  .HourlyVibe = 0
};

static const PropertyAnimationImplementation layer_hidden_implementation = {
  .base = {
    .update = (AnimationUpdateImplementation) property_animation_update_int16,
  },
  .accessors = {
    .setter = { .int16 = (Int16Setter) layer_set_hidden, },
    .getter = { .int16 = (Int16Getter) layer_get_hidden, },
  },
};

static int valueRead, valueWritten;

static bool appStarted = false;

enum {
  BLINK_KEY = 0x0,
  INVERT_KEY = 0x1,
  BLUETOOTHVIBE_KEY = 0x2,
  HOURLYVIBE_KEY = 0x3,
  KEY_REQUEST = 0x4,
  KEY_CONDITION = 0x5,
  KEY_TEMPERATURE = 0x6
};

enum {
  BLINK_OFF = 0,
  BLINK_ON = 1,
  BLINK_DOUBLE_RATE = 2,
};


static GBitmap *background_image;
static BitmapLayer *background_layer;

static GBitmap *separator_image;
static BitmapLayer *separator_layer;
static PropertyAnimation *separator_animation;

static GBitmap *bluetooth_image;
static BitmapLayer *bluetooth_layer;

static GBitmap *battery_image;
static BitmapLayer *battery_image_layer;
static BitmapLayer *battery_layer;

static GBitmap *time_format_image;
static BitmapLayer *time_format_layer;

static GBitmap *day_name_image;
static BitmapLayer *day_name_layer;

static GBitmap *month_name_image;
static BitmapLayer *month_name_layer;

static Layer *big_time_layer;

const int DAY_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DAY_NAME_SUN,
  RESOURCE_ID_IMAGE_DAY_NAME_MON,
  RESOURCE_ID_IMAGE_DAY_NAME_TUE,
  RESOURCE_ID_IMAGE_DAY_NAME_WED,
  RESOURCE_ID_IMAGE_DAY_NAME_THU,
  RESOURCE_ID_IMAGE_DAY_NAME_FRI,
  RESOURCE_ID_IMAGE_DAY_NAME_SAT
};

const int MONTH_NAME_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_MON_NAME_JAN,
  RESOURCE_ID_IMAGE_MON_NAME_FEB,
  RESOURCE_ID_IMAGE_MON_NAME_MAR,
  RESOURCE_ID_IMAGE_MON_NAME_APR,
  RESOURCE_ID_IMAGE_MON_NAME_MAY,
  RESOURCE_ID_IMAGE_MON_NAME_JUN,
  RESOURCE_ID_IMAGE_MON_NAME_JUL,
  RESOURCE_ID_IMAGE_MON_NAME_AUG,
  RESOURCE_ID_IMAGE_MON_NAME_SEP,
  RESOURCE_ID_IMAGE_MON_NAME_OKT,
  RESOURCE_ID_IMAGE_MON_NAME_NOV,
  RESOURCE_ID_IMAGE_MON_NAME_DEC
};

#define TOTAL_DATE_DIGITS 2 
static GBitmap *date_digits_images[TOTAL_DATE_DIGITS];
static BitmapLayer *date_digits_layers[TOTAL_DATE_DIGITS];

const int DATENUM_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9
};

#define TOTAL_STEPS_DIGITS 5 
static GBitmap *steps_digits_images[TOTAL_STEPS_DIGITS];
static BitmapLayer *steps_digits_layers[TOTAL_STEPS_DIGITS];

const char STEPS_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9,
  RESOURCE_ID_IMAGE_STEPS
};

#define TOTAL_TEMP_DIGITS 5 
static GBitmap *temp_digits_images[TOTAL_TEMP_DIGITS];
static BitmapLayer *temp_digits_layers[TOTAL_TEMP_DIGITS];

const char TEMP_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_DATENUM_0,
  RESOURCE_ID_IMAGE_DATENUM_1,
  RESOURCE_ID_IMAGE_DATENUM_2,
  RESOURCE_ID_IMAGE_DATENUM_3,
  RESOURCE_ID_IMAGE_DATENUM_4,
  RESOURCE_ID_IMAGE_DATENUM_5,
  RESOURCE_ID_IMAGE_DATENUM_6,
  RESOURCE_ID_IMAGE_DATENUM_7,
  RESOURCE_ID_IMAGE_DATENUM_8,
  RESOURCE_ID_IMAGE_DATENUM_9,
  RESOURCE_ID_IMAGE_TEMP,
  RESOURCE_ID_IMAGE_CELSIUS,
  RESOURCE_ID_IMAGE_MINUS,
  RESOURCE_ID_IMAGE_DEGREE,
  RESOURCE_ID_IMAGE_C,
  RESOURCE_ID_IMAGE_F
};

static GBitmap *condition_image;
static BitmapLayer *condition_layer;

const int CONDITION_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_SUNNY,
  RESOURCE_ID_IMAGE_SCATTERED_CLOUDS,
  RESOURCE_ID_IMAGE_BROKEN_CLOUDS,
  RESOURCE_ID_IMAGE_FEW_CLOUDS,
  RESOURCE_ID_IMAGE_MIST,
  RESOURCE_ID_IMAGE_RAIN,
  RESOURCE_ID_IMAGE_SHOWER_RAIN,
  RESOURCE_ID_IMAGE_SNOW,
  RESOURCE_ID_IMAGE_THUNDERSTORM,
  RESOURCE_ID_IMAGE_MOON,
  RESOURCE_ID_IMAGE_FEWCLOUDS_NIGHT,
  RESOURCE_ID_IMAGE_RAIN_NIGHT,
  RESOURCE_ID_IMAGE_UNKNOWN
};

#define TOTAL_TIME_DIGITS 4
static GBitmap *time_digits_images[TOTAL_TIME_DIGITS];
static BitmapLayer *time_digits_layers[TOTAL_TIME_DIGITS];

const int BIG_DIGIT_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_NUM_0,
  RESOURCE_ID_IMAGE_NUM_1,
  RESOURCE_ID_IMAGE_NUM_2,
  RESOURCE_ID_IMAGE_NUM_3,
  RESOURCE_ID_IMAGE_NUM_4,
  RESOURCE_ID_IMAGE_NUM_5,
  RESOURCE_ID_IMAGE_NUM_6,
  RESOURCE_ID_IMAGE_NUM_7,
  RESOURCE_ID_IMAGE_NUM_8,
  RESOURCE_ID_IMAGE_NUM_9
};

#define TOTAL_WEEK_DIGITS 3
static GBitmap *week_images[TOTAL_WEEK_DIGITS];
static BitmapLayer *week_layers[TOTAL_WEEK_DIGITS];

const int WEEK_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_TINY_0,
  RESOURCE_ID_IMAGE_TINY_1,
  RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3,
  RESOURCE_ID_IMAGE_TINY_4,
  RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6,
  RESOURCE_ID_IMAGE_TINY_7,
  RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9,
  RESOURCE_ID_IMAGE_TINY_WK
};

#define TOTAL_BATTERY_PERCENT_DIGITS 3
static GBitmap *battery_percent_image[TOTAL_BATTERY_PERCENT_DIGITS];
static BitmapLayer *battery_percent_layers[TOTAL_BATTERY_PERCENT_DIGITS];

const int TINY_IMAGE_RESOURCE_IDS[] = {
  RESOURCE_ID_IMAGE_TINY_0,
  RESOURCE_ID_IMAGE_TINY_1,
  RESOURCE_ID_IMAGE_TINY_2,
  RESOURCE_ID_IMAGE_TINY_3,
  RESOURCE_ID_IMAGE_TINY_4,
  RESOURCE_ID_IMAGE_TINY_5,
  RESOURCE_ID_IMAGE_TINY_6,
  RESOURCE_ID_IMAGE_TINY_7,
  RESOURCE_ID_IMAGE_TINY_8,
  RESOURCE_ID_IMAGE_TINY_9,
  RESOURCE_ID_IMAGE_TINY_PERCENT
};
#define palette_num 2
#ifdef PBL_COLOR
uint8_t my_palettes[2* palette_num]= {
    GColorBlackARGB8, GColorWhiteARGB8,      
    GColorWhiteARGB8, GColorBlackARGB8
};
#endif
enum {COLOUR_USER, COLOUR_BW };

GBitmap* gbitmap_create_with_palette(uint8_t palette, uint32_t resource){

  GBitmap*  bitmap = gbitmap_create_with_resource(resource);
  #ifdef PBL_COLOR
    GColor * colors= gbitmap_get_palette(bitmap);
    if (palette<palette_num){
      colors[0].argb= my_palettes[2* palette];
      colors[1].argb= my_palettes[2* palette+1];
    }
  #endif
  return bitmap;
}

void change_background() {
  gbitmap_destroy(background_image);
  if(settings.Invert) {
    COLOR_SET = COLOUR_BW;
     APP_LOG(APP_LOG_LEVEL_DEBUG, "COLOR_SET = %d ", COLOR_SET);
    background_image = gbitmap_create_with_palette(COLOUR_BW, RESOURCE_ID_IMAGE_BACKGROUND_INVERT);
 }
 else {
   COLOR_SET = COLOUR_USER;
   APP_LOG(APP_LOG_LEVEL_DEBUG, "COLOR_SET = %d ", COLOR_SET);
    background_image = gbitmap_create_with_palette(COLOUR_USER, RESOURCE_ID_IMAGE_BACKGROUND);
  }  
  
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_mark_dirty(bitmap_layer_get_layer(background_layer));
}

void change_battery_icon(bool charging) {
  gbitmap_destroy(battery_image);
  if(charging) {
    battery_image = gbitmap_create_with_palette(COLOR_SET, RESOURCE_ID_IMAGE_BATTERY_CHARGE);
  }
  else {
    battery_image = gbitmap_create_with_palette(COLOR_SET, RESOURCE_ID_IMAGE_BATTERY);
  }  

  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  layer_mark_dirty(bitmap_layer_get_layer(battery_image_layer));
}

void refresh_weather()
  {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Refresh weather");
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    dict_write_uint32(iter, 8, 0);
    app_message_outbox_send();
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed);

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case BLINK_KEY:
      settings.Blink = new_tuple->value->uint8;
      tick_timer_service_unsubscribe();
      if(settings.Blink) {
        tick_timer_service_subscribe(SECOND_UNIT, handle_tick);
      }
      else {
        tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
        layer_set_hidden(bitmap_layer_get_layer(separator_layer), false);
      }
      break;
    case INVERT_KEY:
      settings.Invert = new_tuple->value->uint8;
      change_background();
      break;
    case BLUETOOTHVIBE_KEY:
      settings.BluetoothVibe = new_tuple->value->uint8;
      break;      
    case HOURLYVIBE_KEY:
      settings.HourlyVibe = new_tuple->value->uint8;
      break;
    
    case KEY_TEMPERATURE:
      TEMPERATURE = new_tuple->value->int16; 
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Temperature %d, write persist... ", TEMPERATURE);
      persist_write_int(PERSIST_KEY_TEMPERATURE, TEMPERATURE);
    break;
    
    case KEY_CONDITION:
      CONDITION = new_tuple->value->int16; 
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Condition code %d, write persist... ", CONDITION);
      persist_write_int(PERSIST_KEY_CONDITION, CONDITION);
    break;
		 
      tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
     	layer_set_hidden(big_time_layer, false);
			
			appStarted = false;
			time_t now = time(NULL);
			struct tm *tick_time = localtime(&now);  
			handle_tick(tick_time, HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);
			appStarted = true;
  }
}


static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id, GPoint origin) {
  GBitmap *old_image = *bmp_image;
  *bmp_image = gbitmap_create_with_palette(COLOUR_USER, resource_id);
  GRect frame = (GRect) {
    .origin = origin,
    .size = gbitmap_get_bounds(*bmp_image).size
  };
	
	bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
	layer_set_frame(bitmap_layer_get_layer(bmp_layer), frame);
		
  if (old_image != NULL) {
		gbitmap_destroy(old_image);
		old_image = NULL;
  }
}

static void update_battery(BatteryChargeState charge_state) {

  batteryPercent = charge_state.charge_percent;

  if(batteryPercent==100) {
  change_battery_icon(false);
  layer_set_hidden(bitmap_layer_get_layer(battery_layer), false);
    for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
      layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), true);
    }  
    return;
  }

  layer_set_hidden(bitmap_layer_get_layer(battery_layer), charge_state.is_charging);
  change_battery_icon(charge_state.is_charging);
    
  for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    layer_set_hidden(bitmap_layer_get_layer(battery_percent_layers[i]), false);
  }  
  set_container_image(&battery_percent_image[0], battery_percent_layers[0], TINY_IMAGE_RESOURCE_IDS[charge_state.charge_percent/10], GPoint(99, 5));
  set_container_image(&battery_percent_image[1], battery_percent_layers[1], TINY_IMAGE_RESOURCE_IDS[charge_state.charge_percent%10], GPoint(105, 5));
  set_container_image(&battery_percent_image[2], battery_percent_layers[2], TINY_IMAGE_RESOURCE_IDS[10], GPoint(111, 5));
 
}

static void toggle_bluetooth_icon(bool connected) {
  if(appStarted && !connected && settings.BluetoothVibe) {
    //vibe!
    vibes_long_pulse();
  }
  layer_set_hidden(bitmap_layer_get_layer(bluetooth_layer), !connected);
}

void bluetooth_connection_callback(bool connected) {
  toggle_bluetooth_icon(connected);
}

void battery_layer_update_callback(Layer *me, GContext* ctx) {        
  //draw the remaining battery percentage
  graphics_context_set_stroke_color(ctx, COLOR_FALLBACK(GColorGreen, GColorBlack));
  graphics_context_set_fill_color(ctx, COLOR_FALLBACK(GColorGreen, GColorBlack));
  graphics_fill_rect(ctx, GRect(2, 2, ((batteryPercent/100.0)*11.0), 5), 0, GCornerNone);
  }

unsigned short get_display_hour(unsigned short hour) {
  if (clock_is_24h_style()) {
    return hour;
  }
  unsigned short display_hour = hour % 12;
  // Converts "0" to "12"
  return display_hour ? display_hour : 12;
}

static void update_days(struct tm *tick_time) {
  set_container_image(&day_name_image, day_name_layer, DAY_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_wday], GPoint(5, 78));
  set_container_image(&month_name_image, month_name_layer, MONTH_NAME_IMAGE_RESOURCE_IDS[tick_time->tm_mon], GPoint(93, 78));
  
  set_container_image(&date_digits_images[0], date_digits_layers[0], DATENUM_IMAGE_RESOURCE_IDS[tick_time->tm_mday/10], GPoint(58, 78));
  set_container_image(&date_digits_images[1], date_digits_layers[1], DATENUM_IMAGE_RESOURCE_IDS[tick_time->tm_mday%10], GPoint(75, 78));
  
  static char week_text[] = "00"; 
  strftime(week_text, sizeof(week_text), "%V", tick_time);
  int week = atoi(week_text);

  set_container_image(&week_images[0], week_layers[0], WEEK_IMAGE_RESOURCE_IDS[10], GPoint(56, 5));
  set_container_image(&week_images[1], week_layers[1], WEEK_IMAGE_RESOURCE_IDS[week/10], GPoint(72, 5));
  set_container_image(&week_images[2], week_layers[2], WEEK_IMAGE_RESOURCE_IDS[week%10], GPoint(78, 5));
  
  
}

static void update_steps(int steps) {
  if( ((steps/10000)%10) != 0 ){
    set_container_image(&steps_digits_images[0], steps_digits_layers[0], STEPS_IMAGE_RESOURCE_IDS[(steps/10000)%10], GPoint(58, 48));
    set_container_image(&steps_digits_images[1], steps_digits_layers[1], STEPS_IMAGE_RESOURCE_IDS[(steps/1000)%10], GPoint(75, 48));
    set_container_image(&steps_digits_images[2], steps_digits_layers[2], STEPS_IMAGE_RESOURCE_IDS[(steps/100)%10], GPoint(92, 48));
    set_container_image(&steps_digits_images[3], steps_digits_layers[3], STEPS_IMAGE_RESOURCE_IDS[(steps/10)%10], GPoint(109, 48));
    set_container_image(&steps_digits_images[4], steps_digits_layers[4], STEPS_IMAGE_RESOURCE_IDS[steps%10], GPoint(126, 48));  
    
  }
  if( ((steps/10000)%10) == 0 && ((steps/1000)%10) != 0 ){
    set_container_image(&steps_digits_images[0], steps_digits_layers[0], STEPS_IMAGE_RESOURCE_IDS[10], GPoint(54, 47));
    set_container_image(&steps_digits_images[1], steps_digits_layers[1], STEPS_IMAGE_RESOURCE_IDS[(steps/1000)%10], GPoint(75, 48));
    set_container_image(&steps_digits_images[2], steps_digits_layers[2], STEPS_IMAGE_RESOURCE_IDS[(steps/100)%10], GPoint(92, 48));
    set_container_image(&steps_digits_images[3], steps_digits_layers[3], STEPS_IMAGE_RESOURCE_IDS[(steps/10)%10], GPoint(109, 48));
    set_container_image(&steps_digits_images[4], steps_digits_layers[4], STEPS_IMAGE_RESOURCE_IDS[steps%10], GPoint(126, 48));  
    }
  if( ((steps/10000)%10) == 0 && ((steps/1000)%10) == 0 && ((steps/100)%10) != 0 ){
    set_container_image(&steps_digits_images[0], steps_digits_layers[0], STEPS_IMAGE_RESOURCE_IDS[10], GPoint(54, 47));
    set_container_image(&steps_digits_images[2], steps_digits_layers[2], STEPS_IMAGE_RESOURCE_IDS[(steps/100)%10], GPoint(92, 48));
    set_container_image(&steps_digits_images[3], steps_digits_layers[3], STEPS_IMAGE_RESOURCE_IDS[(steps/10)%10], GPoint(109, 48));
    set_container_image(&steps_digits_images[4], steps_digits_layers[4], STEPS_IMAGE_RESOURCE_IDS[steps%10], GPoint(126, 48));  
    }
  if( ((steps/10000)%10) == 0 && ((steps/1000)%10) == 0 && ((steps/100)%10) == 0 && ((steps/10)%10) != 0 ){
    set_container_image(&steps_digits_images[0], steps_digits_layers[0], STEPS_IMAGE_RESOURCE_IDS[10], GPoint(54, 47));
    set_container_image(&steps_digits_images[3], steps_digits_layers[3], STEPS_IMAGE_RESOURCE_IDS[(steps/10)%10], GPoint(109, 48));
    set_container_image(&steps_digits_images[4], steps_digits_layers[4], STEPS_IMAGE_RESOURCE_IDS[steps%10], GPoint(126, 48));  
  }
  if( ((steps/10000)%10) == 0 && ((steps/1000)%10) == 0 && ((steps/100)%10) == 0 && ((steps/10)%10) == 0 ){
    set_container_image(&steps_digits_images[0], steps_digits_layers[0], STEPS_IMAGE_RESOURCE_IDS[10], GPoint(54, 47));
    set_container_image(&steps_digits_images[4], steps_digits_layers[4], STEPS_IMAGE_RESOURCE_IDS[steps%10], GPoint(126, 48));
  }
}
 
static void update_temp(int TEMPERATURE, int CONDITION) {
  
  set_container_image(&condition_image, condition_layer, CONDITION_IMAGE_RESOURCE_IDS[CONDITION], GPoint(1, 19));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Condition in update_temp function: %d ", CONDITION);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Temperature in update_temp function: %d ", TEMPERATURE);
  
  if(TEMPERATURE > 250 ){
    Celsius = false;
    TEMPERATURE =TEMPERATURE - 400;
  }
  else {
    Celsius = true;
  }
  
  if( TEMPERATURE == 200 ){
    set_container_image(&temp_digits_images[0], temp_digits_layers[0], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[12], GPoint(92, 20));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[12], GPoint(109, 20));
  }
  else if( ((TEMPERATURE/100)%10) != 0 && Celsius == false ){
    set_container_image(&temp_digits_images[0], temp_digits_layers[0], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[(TEMPERATURE/100)%10], GPoint(75, 21));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[(TEMPERATURE/10)%10], GPoint(92, 21));
    set_container_image(&temp_digits_images[3], temp_digits_layers[3], TEMP_IMAGE_RESOURCE_IDS[TEMPERATURE%10], GPoint(109, 21));  
    set_container_image(&temp_digits_images[4], temp_digits_layers[4], TEMP_IMAGE_RESOURCE_IDS[13], GPoint(126, 21));  
  }
  else if( ((TEMPERATURE/10)%10) != 0 && TEMPERATURE >=0 && Celsius == false ){
    set_container_image(&temp_digits_images[0], temp_digits_layers[0], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[(TEMPERATURE/10)%10], GPoint(75, 21));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[TEMPERATURE%10], GPoint(92, 21));  
    set_container_image(&temp_digits_images[3], temp_digits_layers[3], TEMP_IMAGE_RESOURCE_IDS[13], GPoint(109, 21));
    set_container_image(&temp_digits_images[4], temp_digits_layers[4], TEMP_IMAGE_RESOURCE_IDS[15], GPoint(126, 21));
  }
  else if( ((TEMPERATURE/10)%10) != 0 && TEMPERATURE >=0 && Celsius == true ){
    set_container_image(&temp_digits_images[0], temp_digits_layers[0], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[(TEMPERATURE/10)%10], GPoint(75, 21));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[TEMPERATURE%10], GPoint(92, 21));  
    set_container_image(&temp_digits_images[3], temp_digits_layers[3], TEMP_IMAGE_RESOURCE_IDS[13], GPoint(109, 20));
    set_container_image(&temp_digits_images[4], temp_digits_layers[4], TEMP_IMAGE_RESOURCE_IDS[14], GPoint(126, 21));
  }
  else if( ((TEMPERATURE/10)%10) == 0 && TEMPERATURE >=0 && Celsius == false ){
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[TEMPERATURE%10], GPoint(92, 21));
    set_container_image(&temp_digits_images[3], temp_digits_layers[3], TEMP_IMAGE_RESOURCE_IDS[13], GPoint(109, 20));
    set_container_image(&temp_digits_images[4], temp_digits_layers[4], TEMP_IMAGE_RESOURCE_IDS[15], GPoint(126, 21));
  }
  else if(  ((TEMPERATURE/10)%10) == 0 && TEMPERATURE >=0 && Celsius == true ){
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[TEMPERATURE%10], GPoint(92, 21));
    set_container_image(&temp_digits_images[3], temp_digits_layers[3], TEMP_IMAGE_RESOURCE_IDS[13], GPoint(109, 20));
    set_container_image(&temp_digits_images[4], temp_digits_layers[4], TEMP_IMAGE_RESOURCE_IDS[14], GPoint(126, 21));
  }
  else if( ((TEMPERATURE/10)%10) != 0 && TEMPERATURE < 0 ){
    TEMPERATURE = abs(TEMPERATURE);
    set_container_image(&temp_digits_images[0], temp_digits_layers[0], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[12], GPoint(75, 20));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[(TEMPERATURE/10)%10], GPoint(92, 21));
    set_container_image(&temp_digits_images[3], temp_digits_layers[3], TEMP_IMAGE_RESOURCE_IDS[TEMPERATURE%10], GPoint(109, 21));  
    set_container_image(&temp_digits_images[4], temp_digits_layers[4], TEMP_IMAGE_RESOURCE_IDS[13], GPoint(126, 21));
  }
  else if( ((TEMPERATURE/10)%10) == 0 && TEMPERATURE < 0 && Celsius == false ){
    TEMPERATURE = abs(TEMPERATURE);
    set_container_image(&temp_digits_images[0], temp_digits_layers[0], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[12], GPoint(75, 20));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[TEMPERATURE%10], GPoint(92, 21));
    set_container_image(&temp_digits_images[3], temp_digits_layers[3], TEMP_IMAGE_RESOURCE_IDS[13], GPoint(109, 20));
    set_container_image(&temp_digits_images[4], temp_digits_layers[4], TEMP_IMAGE_RESOURCE_IDS[15], GPoint(126, 20));
  }
  else if( ((TEMPERATURE/10)%10) == 0 && TEMPERATURE < 0 && Celsius == true ){
    TEMPERATURE = abs(TEMPERATURE);
    set_container_image(&temp_digits_images[0], temp_digits_layers[0], TEMP_IMAGE_RESOURCE_IDS[10], GPoint(54, 20));
    set_container_image(&temp_digits_images[1], temp_digits_layers[1], TEMP_IMAGE_RESOURCE_IDS[12], GPoint(75, 20));
    set_container_image(&temp_digits_images[2], temp_digits_layers[2], TEMP_IMAGE_RESOURCE_IDS[TEMPERATURE%10], GPoint(92, 21));
    set_container_image(&temp_digits_images[3], temp_digits_layers[3], TEMP_IMAGE_RESOURCE_IDS[13], GPoint(109, 20));
    set_container_image(&temp_digits_images[4], temp_digits_layers[4], TEMP_IMAGE_RESOURCE_IDS[14], GPoint(126, 20));
  }
    }
  

static void update_hours(struct tm *tick_time) {

  if(appStarted && settings.HourlyVibe) {
    //vibe!
    vibes_short_pulse();
  }
  
  unsigned short display_hour = get_display_hour(tick_time->tm_hour);


		set_container_image(&time_digits_images[0], time_digits_layers[0], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour/10], GPoint(3, 117));
		set_container_image(&time_digits_images[1], time_digits_layers[1], BIG_DIGIT_IMAGE_RESOURCE_IDS[display_hour%10], GPoint(31, 117));
    
  if (!clock_is_24h_style()) {
    if (tick_time->tm_hour >= 12) {
      set_container_image(&time_format_image, time_format_layer, RESOURCE_ID_IMAGE_PM_MODE, GPoint(20, 5));
      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), false);
    } 
    else {
      layer_set_hidden(bitmap_layer_get_layer(time_format_layer), true);
    }
    if (display_hour/10 == 0) {
			
				layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), true);
    }
    else {
			
				layer_set_hidden(bitmap_layer_get_layer(time_digits_layers[0]), false);
			}
  }
}

static void update_minutes(struct tm *tick_time) {
		set_container_image(&time_digits_images[2], time_digits_layers[2], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min/10], GPoint(87, 117));
		set_container_image(&time_digits_images[3], time_digits_layers[3], BIG_DIGIT_IMAGE_RESOURCE_IDS[tick_time->tm_min%10], GPoint(115, 117));
}
  
static void update_seconds(struct tm *tick_time) {

  if(settings.Blink == BLINK_ON) {
    animation_unschedule_all();
    layer_set_hidden(bitmap_layer_get_layer(separator_layer), tick_time->tm_sec%2);
  }
 
  else if (settings.Blink == BLINK_OFF) {
			animation_unschedule_all();
  }	
    else {
				layer_set_hidden(bitmap_layer_get_layer(separator_layer), false);
			}	
}

static void health_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate) {
    // display the step count
    //int steps = 8888;
    int steps = ((int)health_service_sum_today(HealthMetricStepCount));
    update_steps(steps);
  }
}

static void handle_tick(struct tm *tick_time, TimeUnits units_changed) {
  if (units_changed & DAY_UNIT) {
    update_days(tick_time);
  }
  if (units_changed & HOUR_UNIT) {
    update_hours(tick_time);
  }
  if (units_changed & MINUTE_UNIT) {
    update_minutes(tick_time);
    update_temp(TEMPERATURE, CONDITION);
    //refresh_weather();
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating weather display");
  } 
  if (units_changed & SECOND_UNIT) {
    update_seconds(tick_time);
  }
  if (tick_time->tm_min % 15 == 0 && tick_time->tm_sec == 0) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Updating weather tick_based");
    refresh_weather();
  }
}

static void loadPersistentSettings() {  
  valueRead = persist_read_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void savePersistentSettings() {
  valueWritten = persist_write_data(SETTINGS_KEY, &settings, sizeof(settings));
}

static void LoadPersistentWeather() {
  if (persist_exists(PERSIST_KEY_TEMPERATURE)) {
        TEMPERATURE = persist_read_int(PERSIST_KEY_TEMPERATURE);
    } else {
        TEMPERATURE = 200;
    }
  if (persist_exists(PERSIST_KEY_CONDITION)) {
        CONDITION = persist_read_int(PERSIST_KEY_CONDITION);
    } else {
        CONDITION = 12;
    }
}

static void SavePersistentWeather() {
  persist_write_int(PERSIST_KEY_TEMPERATURE, TEMPERATURE);
  persist_write_int(PERSIST_KEY_CONDITION, CONDITION);
}

static void init(void) {
  memset(&time_digits_layers, 0, sizeof(time_digits_layers));
  memset(&time_digits_images, 0, sizeof(time_digits_images));
  memset(&date_digits_layers, 0, sizeof(date_digits_layers));
  memset(&date_digits_images, 0, sizeof(date_digits_images));
  memset(&week_layers, 0, sizeof(week_layers));
  memset(&week_images, 0, sizeof(week_images));
  memset(&steps_digits_layers, 0, sizeof(steps_digits_layers));
  memset(&steps_digits_images, 0, sizeof(steps_digits_images));
  memset(&temp_digits_layers, 0, sizeof(temp_digits_layers));
  memset(&temp_digits_images, 0, sizeof(temp_digits_images));
  memset(&battery_percent_layers, 0, sizeof(battery_percent_layers));
  memset(&battery_percent_image, 0, sizeof(battery_percent_image));

  const int inbound_size = 128;
  const int outbound_size = 128;
  app_message_open(inbound_size, outbound_size);  
  //TEMPERATURE = 200;

  window = window_create();
  if (window == NULL) {
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "OOM: couldn't allocate window");
      return;
  }
  window_stack_push(window, true /* Animated */);
  window_layer = window_get_root_layer(window);
  
  loadPersistentSettings();
  
  LoadPersistentWeather();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "INIT: Temperature from persist storage %d ", TEMPERATURE);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "INIT: Condition from persist storage %d ", CONDITION);
  
  background_image = gbitmap_create_with_palette(COLOUR_USER, RESOURCE_ID_IMAGE_BACKGROUND);
  background_layer = bitmap_layer_create(layer_get_frame(window_layer));
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
  
  big_time_layer = layer_create(layer_get_frame(window_layer));
  layer_add_child(window_layer, big_time_layer);
  
  separator_image = gbitmap_create_with_palette(COLOUR_USER, RESOURCE_ID_IMAGE_SEPARATOR);
  
  GRect frame_steps = (GRect) {
    .origin = { .x = 59, .y = 117},
    .size = gbitmap_get_bounds(separator_image).size
  };
  separator_layer = bitmap_layer_create(frame_steps);
  
  
  bitmap_layer_set_bitmap(separator_layer, separator_image);
  layer_add_child(big_time_layer, bitmap_layer_get_layer(separator_layer));   
 
  bluetooth_image = gbitmap_create_with_palette(COLOUR_USER, RESOURCE_ID_IMAGE_BLUETOOTH);
  GRect frame3 = (GRect) {
    .origin = { .x = 7, .y = 5 },
    .size = gbitmap_get_bounds(bluetooth_image).size
  };
  bluetooth_layer = bitmap_layer_create(frame3);
  bitmap_layer_set_bitmap(bluetooth_layer, bluetooth_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(bluetooth_layer));
  
  battery_image = gbitmap_create_with_palette(COLOUR_USER, RESOURCE_ID_IMAGE_BATTERY);
  GRect frame4 = (GRect) {
    .origin = { .x = 121, .y = 5 },
    .size = gbitmap_get_bounds(battery_image).size
  };
  battery_layer = bitmap_layer_create(frame4);
  battery_image_layer = bitmap_layer_create(frame4);
  bitmap_layer_set_bitmap(battery_image_layer, battery_image);
  layer_set_update_proc(bitmap_layer_get_layer(battery_layer), battery_layer_update_callback);
  
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_image_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(battery_layer));
  
  GRect frame5 = (GRect) {
    .origin = { .x = 20, .y = 5 },
    .size = {.w = 19, .h = 8}
  };
  time_format_layer = bitmap_layer_create(frame5);
  if (clock_is_24h_style()) {
    time_format_image = gbitmap_create_with_palette(COLOUR_USER, RESOURCE_ID_IMAGE_24_HOUR_MODE);
    bitmap_layer_set_bitmap(time_format_layer, time_format_image);
    
  }
  layer_add_child(window_layer, bitmap_layer_get_layer(time_format_layer));

  // Create time and date layers
  GRect dummy_frame = { {0, 0}, {0, 0} };
  day_name_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(window_layer, bitmap_layer_get_layer(day_name_layer));
  
  //GRect dummy_frame = { {0, 0}, {0, 0} };
  month_name_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(window_layer, bitmap_layer_get_layer(month_name_layer));
  
  condition_layer = bitmap_layer_create(dummy_frame);
  layer_add_child(window_layer, bitmap_layer_get_layer(condition_layer));
  
  for (int i = 0; i < TOTAL_TIME_DIGITS; ++i) {
    time_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(big_time_layer, bitmap_layer_get_layer(time_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_DATE_DIGITS; ++i) {
    date_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(date_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_STEPS_DIGITS; ++i) {
    steps_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(steps_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_TEMP_DIGITS; ++i) {
    temp_digits_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(temp_digits_layers[i]));
  }
  for (int i = 0; i < TOTAL_WEEK_DIGITS; ++i) {
    week_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(week_layers[i]));
  }
  for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; ++i) {
    battery_percent_layers[i] = bitmap_layer_create(dummy_frame);
    layer_add_child(window_layer, bitmap_layer_get_layer(battery_percent_layers[i]));
  }
  
  toggle_bluetooth_icon(bluetooth_connection_service_peek());
  update_battery(battery_state_service_peek());

  Tuplet initial_values[] = {
    TupletInteger(BLINK_KEY, settings.Blink),
    TupletInteger(INVERT_KEY, settings.Invert),
    TupletInteger(BLUETOOTHVIBE_KEY, settings.BluetoothVibe),
    TupletInteger(HOURLYVIBE_KEY, settings.HourlyVibe),
    //TupletInteger(KEY_REQUEST, settings.HourlyVibe),
    TupletInteger(KEY_CONDITION, CONDITION),
    TupletInteger(KEY_TEMPERATURE, TEMPERATURE),
  };
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, NULL, NULL);
   
  bool _true = true;
  bool _false = false;
  separator_animation = property_animation_create(&layer_hidden_implementation, bitmap_layer_get_layer(separator_layer), &_false, &_true);
  animation_set_duration((Animation *) separator_animation, 1);
  animation_set_delay((Animation *) separator_animation, 500);
  animation_set_curve((Animation *) separator_animation, AnimationCurveLinear);

  // Avoids a blank screen on watch start.
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);  

  APP_LOG(APP_LOG_LEVEL_DEBUG, "refresh on start");
  handle_tick(tick_time, DAY_UNIT + HOUR_UNIT + MINUTE_UNIT + SECOND_UNIT);

  appStarted = true;

	tick_timer_service_subscribe(MINUTE_UNIT, handle_tick);
	refresh_weather();
  
  bluetooth_connection_service_subscribe(bluetooth_connection_callback);
  battery_state_service_subscribe(&update_battery);
  
  if(health_service_events_subscribe(health_handler, NULL)) {
    // force initial steps display
    health_handler(HealthEventMovementUpdate, NULL);
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  }


}


static void deinit(void) {

  savePersistentSettings();
  
  SavePersistentWeather();

  app_sync_deinit(&sync);
  
  tick_timer_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  health_service_events_unsubscribe();

  animation_unschedule_all();

  property_animation_destroy(separator_animation);
  separator_animation = NULL;

  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  background_image = NULL;
  
  layer_remove_from_parent(bitmap_layer_get_layer(separator_layer));
  bitmap_layer_destroy(separator_layer);
  gbitmap_destroy(separator_image);
  separator_image = NULL;
  
  layer_remove_from_parent(bitmap_layer_get_layer(bluetooth_layer));
  bitmap_layer_destroy(bluetooth_layer);
  gbitmap_destroy(bluetooth_image);
  bluetooth_image = NULL;
  
  layer_remove_from_parent(bitmap_layer_get_layer(battery_layer));
  bitmap_layer_destroy(battery_layer);
  gbitmap_destroy(battery_image);
  battery_image = NULL;
  
  layer_remove_from_parent(bitmap_layer_get_layer(battery_image_layer));
  bitmap_layer_destroy(battery_image_layer);

  layer_remove_from_parent(bitmap_layer_get_layer(time_format_layer));
  bitmap_layer_destroy(time_format_layer);
  gbitmap_destroy(time_format_image);
  time_format_image = NULL;
  
  layer_remove_from_parent(bitmap_layer_get_layer(day_name_layer));
  bitmap_layer_destroy(day_name_layer);
  gbitmap_destroy(day_name_image);
  day_name_image = NULL;
  
  layer_remove_from_parent(bitmap_layer_get_layer(month_name_layer));
  bitmap_layer_destroy(month_name_layer);
  gbitmap_destroy(month_name_image);
  month_name_image = NULL;
  
  layer_remove_from_parent(bitmap_layer_get_layer(condition_layer));
  bitmap_layer_destroy(condition_layer);
  gbitmap_destroy(condition_image);
  condition_image = NULL;
  
  for (int i = 0; i < TOTAL_DATE_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(date_digits_layers[i]));
    gbitmap_destroy(date_digits_images[i]);
    date_digits_images[i] = NULL;
    bitmap_layer_destroy(date_digits_layers[i]);
    date_digits_layers[i] = NULL;
  }
  
   for (int i = 0; i < TOTAL_STEPS_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(steps_digits_layers[i]));
    gbitmap_destroy(steps_digits_images[i]);
    steps_digits_images[i] = NULL;
    bitmap_layer_destroy(steps_digits_layers[i]);
    steps_digits_layers[i] = NULL;
  }
  for (int i = 0; i < TOTAL_TEMP_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(temp_digits_layers[i]));
    gbitmap_destroy(temp_digits_images[i]);
    temp_digits_images[i] = NULL;
    bitmap_layer_destroy(temp_digits_layers[i]);
    temp_digits_layers[i] = NULL;
  }
  
   for (int i = 0; i < TOTAL_WEEK_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(week_layers[i]));
    gbitmap_destroy(week_images[i]);
    week_images[i] = NULL;
    bitmap_layer_destroy(week_layers[i]);
    week_layers[i] = NULL;
  }
  
  for (int i = 0; i < TOTAL_TIME_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(time_digits_layers[i]));
    gbitmap_destroy(time_digits_images[i]);
    time_digits_images[i] = NULL;
    bitmap_layer_destroy(time_digits_layers[i]);
    time_digits_layers[i] = NULL;
  }

  for (int i = 0; i < TOTAL_BATTERY_PERCENT_DIGITS; i++) {
    layer_remove_from_parent(bitmap_layer_get_layer(battery_percent_layers[i]));
    gbitmap_destroy(battery_percent_image[i]);
    battery_percent_image[i] = NULL;
    bitmap_layer_destroy(battery_percent_layers[i]); 
    battery_percent_layers[i] = NULL;
  } 
  
  layer_remove_from_parent(big_time_layer);
  big_time_layer = NULL;
  
  appStarted = NULL;
  
  layer_remove_from_parent(window_layer);
  layer_destroy(window_layer);
  
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}