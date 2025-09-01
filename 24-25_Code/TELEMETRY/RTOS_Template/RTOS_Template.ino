// Instruct to run on 1 core
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Globals ----------------------------------------------------------------------------------------

/*
Special Words
-----------------------------
SemaphoreHandle_t
QueueHandle_t
TaskHandle_t
TickType_t
BaseType_t
*/

// Tasks ------------------------------------------------------------------------------------------
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

void RTOSTask2(void *parameter)
{
  while (1)
  {
    Serial.println("Task 2 is running");
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

void setup()
{
  Serial.begin(115200);
  // This order of initialization does not have to be followed, but it is good to use these comments for organization purposes

  // Initialize Peripherals -------------------------------------------------------------

  // Create Queues ----------------------------------------------------------------------

  // Create Semaphores & Mutexes --------------------------------------------------------

  // Tasks -----------------------------------------------------------------------------
  BaseType_t result;

  // Task1 -----------------------------------------------------
  result = xTaskCreatePinnedToCore(
      RTOSTask1,     // Function to run
      "RTOS Task 1", // Name of the task
      2048,          // Stack size
      NULL,          // Parameters to pass to the task
      2,             // Priority
      NULL,          // Task handle
      app_cpu);      // Core to run the task on
  if (result != pdPASS)
  {
    Serial.println("Error creating lora task");
    while (1)
      ;
  }

  // Task2 -----------------------------------------------------
  result = xTaskCreatePinnedToCore(
      RTOSTask2,
      "RTOS Task 2",
      2048,
      NULL,
      2,
      NULL,
      app_cpu);
  if (result != pdPASS)
  {
    Serial.println("Error creating lora task");
    while (1)
      ;
  }
}