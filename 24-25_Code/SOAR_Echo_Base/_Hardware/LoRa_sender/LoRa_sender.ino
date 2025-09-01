#include <SoftwareSerial.h>

#define RX 3
#define TX 2
SoftwareSerial lora(RX, TX); // RX, TX --> physically(RX=2, TX=3)
bool reporting_lock = false;
String lora_input="";
String address="";
void setup() {
  Serial.begin(115200); // Initialize USB Serial
  lora.begin(115200); // Initialize Software Serial
  sendATcommand("AT+ADDRESS=1", 500);
  sendATcommand("AT+BAND=433000000", 500);
  sendATcommand("AT+NETWORKID=5", 500);  
  delay(5000);
}
uint32_t count = 0;
boolean sender=true;
void loop() {
  
  checkUserInput();
  count ++;
  
  if (lora_input && address != ""){
    send_command(lora_input, address);
    if(!reporting_lock){
      lora_input = "";
      address="";
    }
    sender=false;
    count = 0;
  }
  else{
    String incomingString;
    if (lora.available()) {
      incomingString = lora.readString();
      int end_index = incomingString.indexOf('\n');
      incomingString = incomingString.substring(0, end_index);
      Serial.print("<LORA>");
      Serial.print(incomingString);
      Serial.println("</LORA>"); 
      sender=true;
   }
  }
}

void checkUserInput() {
  if (Serial.available() > 0) {
    String userInput = Serial.readStringUntil('\n'); // Read the input until newline character

    // Process user input
    if (userInput.length() > 0) {
      int commaIndex=userInput.indexOf(","); 
      lora_input = userInput.substring(0,commaIndex);
      address=userInput.substring(commaIndex+1);
      sender = true;
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

void send_command(String inputString, String address){
     int len=inputString.length();
     int addressInt=address.toInt();
     Serial.println(inputString);
     char returnedStr[len];
     inputString.toCharArray(returnedStr,len+1);
     Serial.println(returnedStr);
     if (len<=9){
       char tempArray[12+len];
       sprintf(tempArray,"AT+SEND=%d,%d,",addressInt,len);
       strcat(tempArray, returnedStr);
       Serial.println(tempArray);
       sendATcommand(tempArray, 500);
     }else if (len>9 && len<=99){
       char tempArray[13+len];
       sprintf(tempArray,"AT+SEND=%d,%d,",addressInt,len);
       strcat(tempArray, returnedStr);
       Serial.println(tempArray);
       sendATcommand(tempArray, 500);
     }else{
       char tempArray[13+len];
       sprintf(tempArray,"AT+SEND=%d,%d,",addressInt,len);
       strcat(tempArray,char(len));
       strcat(tempArray, returnedStr);
       Serial.println(tempArray);
       sendATcommand(tempArray, 500);
     }
}

//  $GNRMC
// time is 10 chars long starting after 7 characters. Ends at 17 chars
//
String RaspberryIncomingStr(){
    //int strLength=inputStr.length();
    return "GPS";
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

/*for char in dataArray(){
        tempArray.add(char);
        counter+=1;
        if counter==6:
           if tempArray.toString()="$GNRMC"{
              //execute action
           }else if{
              break;
           }*/
