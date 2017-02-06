#include <Adafruit_NeoPixel.h>
#define RGB_LED        4
#define NUMPIXELS      1
#define BIG_COFFEE    60
#define SMALL_COFFEE  30
#define WATER_THRESH 100
#define TEMP_THRESH  700 //+150
#define IDLE_TIME 120000
#define REFILL_TIMEOUT 5000 // how long to refill the watertank
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, RGB_LED, NEO_GRB + NEO_KHZ800);
short led_value = 0;
bool spin = false;
short temperature = 0; //contains the current temperature. init with low value to disable boiler at boot
short water_level = 1000; // contains the current waterlevel. init with high value to disable pump at boot and update with real value
short big_button = 0;
short small_button = 1;
long coffee_request = 0;
long coffee_request_2 = 0; //logically this is not required, but feedback from refill relay, invokes a false positive on the button press
bool hot = false;
short boiler = 5;
short pump = 6;
short valve = 7;
short led = 13;
bool led_state = false;
//// states
short state = 1;
short s_idle = 0;
short s_prepare = 1;
short s_ready = 2;
short s_serve = 3;
//// checks
long refill_start = 0;

long last_edge = 0;//last edge for led state
long coffee_start = 0; //when we started the pump for serving the coffee
long cur_time = 0; //current time
long last_interaction = 0; //when a button was pressed the last time
long last_log = -10000; //when the last log was sent
long last_coffee = 0; // wait some time after last coffee
void setup() {
  // relay 1
  pinMode(boiler, OUTPUT);
  digitalWrite(boiler, HIGH);
  // relay 2
  pinMode(pump, OUTPUT);
  digitalWrite(pump, HIGH);
  // relay 3
  pinMode(valve, OUTPUT);
  digitalWrite(valve, LOW);
  //water level sensor
  pinMode(A0, INPUT);
  // temperature sensor
  pinMode(A1, INPUT);
  // led
  //pinMode(A2,OUTPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  pinMode(RGB_LED, OUTPUT);

  // button 1 - interrupt
  pinMode(big_button + 2, INPUT_PULLUP);
  // button 2 - interrupt
  pinMode(small_button + 2, INPUT_PULLUP);


  last_edge = millis();
  attachInterrupt(big_button, coffee_big, FALLING);
  attachInterrupt(small_button, coffee_small, FALLING);
  pixels.begin();

  Serial.begin(9600);
}

void coffee_big() {
  setCoffeeRequest(BIG_COFFEE);
}
void coffee_small() {
  setCoffeeRequest(SMALL_COFFEE);
}
void setCoffeeRequest(short req) {
  last_interaction = millis();
  if (coffee_request != 0) {
    return;
  }
  if (millis() - last_coffee < 1000) {
    //wait some time until last coffee has finished;
    return;
  }
  Serial.println("set coffee req");
  coffee_request = req;

}
void loop() {
  cur_time = millis();
  if (cur_time < 200) // no request during boot
    coffee_request = 0;


  if (cur_time - last_log > 2000) {
    last_log = cur_time;
    Serial.print("state ");
    Serial.print(state);
    Serial.print(" temp ");
    Serial.print(temperature);
    Serial.print(" water ");
    Serial.print(water_level);
    //Serial.print(" hot ");
    //Serial.print(hot);
    int hotness = min(64, max((temperature - 400), 0) * 64 / 300);
    Serial.print(" hotness ");
    Serial.println(hotness);
    if (coffee_request != 0) {
      Serial.print(" coffee request: ");
      Serial.print(coffee_request_2 - (cur_time - coffee_start) / 1000);
    }
    Serial.println();
  }
  // check for any interaction
  idleCheck();
  // check if there is sufficient water
  waterCheck();
  // checkTemperature
  tempCheck();

  // if no interaction has been recognized for 2 min the boiler is turned off.
  if (state == s_idle)
    idleState();
  // check prepare some hot water before starting
  if (state == s_prepare)
    prepareState();
  // if we are ready check, if there has been a coffee request and start pump
  if (state == s_ready)
    readyState();

  // serving coffee. deactivate pump as soon as the time for a coffee has been reached.
  //double time for big coffee
  if (state == s_serve)
    serveState();

  //display the state with the led
  showState();
  delay(50);
}
///////////////////HELPER FUNCTIONS//////////////////////
void showState() {
  if (state == s_ready) {
    pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  }
  if (state == s_serve) {
    pixels.setPixelColor(0, pixels.Color(100, 100, 0));
  }
  if (state == s_prepare) {
    if (millis() - last_edge > 800) { // blink every 800ms
      int hotness = min(64, max((temperature - 400), 0) * 64 / 300);

      pixels.setPixelColor(0, pixels.Color(hotness, 0, hotness - 64));
    }
  }
  if (state == s_idle) {
    if (led_value >= 50)
      spin = true;
    if (led_value <= 0)
      spin = false;
    if (!spin)
      led_value++;
    else
      led_value--;
    pixels.setPixelColor(0, pixels.Color(led_value, led_value, led_value));
  }
  pixels.show();
}



