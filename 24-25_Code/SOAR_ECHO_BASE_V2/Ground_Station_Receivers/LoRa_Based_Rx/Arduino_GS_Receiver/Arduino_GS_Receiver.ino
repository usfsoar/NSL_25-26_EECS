// #include "SOAR_Lora.h"
#include "V1_SOAR_LORA.h"

#define RX 3
#define TX 2

SOAR_Lora lora("6", "5", "915000000"); // LoRa
bool reporting_lock = false;
String lora_input = "";
String address = "";
uint32_t latency_checkpoint = 0;

struct txMessage {
  char strToSend[500];
  int addressToSend;
};
struct rxMessage {
  int addressReceived;
  int length;
  byte* strReceived;
  int rssi;
  int snr;
};

// Classes ----------------------------------------------------------------------------------------
/*class InputCheck{
  //Byte array to store the input
  byte input_data[255];
  int destination_address = 0;
  int data_length = 0;
  String input_string = "";

  void check(){
    //If connected to a computer check for serial input
    //Input will be in the format: {0x02}{0x02}{destination_address 2B}{message_length 2B}{data_bytes undefined length}\n

    if(Serial.available()){
      //Empty both the input string and the input data array
      input_string = "";
      for(int i = 0; i < 255; i++){
        input_data[i] = 0;
      }
      data_length = 0;
      byte cmd1 = Serial.read();
      byte cmd2 = Serial.read();
      //If cmd1 and cmd2 are 0x02 then we have a valid input
      //Else treat it as a string input
      if(cmd1 == 0x02 && cmd2 == 0x02){
        // Read the destination address as 2 bytes
        byte dest_address[2];
        Serial.readBytes(dest_address, 2);
        destination_address = (dest_address[0] << 8) | dest_address[1];
        // Read the message length
        byte msg_length[2];
        Serial.readBytes(msg_length, 2);
        data_length = (msg_length[0] << 8) | msg_length[1];
        // Read the message
        Serial.readBytes(input_data, data_length);
      }
      else{
        input_string = (char)cmd1;
        input_string += (char)cmd2;
        //Read until the ',' character
        input_string += Serial.readStringUntil(',');
        //Read the rest of the input until the newline character
        String rest_of_input = Serial.readStringUntil('\n');
        //That is the destination address
        destination_address = rest_of_input.toInt();
      }
    }
  }

  bool UserInput(){
    return input_string != "";
  }

  bool ComputerInput(){
    return data_length != 0;
  }

  //pop the input string
  String popInput(){
    String temp = input_string;
    input_string = "";
    return temp;
  }
  //copy and pop the input data
  byte *popData(){
    byte *temp = new byte[data_length];
    for(int i = 0; i < data_length; i++){
      temp[i] = input_data[i];
    }
    data_length = 0;
    return temp;
  }
};
*/
// InputCheck inputInstance;
void setup() {
  Serial.begin(115200); // Initialize USB Serial
  lora.begin();
  delay(1000);
}

void loop() {
  checkUserInput();
  if (lora_input.length() > 0 && address.length() > 0 && lora.available()) {
    // lora.sendSingleStr(inputInstance.popInput().c_str(), inputInstance.destination_address);
    // lora.stringPacketWTime(lora_input.c_str(), address.toInt());
    delay(500);
    latency_checkpoint = millis();

    txMessage send_Item;
    strncpy(send_Item.strToSend, lora_input.c_str(), sizeof(send_Item.strToSend) - 1);
    send_Item.addressToSend = address.toInt();

    lora.stringPacketWTime(send_Item.strToSend, send_Item.addressToSend);
    delay(500);
  }
  else {
    loraRead();
  }
}

void loraRead() {
  int address, length, rssi, snr;
  byte* data;

  bool lora_available = lora.read(&address, &length, &data, &rssi, &snr);
  // Serial.println("Read successful:");
  // Serial.println((char*)data);
  if (lora_available && length > 0 && lora.checkChecksum(data, length)) {
    // removed the "&& lora.checkChecksum(data, length)" part from above if statement to troubleshoot IMU
    // Serial.print("Printing data: ");
    char response[200];
    snprintf(response, sizeof(response), "Received--- Address: %d, Length: %d, Data: %s, RSSI: %d, SNR: %d",
      address, length, data, rssi, snr);
    // Serial.println((char*)data);
    Serial.println(response);
    Serial.println("--Packet End--");


    if (latency_checkpoint != 0) {
      Serial.print("Latency: ");
      Serial.println(millis() - latency_checkpoint);
      latency_checkpoint = 0;
    }
  }
}

void checkUserInput() {
  if (Serial.available() > 0) {
    String userInput = Serial.readStringUntil('\n'); // Read the input until newline character

    // Process user input
    if (userInput.length() > 0) {
      int commaIndex = userInput.indexOf(",");
      lora_input = userInput.substring(0, commaIndex);
      address = userInput.substring(commaIndex + 1);
      // Check if the input ends with ":repeat"
      if (userInput.endsWith(":repeat")) {
        // Extract the part before ":"
        int colonIndex = userInput.indexOf(':');
        if (colonIndex != -1) {
          String prefix = userInput.substring(0, colonIndex);
          lora_input = prefix;
          // Set reporting_lock to true
          reporting_lock = true;
          // Use 'prefix' variable as needed
        }
      }
      else {
        reporting_lock = false;
      }

    }
  }
}