#if CONFIG_FREERTOS_UNICORE
static const BaseType_t telemetry_cpu = 0;
static const BaseType_t serial_cpu = 0;
#else
static const BaseType_t telemetry_cpu = 0;
static const BaseType_t serial_cpu = 1;
#endif

// #include "sensor_data_types.h"
#include "XBeeArduino.h"
#include "xbee_api_frames.h"

typedef struct
{
  char text[256]; // Buffer to hold the string
} XBeeMsg;

// Globals ----------------------------------------------------------------------------------------
// RTOS Handles -----------------------------------------------------
// Task Handles
TaskHandle_t serialHandle;
TaskHandle_t XBeeHandle;

// Queue Handles
static QueueHandle_t XBee_Qu;
static QueueHandle_t serialQu;


// XBee setup -------------------------------------------------------
HardwareSerial mySerial(0);
Stream* serialPort = &mySerial;
XBeeArduino* xbee;

// Tasks ------------------------------------------------------------------------------------------
// XBee Stuff -------------------------------------------------------
void XBeeTask(void* Parameters) {
  while (1) {
    // unsigned long startTime = xTaskGetTickCount(); // Record start time
    xbee->process();
    // unsigned long endTime = xTaskGetTickCount(); // Record end time

    // unsigned long executionTime = endTime - startTime; // Calculate execution time
    // Serial.print("XBee process execution time: ");
    // Serial.print(pdTICKS_TO_MS(executionTime)); // Convert ticks to milliseconds
    // Serial.println(" ms");

    vTaskDelay(pdMS_TO_TICKS(100));

    // XBeeMsg transmission;
    // if (xQueueReceive(XBee_Qu, &transmission,
    //   pdMS_TO_TICKS(100)) == pdTRUE) {  // poll with timeout
    //   Serial.print("XBeeTask: Data to send: ");
    //   Serial.println(transmission.text);
    //   if (xbee->isConnected()) {
    //     if (!xbee->completeTx(transmission.text)) {
    //       Serial.println("Failed to send data");
    //     }
    //     else {
    //       Serial.println("XBeeTask: Data sent successfully");
    //     }
    //   }
    // }
    // vTaskDelay(pdMS_TO_TICKS(10));              // yield
  }
}
// WARNING: NEED TO ADD A THING TO USE THE ONRECEIVE CALLBACK

void OnReceiveCallback(void* data) {
  // Serial.println("----------------------------------------------------------------------------------------------------");
  // Serial.println("Onreceive callback executed");
  // Serial.print("RX Packet Data: ");
  // Serial.println("\n");
  xbee_api_frame_t* frame = (xbee_api_frame_t*)data;
  for (int i = 0; i < frame->length; i++) {
    Serial.printf("0x%02X ", frame->data[i]);
  }
  Serial.println("\n");
  // Serial.println("----------------------------------------------------------------------------------------------------");
}

void OnSendCallback(void* data) {
  Serial.println("Send callback initiated");
}

// Serial Task ------------------------------------------------------
void serial_read(void* parameters) {
  while (1) {
    // Serial.println("Serial Read Task is running");
    // Read from serial
    if (Serial.available() > 0) {
      String userInput = Serial.readStringUntil('\n');

      if (userInput.length() > 0) {
        Serial.println("Received something from serial");
        int commaIndex = userInput.indexOf(",");

        // Send the data to the XBee
        XBeeMsg message;
        strncpy(message.text, userInput.c_str(), sizeof(message.text) - 1);
        message.text[sizeof(message.text) - 1] = '\0'; // Ensure null-termination
        xQueueSend(XBee_Qu, &message, portMAX_DELAY);
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting setup...");
  // Initialize the XBee --------------------------------------------------------------------------
  xbee = new XBeeArduino(serialPort, 115200, XBEE_STANDARD, OnReceiveCallback, OnSendCallback);
  mySerial.begin(115200, SERIAL_8N1, -1, -1);

  if (!xbee->begin()) {
    Serial.println("Failed to initialize XBee");
    while (1)
      ; // Halt the program on failure
  }


  // RTOS Initialization --------------------------------------------------------------------------
  // Initialize Queues ----------------------------------------------
  XBee_Qu = xQueueCreate(10, sizeof(XBeeMsg)); // Changed to XBeeMsg type
  serialQu = xQueueCreate(5, sizeof(XBeeMsg));
  // Check if Queues were created successfully
  if (XBee_Qu == NULL || serialQu == NULL) {
    Serial.println("Error creating queues");
    while (1);
  }
  else {
    Serial.println("Queues created successfully");
  }



  // Tasks ----------------------------------------------------------------------------------------
  BaseType_t result;
  result = xTaskCreatePinnedToCore(
    XBeeTask,
    "XBee Task",
    16384,
    NULL,
    2,
    &XBeeHandle,
    telemetry_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating serial_read");
    while (1);
  }

  result = xTaskCreatePinnedToCore(
    serial_read,
    "Serial Read",
    2048,
    NULL,
    1,
    &serialHandle,
    serial_cpu
  );
  if (result != pdPASS) {
    Serial.println("Error creating serial_read");
    while (1);
  }

}

void loop() {
}