#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

#include "V1_SOAR_RTC.h"
#include "sensor_data_types.h"
#include "V1_SOAR_RTOS_SD_CARD.h"


SOAR_RTC timer;
SOAR_SD_CARD sd(D0);
static QueueHandle_t TimeQu;
SemaphoreHandle_t sd_mutex;
const char* TIME_FILEPATH = "/time.csv";

void ReadTimerTask (void* Parameters) {
  while(1) {
  SensorData TimeData;
    // Serial.println("Beginning RTC");
    // char date[20];
    // int time[4];
    // // char* current_date = timer.getDate(date);
    int current_hours = timer.getTimeHours();
    int current_minutes = timer.getTimeMinutes();
    int current_seconds = timer.getTimeSeconds();
    int current_microseconds = timer.getTimeMicroseconds();
    // TimeData.data.rtc.date = 0;
    TimeData.data.rtc.hours = current_hours;
    TimeData.data.rtc.minutes = current_minutes;
    TimeData.data.rtc.seconds = current_seconds;
    TimeData.data.rtc.microseconds = current_microseconds;
    // Serial.println("RTC data read and put in struct");

    if (xSemaphoreTake(sd_mutex, portMAX_DELAY) == pdTRUE) {
      // Serial.println("Beginning SD card Storage");
      String filename = TIME_FILEPATH;
      String data_str;

      data_str = 
      // "2 + 2" + "," + 
      // String(TimeData.data.rtc.date) + "," + 
      String(TimeData.data.rtc.hours) + ","
       + String(TimeData.data.rtc.minutes) + "," + String(TimeData.data.rtc.seconds) + "," + String(TimeData.data.rtc.microseconds);
        
      data_str += "\n";
      // Serial.println("About to put into SD card");
      sd.appendFile(filename.c_str(), data_str.c_str());
      xSemaphoreGive(sd_mutex);
    }
    xQueueSend(TimeQu, &TimeData, portMAX_DELAY);
    // Serial.println("Data sent to SD card");
}
  // vTaskDelay(200 / portTICK_PERIOD_MS);
}

void SensQuReadTask(void* Parameters) {
  while (1) {
    const uint16_t warningThreshold = 8000;
    SensorData sensor_data;
    // Serial.println("Sens queue read task executing");
    if (xQueueReceive(TimeQu, &sensor_data, portMAX_DELAY) == pdTRUE) {
      // Serial.println(sensor_data.data.rtc.date);
      Serial.println(sensor_data.data.rtc.hours);
      Serial.println(sensor_data.data.rtc.minutes);
      Serial.println(sensor_data.data.rtc.seconds);
      Serial.println(sensor_data.data.rtc.microseconds);
      // Serial.println("Hello World");
    }
      // vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  // timer.TimeInitialize();
  timer.adjustTime(4, 3, 2025, 18, 37, 00, 000000);

  sd.begin();
  sd.deleteFile(TIME_FILEPATH);
  sd.writeFile(TIME_FILEPATH, "hours, minutes, seconds, microseconds\n");
  TimeQu = xQueueCreate(10, sizeof(SensorData));
  sd_mutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    ReadTimerTask,
    "Timer Read Task",
    4096,
    NULL,
    3,
    NULL,
    app_cpu
  );

    xTaskCreatePinnedToCore(
    SensQuReadTask,
    "Sensor Queue Read Task",
    4096,
    NULL,
    3,
    NULL,
    0
  );
}

void loop() {
  // Empty, tasks are handled by FreeRTOS
}
