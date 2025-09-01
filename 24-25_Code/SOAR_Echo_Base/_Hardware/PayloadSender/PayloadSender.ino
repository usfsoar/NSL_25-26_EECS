#include "SOAR_Lora.h"

#define RX 3
#define TX 2

SOAR_Lora lora("6", "5", "420000000"); // LoRa
bool reporting_lock = false;
String lora_input="";
String address="";
uint32_t latency_checkpoint = 0;
class InputCheck{
  private:
  public:
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

InputCheck inputCheck;
void setup() {
  Serial.begin(115200); // Initialize USB Serial
  lora.begin();
  delay(1000);
}
void loop() {
  inputCheck.check();

  if (inputCheck.UserInput()){
    lora.sendSingleStr(inputCheck.popInput().c_str(), inputCheck.destination_address);
    latency_checkpoint = millis();
  }
  else if(inputCheck.ComputerInput()){
    lora.beginPacket();
    lora.sendBytes(inputCheck.popData(), inputCheck.data_length);
    lora.endPacket(inputCheck.destination_address);
    latency_checkpoint = millis();
  }
  else{
    
    if(lora.available()){
      int address, length, rssi, snr;
      byte *data;
      bool valid_data = lora.read(&address, &length, &data, &rssi, &snr) && lora.checkChecksum(data, length);
      
      if(latency_checkpoint != 0){
        Serial.print("Latency: ");
        Serial.println(millis() - latency_checkpoint);
        latency_checkpoint = 0;
      }
      //{valid_data 1 byte}{address 2 bytes}{length 2 bytes}{data_bytes unknown # of bytes};{2 byte checksum}{2 byte rssi}{2 byte snr}\n
      Serial.write(0x05);
      Serial.write(0x15);
      Serial.write(valid_data ? 0x01 : 0x00); // 1 byte (0x01 if valid data, 0x00 if invalid data
      Serial.write((address >> 8) & 0xFF);
      Serial.write(address & 0xFF);
      Serial.write((length >> 8) & 0xFF);
      Serial.write(length & 0xFF);
      for(int i = 0; i < length-2; i++){
        Serial.write(data[i]);
      }
      Serial.write(data[length-2]);
      Serial.write(data[length-1]);
      Serial.write((rssi >> 8) & 0xFF);
      Serial.write(rssi & 0xFF);
      Serial.write((snr >> 8) & 0xFF);
      Serial.write(snr & 0xFF);

      
      
   }
  }
  lora.handleQueue();
}



void checkUserInput() {
  if (Serial.available() > 0) {
    String userInput = Serial.readStringUntil('\n'); // Read the input until newline character

    // Process user input
    if (userInput.length() > 0) {
      int commaIndex=userInput.indexOf(","); 
      lora_input = userInput.substring(0,commaIndex);
      address=userInput.substring(commaIndex+1);
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
      else{
        reporting_lock = false;
      }
    }
  }
}