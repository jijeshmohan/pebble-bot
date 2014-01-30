#include <pebble.h>
#define DELAY 100
#define ACCEL_RATIO 0.85
#define xAccelScale 1.6
#define yAccelScale 1.4
#define DELTA 3

static Window *window;
static TextLayer *text_layer;
static AppTimer *timer;
static char buff[32];
static bool initialized=false;
static int16_t initial_x;
static int16_t initial_y;
static double computed_x;
static double computed_y;
static int last_angle=0;
static int last_speed=0;
enum {
  BOT_ANGLE = 0x0,
  BOT_SPEED = 0x1
};

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Up");
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Down");
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w, 20 } });
  text_layer_set_text(text_layer, "theBot");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}


static void handle_accel(AccelData *accel_data, uint32_t num_samples) {
  
}

static int map(int n,int imin,int imax,int omin,int omax){
  return (n-imin)*(omax-omin)/(imax-imin) + omin;
}

static void control_bot(int angle, int speed) {
   int angleDelta = angle - last_angle;
   int speedDelta = speed - last_speed;
   if(angleDelta < 0 ){
    angleDelta=angleDelta*-1;
   }
   if(speedDelta < 0 ){
    speedDelta=speedDelta*-1;
   }
   if ( angleDelta < DELTA  && speedDelta < DELTA){
    return;
   }
   last_angle = angle;
   last_speed = speed;

  Tuplet angleTuple = TupletInteger(BOT_ANGLE, angle);
  Tuplet speedTuple = TupletInteger(BOT_SPEED, speed);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &angleTuple);
  dict_write_tuplet(iter, &speedTuple);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  int rc = accel_service_peek(&accel);
  if(rc==-1){
    text_layer_set_text(text_layer, "not running");
  }else if(rc==-2){
    text_layer_set_text(text_layer, "sub");
  }else{
      if(!initialized){
        initialized=true;
        initial_x=accel.x;
        initial_y=accel.y;      
        computed_x=accel.x;
        computed_y=accel.y;
      }else{
          computed_x = ACCEL_RATIO * computed_x + (1.0 - ACCEL_RATIO) * accel.x;
          computed_y = ACCEL_RATIO * computed_y + (1.0 - ACCEL_RATIO) * accel.y;
          int px = (computed_x - initial_x) * xAccelScale;
          int py = (computed_y - initial_y) * yAccelScale;
          int angle = map(px,-2000,2000,-45,45);
          int speed = map(py,-2000,2000,-99,99);

          if(speed < 0 ){
            speed=0;
          }
          snprintf(buff,32,"%d:%d",angle,speed);
          text_layer_set_text(text_layer, buff);
          control_bot(angle,speed);
      }
    }
  timer = app_timer_register(DELAY /* milliseconds */, timer_callback, NULL);
}



static void app_message_init(void) {
  app_message_open(64, 64);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  app_message_init();
  accel_data_service_subscribe(0, &handle_accel);
  timer = app_timer_register(DELAY /* milliseconds */, timer_callback, NULL);
}

static void deinit(void) {
  accel_data_service_unsubscribe();
  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
