#include <SoftwareSerial.h>

#define RX 3
#define TX 2

SoftwareSerial lora(RX, TX); // RX, TX --> physically(RX=2, TX=3) 902 mhz band
unsigned int rec_timeout = 10000;
unsigned int prev_sent = 0;

void setup() {
  Serial.begin(115200); // Initialize USB Serial
  lora.begin(115200); // Initialize Software Serial
  sendATcommand("AT+ADDRESS=7", 500);
  sendATcommand("AT+BAND=905000000", 500);
  sendATcommand("AT+NETWORKID=5", 500); 
  delay(5000);
}
bool sender = true;
void loop() {
  // Check if data is available on USB Serial
  // while (Serial.available()) {
  //   String data = Serial.readString();
  //   lora.println(data); // Send data to Software Serial

  //   //Serial.flush() //is very slow
  // }
  if (sender){
    sendATcommand("AT+SEND=1,5,Hello", 500);
    sender = false;
    prev_sent = millis();
  }
  else if (!sender){
    // Check if data is available on Software Serial
    String incomingString;
    if (lora.available()) {
      Serial.println("RECEIVED SOMETHING!");
      incomingString = lora.readString();
      Serial.println(incomingString);

      char dataArray[30]; 
      incomingString.toCharArray(dataArray,30);
      char* data = strtok(dataArray, ",");
      data = strtok(NULL, ",");
      data = strtok(NULL, ",");
      Serial.println(data);
      delay(2000);
      sender = true;
    }
    // else if ((millis()-prev_sent)>rec_timeout){
    //   Serial.println("Timeout");
    //   sender = true;
    //   prev_sent = millis();
    // }
  }
}

String sendATcommand(const char *toSend, unsigned long milliseconds) {
  String result;
  Serial.print("Sending: ");
  Serial.println(toSend);
  lora.println(toSend);
  unsigned long startTime = millis();
  Serial.print("Received: ");
  while (millis() - startTime < milliseconds) {
    if (lora.available()) {
      char c = lora.read();
      Serial.write(c);
      result += c;  // append to the result string
    }
  }
  Serial.println();  // new line after timeout.
  return result;
}
