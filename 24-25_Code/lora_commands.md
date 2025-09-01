## Drone

## Deployment

| Command | Definition                | Response                                                        | Definition                                 |
| ------- | ------------------------- | --------------------------------------------------------------- | ------------------------------------------ |
| `PI`    | Ping                      | `PO{T-time}`                                                    | Pong with time                             |
| `AS`    | Altitude Single           | `AS{Altitude-float}{T-time}`                                    | Altitude Single Data with time             |
| `AR`    | Altitude Repeat           | `AR{T-time}`                                                    | Altitude Repeat Received with time         |
| `AW`    | Write altitude thresholds | `AW{T-time}`                                                    | Thresholds written with time               |
|         |                           | `AF{T-time}`                                                    | Failed to set thresholds with time         |
| `AT`    | Get altitude thresholds   | `AT{H1-float}{H2-float}{H3-float}{T-time}`                      | Thresholds data with time                  |
| `DP`    | Deploy                    | `DP{T-time}`                                                    | Deploy Received with time                  |
| `DS`    | Deploy Status             | `DS{Status-uint8_t}{T-time}`                                    | Deploy Status Data with time               |
| `DR`    | Deploy Status Repeat      | `DR{T-time}`                                                    | Deploy Status Repeat Received with time    |
| `DT`    | Deploy Stop               | `DT{T-time}`                                                    | Deploy Stop Received with time             |
| `DR`    | Deploy Reset              | `DR{T-time}`                                                    | Deploy Reset Received with time            |
| `DC`    | Deploy Retract            | `DC{T-time}`                                                    | Deploy Retract Received with time          |
| `LI`    | Distance sensor           | `LI{Distance-uint16_t}{T-time}`                                 | Distance sensor data with time             |
| `LR`    | Distance sensor repeat    | `LR{T-time}`                                                    | Distance sensor repeat received with time  |
| `IS`    | All info single           | `IS{Altitude-float}{Distance-uint16_t}{Status-uint8_t}{T-time}` | All info data with time                    |
| `IR`    | All info repeat           | `IR{T-time}`                                                    | All info repeat received with time         |
| `JF`    | Jog Forward               | `JF{T-time}`                                                    | Jog Forward Received with time             |
| `JR`    | Jog Reverse               | `JR{T-time}`                                                    | Jog Reverse Received with time             |
| `RS`    | Stop any repeating data   | `RS{T-time}`                                                    | Stop any repeating data received with time |
| `NH`    | Not handled (n bytes)     | `NH{Command-nB}{T-time}`                                        | Not handled command with time              |

By making these adjustments, the `{T-time}` token is consistently placed at the end of each response definition.

## Recovery

| Command                             | Definition                         | Response                                                                                                                                                                           | Definition                     |
| ----------------------------------- | ---------------------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------ |
| `PI`                                | Ping                               | `PO`                                                                                                                                                                               | Pong                           |
| `GS`                                | GPS Single                         | `GS{T-time}`                                                                                                                                                                       | GPS Single Received            |
|                                     |                                    | `GS{NMEA String}{T-Time}`                                                                                                                                                          | GPS NEMA reponse               |
|                                     |                                    | `GSF{T-time}`                                                                                                                                                                      | GPS Failed to get a fix        |
| `GR`                                | GPS Repeat                         | `GR{T-Time}`                                                                                                                                                                       | GPS Repeat Received            |
| `BS`                                | IMU Single                         | `BS{LinearX-float}{LinearY-float}{LinearZ-float}{GravityX-float}{GravityY-float}{GravityZ-float}{GyroX-float}{GyroY-float}{GyroZ-float}{T-time}`                                   | IMU Data (14 bytes total)      |
| `BR`                                | IMU Repeat                         | `BR{T-time}`                                                                                                                                                                       | IMU Repeat Received            |
| `AS`                                | Altitdue Single                    | `AS{T-time}`                                                                                                                                                                       | Altitude Response Received     |
|                                     |                                    | `AS{altitue-float}{T-time}`                                                                                                                                                        | Altitude response              |
|                                     |                                    | `ASF{T-time}`                                                                                                                                                                      | Altitude failed to read        |
| `AR`                                | Altitude Repeat                    | `AR{T-time}`                                                                                                                                                                       | Altitude repeat received       |
| `IS`                                | All info single                    | `IS{GPS NMEA string};{altitude-float}{LinearX-float}{LinearY-float}{LinearZ-float}{GravityX-float}{GravityY-float}{GravityZ-float}{GyroX-float}{GyroY-float}{GyroZ-float}{T-time}` | All info response              |
| `IR`                                | All info repeat                    | `IR`                                                                                                                                                                               | All info repeat received       |
| `PA{selection-int}`                 | Play audio with index of selection | `PA{selection int}`                                                                                                                                                                | Play audio received with index |
| `WR{ssid string};{password string}` | Retry wifi connection              | `WR{ssid-string};{password-string}`                                                                                                                                                | Wifi retry received            |

## LoRa Telemetry Commands

| Command | Definition         | Response                                                                                                                               | Definition                     |
| ------- | ------------------ | -------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------ |
| `PI`    | Ping               | `PO{T-time}`                                                                                                                           | Pong with time                 |
| `AB`    | Activate Parachute | `AB{T-time}`                                                                                                                           | Parachute Activated with time  |
| `JL`    | Jettison Lock      | `JL{T-time}`                                                                                                                           | Jettison Locked with time      |
| `JT`    | Jettison Trigger   | `JT{T-time}`                                                                                                                           | Jettison Triggered with time   |
| `SS`    | Send Sensor Data   | `SS{T-time}{AccelX-float}{AccelY-float}{AccelZ-float}{GyroX-float}{GyroY-float}{GyroZ-float}{Temp-float}{Sound-float}{Pressure-float}` | Sensor Data with time          |
| `SR`    | Stemnaut Repeat    | `SR{T-time}`                                                                                                                           | Repeat Stemnaut Data with time |
| `GS`    | GPS Data Send      | `GS{T-time}`                                                                                                                           | GPS Data Sent with time        |
| `GR`    | GPS Repeat         | `GR{T-time}`                                                                                                                           | Repeat GPS Data with time      |
| `NH`    | Not Handled        | `NH{Command-nB}{T-time}`                                                                                                               | Not handled command with time  |
