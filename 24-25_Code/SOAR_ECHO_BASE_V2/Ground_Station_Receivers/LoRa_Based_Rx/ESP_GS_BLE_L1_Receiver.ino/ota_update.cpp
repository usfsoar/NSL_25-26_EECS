#include "OTA_Update.h"

const char* loginIndex = "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>SOAR OTA Login</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
             "<td>Username:</td>"
             "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>"; // Your loginIndex content
const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data' id='upload_form'>"
                  "<input type='file' name='update'>"
                  "<input type='submit' value='Update'>"
                "</form>"
                "<div id='prg'>Code Upload Progress: 0%</div>"
                "<script>"
                  "document.querySelector('form').addEventListener('submit', function(e) {"
                    "e.preventDefault();"
                    "var form = document.getElementById('upload_form');"
                    "var data = new FormData(form);"
                    "var xhr = new XMLHttpRequest();"
                    "xhr.open('POST', '/update');"
                    "xhr.upload.addEventListener('progress', function(evt) {"
                      "if (evt.lengthComputable) {"
                        "var per = evt.loaded / evt.total;"
                        "document.getElementById('prg').innerHTML = 'progress: ' + Math.round(per * 100) + '%';"
                      "}"
                    "}, false);"
                    "xhr.send(data);"
                  "});"
                "</script>"; // Your serverIndex content


OTA_Update::OTA_Update(const char* host, const char* ssid, const char* password)
    : host(host), ssid(ssid), password(password) {}

void OTA_Update::Setup() {
    // Connect to WiFi network
    Serial.print("Connecting to: ");
    Serial.print(ssid);
    Serial.print("::");
    Serial.println(password);
    WiFi.begin(ssid, password);
    Serial.println("");

    // Attempt to connect for 30 seconds
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    // Check if connection was successful
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Failed to connect to WiFi. Continuing...");
        return; // Exit the function if connection fails
    }

    // Print connection details
    Serial.print("Connected to: ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

    /*use mdns for host name resolution*/
    if (!MDNS.begin(host)) { //http://esp32.local
        Serial.println("Error setting up MDNS responder!");
        while (1) {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");

    /*return index page which is stored in serverIndex */
    server.on("/", HTTP_GET, [this]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", loginIndex);
    });

    server.on("/serverIndex", HTTP_GET, [this]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
    });

    /*handling uploading firmware file */
    server.on("/update", HTTP_POST, [this]() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart();
    }, [this]() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.printf("Update: %s\n", upload.filename.c_str());
            if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            /* flashing firmware to ESP*/
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) { //true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
        }
    });

    server.begin();
}


void OTA_Update::Handle() {
    server.handleClient();
    delay(1);
}
