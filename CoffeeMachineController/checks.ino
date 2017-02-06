
void idleCheck() {
  if (cur_time - last_interaction > IDLE_TIME && state != s_idle) {
    Serial.println("idling");
    state = s_idle;
  }
}

void waterCheck() {
  water_level = water_level * 0.9 + 0.1 * analogRead(A0);
  if (water_level < WATER_THRESH) {
    if (!digitalRead(valve)) {
      digitalWrite(valve, HIGH);
      Serial.println("open valve");
      refill_start = cur_time;
    }
  }
  else {
    if (cur_time - refill_start > REFILL_TIMEOUT) {
      if (digitalRead(valve)) {
        Serial.println("close valve");
        digitalWrite(valve, LOW);
        coffee_request = 0;
      }
    }
  }
}

void tempCheck() {
  temperature = analogRead(A1);
  int threshold = TEMP_THRESH;
  if (state != s_prepare)
    threshold = TEMP_THRESH + 150;
  if (temperature > threshold + 5) {
    hot = true;
    digitalWrite(boiler, HIGH);
  }
  if (temperature < threshold - 5) {
    hot = false;
    digitalWrite(boiler, LOW);
  }
}


