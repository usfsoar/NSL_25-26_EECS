void setup() {
    Serial.begin(9600); 
}

void loop() {
    // Create a comma-separated string with all the data fields
    String data = "2,2,12,3 ,45,123456,1.2,2.3,3.4,0.1,0.2,0.3,9.8,0.0,0.0,1.0,0.0,0.0,0.0,0.01,0.02,0.03,100.5,25.0,1013.25,OK,37.7749,N,122.4194,W";
    Serial.println(data);
    delay(2000); // Wait for 2 seconds before sending again
}