#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//static const int led_pin = 2;
int delayTime = 500;
bool loopControl = true;
const TickType_t xDelay = 500 / portTICK_PERIOD_MS;

void startTask1(void *parameter) {
  while(loopControl) {

    if (Serial.available() > 0) {  // Check if data is available
      String receivedData = Serial.readStringUntil('\n'); // Read input
      receivedData.trim(); // Remove spaces and newline

      if (receivedData == "on") {
        digitalWrite(LED_BUILTIN, LOW); 
        Serial.println("Light is turned ON");
      }

      else if (receivedData == "off") {
        digitalWrite(LED_BUILTIN, HIGH);
        Serial.println("Light is turned OFF");
      }

      else if (receivedData == "n" || receivedData == "N"  ) {
        Serial.println("Exiting...");
        loopControl = false;
      }

      else {
        Serial.println("Invalid command! Use 'on', 'off', or 'N'.");
      }

    }

    else{
      Serial.println("constant print");
    }

    vTaskDelay(xDelay);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  xTaskCreatePinnedToCore(startTask1, "Task 1", 1024, NULL, 1, NULL, app_cpu);

}

void loop() {
  // put your main code here, to run repeatedly:

}