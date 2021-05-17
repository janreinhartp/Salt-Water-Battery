#include <WiFi.h>
#include <SPIFFS.h>
#include <ESPAsyncWebServer.h>

#include <ErriezDS1302.h>

// Connect DS1302 data pin to Arduino DIGITAL pin
#if defined(ARDUINO_ARCH_AVR)
#define DS1302_CLK_PIN      2
#define DS1302_IO_PIN       3
#define DS1302_CE_PIN       4
#elif defined(ARDUINO_ARCH_ESP8266)
// Swap D2 and D4 pins for the ESP8266, because pin D2 is high during a
// power-on / MCU reset / and flashing. This corrupts RTC registers.
#define DS1302_CLK_PIN      D4 // Pin is high during power-on / reset / flashing
#define DS1302_IO_PIN       D3
#define DS1302_CE_PIN       D2
#elif defined(ARDUINO_ARCH_ESP32)
#define DS1302_CLK_PIN      13
#define DS1302_IO_PIN       12
#define DS1302_CE_PIN       14
#else
#error #error "May work, but not tested on this target"
#endif

// Create DS1302 RTC object
ErriezDS1302 rtc = ErriezDS1302(DS1302_CLK_PIN, DS1302_IO_PIN, DS1302_CE_PIN);

// Global date/time object
struct tm dt;

// Constants
const char *ssid = "Salt Water Battery Lamp";
const char *password =  "123456789";

const char* PARAM_INPUT_1 = "TIMER";

String Time;

double timer = 0.1;
int timerOnMinutes = 0;

// Globals
AsyncWebServer server(80);

// Callback: send homepage
void onIndexRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/index.html", "text/html");
}

// Callback: send style sheet
void onCSSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/bootstrap.css", "text/css");
}

// Callback: send style sheet
void onJSRequest(AsyncWebServerRequest *request) {
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(SPIFFS, "/bootstrap.js", "text/javascript");
}

void onTimeRequest(AsyncWebServerRequest *request) {
  getTime();
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(200, "text/plain", Time);
}

void onTimerRequest(AsyncWebServerRequest *request) {
 
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }

  request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");

  timer = inputMessage.toDouble();
  Serial.println(inputMessage);
}




// Callback: send 404 if requested file does not exist
void onPageNotFound(AsyncWebServerRequest *request) {
  
  IPAddress remote_ip = request->client()->remoteIP();
  Serial.println("[" + remote_ip.toString() +
                  "] HTTP GET request of " + request->url());
  request->send(404, "text/plain", "Not found");
}

void rtcInit()
{
    // Initialize RTC
    while (!rtc.begin()) {
        Serial.println(F("RTC not found"));
        delay(3000);
    }

    // Enable RTC clock
    rtc.clockEnable(true);
}

int hourT = 0;
int minT = 0;
int secT = 0;


int hourTS = 0;
int minTS = 0;
int secTS = 0;

void getTime() {
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  // Get date/time
    // Get time from RTC
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    if (!rtc.getTime(&hour, &min, &sec)) {
        Serial.println(F("Get time failed"));
    } else {
        
        Time = hour;
        hourT = hour;
        Time +=":";
       Time += min;
       minT = min;
        Time +=":";
        Time += sec;
        secT = sec;
    }
}



bool timerFlag= false;

void createTime(){
getTime();

timerOnMinutes = timer * 60;

if(timerOnMinutes > 60){
  hourTS = hourT + (timerOnMinutes/60);
  if(hourTS > 24){
    hourTS -= 24;
  }
  minTS = minT;
  secTS = secT;

}else{
  hourTS = hourT;
  minTS = minT + timerOnMinutes;
  secTS = secT;
}

timerFlag = true;
}


void runTimer(){
  getTime();
  if(timerFlag == true){
    if(hourT == hourTS && minT == minTS && secT == secTS){
      timerFlag = false;
    }else{
      timerRelayOn();
    }
  }else{
    timerRelayOff();
  }
}

void timerRelayOn(){
  Serial.println("Relay ON Time OFF");
  Serial.print(hourTS);
  Serial.print(":");
  Serial.print(minTS);
  Serial.print(":");
  Serial.println(secTS);
  Serial.println("Current Time");
  Serial.print(hourT);
  Serial.print(":");
  Serial.print(minT);
  Serial.print(":");
  Serial.println(secT);


}
void timerRelayOff(){
  Serial.println("Relay OFF");
  Serial.print(hourT);
  Serial.print(":");
  Serial.print(minT);
  Serial.print(":");
  Serial.println(secT);
}



void setup() {


  // Start Serial port
  Serial.begin(115200);
// rtcInit();
  // Make sure we can read the file system
  if( !SPIFFS.begin()){
    Serial.println("Error mounting SPIFFS");
    while(1);
  }

  // Start access point
  WiFi.softAP(ssid, password);

  // Print our IP address
  Serial.println();
  Serial.println("AP running");
  Serial.print("My IP address: ");
  Serial.println(WiFi.softAPIP());

  // On HTTP request for root, provide index.html file
  server.on("/", HTTP_GET, onIndexRequest);

  // On HTTP request for style sheet, provide style.css
  server.on("/bootstrap.css", HTTP_GET, onCSSRequest);
  
  // On HTTP request for style sheet, provide style.css
  server.on("/bootstrap.js", HTTP_GET, onJSRequest);

    // Route to set GPIO to HIGH
  server.on("/TIME", HTTP_GET, onTimeRequest);

   // Route to set GPIO to HIGH
  server.on("/get", HTTP_GET, onTimerRequest);
  
  // Handle requests for pages that do not exist
  server.onNotFound(onPageNotFound);

  // Start web server
  server.begin();

  // Start WebSocket server and assign callback
  //createTime();
}

void loop() {
  
 //runTimer();

 delay(2000);
}
