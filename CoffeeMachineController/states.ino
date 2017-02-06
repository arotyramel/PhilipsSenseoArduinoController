
void idleState() {
  {
    digitalWrite(boiler, HIGH);
    digitalWrite(pump, HIGH);
    digitalWrite(valve, LOW);
    if (cur_time - last_interaction < IDLE_TIME) {
      state = s_prepare;
      coffee_request = 0;
      Serial.println("woke up. going to prepare");
    }
  }
}
void prepareState() {
  // if water is hot, show ready
  if (hot) {
    state = s_ready;
    Serial.println("going to ready");
  }
}

void readyState() {
  if (coffee_request) {
    Serial.print("start serving coffee ");
    Serial.println(coffee_request);
    state = s_serve;
    coffee_start = cur_time;
    digitalWrite(pump, LOW);
    coffee_request_2 = coffee_request;
  }
}

void serveState() {
  if (cur_time - coffee_start > (coffee_request_2 * 1000)) {
    Serial.println("coffee done");
    digitalWrite(pump, HIGH);
    state = s_prepare; // start from beginning
    coffee_request = 0;
    last_coffee = cur_time;
  }
}


