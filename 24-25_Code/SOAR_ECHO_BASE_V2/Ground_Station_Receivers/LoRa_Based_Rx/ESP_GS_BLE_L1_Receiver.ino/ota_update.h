#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

class OTA_Update {
public:
    OTA_Update(const char* host, const char* ssid, const char* password);

    void Setup();
    void Handle();

private:
    const char* host;
    const char* ssid;
    const char* password;
    WebServer server{80};
};

#endif // OTA_UPDATE_H
