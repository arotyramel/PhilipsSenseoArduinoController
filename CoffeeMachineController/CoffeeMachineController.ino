#include <Adafruit_NeoPixel.h>
#define RGB_LED        4
#define NUMPIXELS      1
#define BIG_COFFEE    60
#define SMALL_COFFEE  30
#define WATER_THRESH 100
#define TEMP_THRESH  700 //+150
#define IDLE_TIME 120000
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, RGB_LED, NEO_GRB + NEO_KHZ800);
short led_value = 0;
bool spin = false;
short temperature = 0;
short water_level = 0;
short big_button = 0;
short small_button = 1;
long coffee_request = 0;
bool hot = false;
short boiler = 5;
short pump = 6;
short valve = 7;
short led = 13;
bool led_state = false;
// states
short state = 1;
short idle = 0;
short no_water = 1;
short refill = 2;
short warm_up = 3;
short ready_ = 4;
short serving = 5;
long last_edge = 0;
long coffee_start = 0;
long cur_time = 0;
long last_interaction = 0;
long last_log = -10000;
long time_without_water = 0;
long watering_time = 0;
long last_coffee=0;
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
void setCoffeeRequest(short req){
  last_interaction = millis();
  if(coffee_request != 0){
    return;
  }
  if(millis()-last_coffee < 1000){
    //wait some time until last coffee has finished;
    return;
  }
  coffee_request = req;
  
}
void loop() {
  cur_time = millis();
  if(cur_time<200) // no request during boot
    coffee_request = 0;
  water_level = water_level * 0.9 + 0.1 * analogRead(A0);

  if (cur_time - last_log > 2000) {
    last_log = cur_time;
    Serial.print("state ");
    Serial.print(state);
    Serial.print(" temperature ");
    Serial.print(temperature);
    Serial.print(" water_level ");
    Serial.print(water_level);
    Serial.print(" hot ");
    Serial.print(hot);
    Serial.print(" coffee request: ");
    Serial.print(coffee_request);
    Serial.println();
  }
  // check for any interaction

  if (cur_time - last_interaction > IDLE_TIME) {

    if (state != idle)
      Serial.println("idling");
    state = idle;
  }

  // if no interaction has been recognized for 2 min the boiler is turned off.
  if (state == idle) {
    digitalWrite(boiler, HIGH);
    digitalWrite(pump, HIGH);
    digitalWrite(valve, LOW);
    if (cur_time - last_interaction < IDLE_TIME) {
      state = no_water;
      coffee_request = 0;
      temperature = TEMP_THRESH;
      water_level = WATER_THRESH;
      Serial.println("going back to start -> no water");
    }
  }

  // check if there is sufficient water
  if (state >= no_water) { //if not idle
    if (water_level < WATER_THRESH) {
      if (state != no_water && state != refill) {
        Serial.println("no_water");
      }

      if (state == serving) { //emergency refill
        Serial.println("emergency refill");
        watering_time = cur_time;
        digitalWrite(valve,HIGH); 
        do_refill(cur_time, true);
      }
      else {
        if (state != refill) {
          state = no_water; //no water
        }
      }
    }
    else { //enough water
      digitalWrite(valve, LOW);
      time_without_water = cur_time;
      if (state == no_water) {
        state = warm_up; // water -> warm up water
        Serial.println("going to warm-up");
        delay(500);
      }
    }
  }

  // if there is no water. turn off boiler and pump
  if (state == no_water) {
    digitalWrite(boiler, HIGH);
    digitalWrite(pump, HIGH);
    if (cur_time - time_without_water > 5000) {
      time_without_water = cur_time;
      //if there has been no water for 5 sec -> activate valve
      Serial.println("go to refill");
      state = refill;
      watering_time = cur_time;
      digitalWrite(valve,HIGH);
    }

  }
  if (state == refill) {
    do_refill(cur_time, false);
  }

  // if there is water and we are in not idling, then heat up until we reach temperature.
  if (state >= warm_up) {
    checkTemperature();
  }
  // if water is hot, show ready
  if ((state == warm_up) && hot) {
    state = ready_;
    Serial.println("going to ready from warm_up");
  }

  // if we are ready check, if there has been a coffee request and start pump
  if (state == ready_) {
    if (coffee_request) {
      Serial.print("start serving coffee ");
      Serial.println(coffee_request);
      state = serving;
      coffee_start = cur_time;
      digitalWrite(pump, LOW);
    }
  }

  // serving coffee. deactivate pump as soon as the time for a coffee has been reached.
  //double time for big coffee
  if (state == serving) {
    if (cur_time - coffee_start > (coffee_request * 1000)) {
      Serial.println("coffee done");
      digitalWrite(pump, HIGH);
      state = no_water; // start from beginning
      coffee_request = 0;
      last_coffee = cur_time;
      last_interaction = cur_time;
    }
  }
  //display the state with the led
  showState();
  delay(50);
}
///////////////////HELPER FUNCTIONS//////////////////////
void showState() {
  if (state == ready_) {
    setLed(true);
    pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  }
  if (state == serving) {
    setLed(true);
    pixels.setPixelColor(0, pixels.Color(100, 0, 100));
  }
  if (state == refill) {
    pixels.setPixelColor(0, pixels.Color(100, 100, 0));
  }
  if (state == warm_up) {
    if (millis() - last_edge > 800) { // blink every 800ms
      led_state = !led_state;
      setLed(led_state);
    }
  }
  if (state == no_water) {
    if (millis() - last_edge > 300) { //blink every 300 ms
      led_state = !led_state;
      setLed(led_state);

    }
  }
  if (state == idle) {
    setLed(false);

    if (led_value >= 64)
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

void checkTemperature() {
  temperature = analogRead(A1);
  int threshold = TEMP_THRESH;
  if (state!=warm_up)
    threshold= TEMP_THRESH+150;
  if (temperature > threshold + 5) {
    hot = true;
    digitalWrite(boiler, HIGH);
  }
  if (temperature < threshold - 5) {
    hot = false;
    digitalWrite(boiler, LOW);
  }
}
void setLed(bool led_state) {
  last_edge = millis();
  //digitalWrite(led, led_state);
  if (led_state) {
    //analogWrite(A2,675); //approx 3.3V
    if (state == no_water)
      pixels.setPixelColor(0, pixels.Color(0, 0, 128));

  }
  else {
    //analogWrite(A2,0);
    if (state == no_water)
      pixels.setPixelColor(0, pixels.Color(0, 0, 0));
  }
  if (state == warm_up) {
    int hotness = temperature / 8;
    pixels.setPixelColor(0, pixels.Color(hotness, 0, hotness - 128));
  }
}


void do_refill(long &cur_time, bool emergency_refill) {
  if (cur_time - watering_time > 6000) {
    digitalWrite(valve,LOW);
    if (!emergency_refill) {
      state = no_water; //check again
      coffee_request = 0;//just in case 
    }
    else
      Serial.println("refilled");
  }
}

