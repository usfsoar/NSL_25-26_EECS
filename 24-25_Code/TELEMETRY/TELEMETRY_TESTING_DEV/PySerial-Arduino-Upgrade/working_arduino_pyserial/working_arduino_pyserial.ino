void setup() {
  Serial.begin(9600);  // Start Serial communication
  pinMode(2, OUTPUT); // Set pin 2 as OUTPUT
  digitalWrite(2, LOW); // Start with LED OFF
}


void loop() {
  if (Serial.available() > 0) {  // Check if data is available
    String receivedData = Serial.readStringUntil('\n'); // Read input
    receivedData.trim(); // Remove spaces and newline

    if (receivedData == "on") {
      digitalWrite(2, HIGH);
      Serial.println("Light is turned ON");
    }
    else if (receivedData == "off") {
      digitalWrite(2, LOW);
      Serial.println("Light is turned OFF");
    }
    else if (receivedData == "N") {
      Serial.println("Exiting...");
      while (true);  // Stop execution
    }
    else {
      Serial.println("Invalid command! Use 'on', 'off', or 'N'.");
    }
  }
}

