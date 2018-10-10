#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <TM1637Display.h>
#include <RTClib.h>
#include <ESP8266HTTPClient.h>
 
RTC_Millis rtc;

// wifi data
const char* ssid = "Linus Netzwerk";
const char* password = "*********";
const char* host_name = "ESP-UHR-DHCP-73";

// Module connection pins (Digital Pins)
const int CLK = D6; //Set the CLK pin connection to the display
const int DIO = D5; //Set the DIO pin connection to the display

int bri;
int state;

ESP8266WebServer server(8080);

const uint8_t SEG_DONE[] = {
  SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,           // d
  SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,   // O
  SEG_C | SEG_E | SEG_G,                           // n
  SEG_A | SEG_D | SEG_E | SEG_F | SEG_G            // E
  };
    
TM1637Display display(CLK, DIO);

void handleRoot() {
  server.send(200, "text/plain", "Hello from esp8266!");
}

void handle_debug() {
  server.send(200, "text/plain", "Host:\t"+WiFi.hostname()+"\n"+
                                   "WiFi:\t"+ssid+"\t"+
                                   "IP:\t"+WiFi.localIP()+"\n"+
                                   "URI:\t"+server.uri()+"\n"+
                                   "----------------------------\n"+
                                   "Status:\t"+String(state)+"\n"+
                                   "Brightness:\t"+bri+"\n"
                                   );  
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (int i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);

}

void handle_state() {
  server.send(200, "text/plain", String(state));
}

void handle_on() {
  state = 1;
  Serial.print("handle_on "+bri);
  display.setBrightness(bri);
  server.send(200, "text/plain", String(state));
}

void handle_off() {
  state = 0;
  Serial.print("handle_off "+bri);
  display.setBrightness(0);
  server.send(200, "text/plain", String(state));
}

void get_brightness() {
  server.send(200, "text/plain", String(bri));
}

void handle_brightness() {
  bri = server.arg("para").toInt();
  Serial.print("handle_brightness "+bri);
  display.setBrightness(int((bri/36.428)));
  server.send(200, "text/plain", String(bri));
}

void handle_display_number() {
  String payload = "";
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;  //Declare an object of class HTTPClient
    http.begin("http://192.168.178.66/date.php");  //Specify request destination
    int httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      payload = http.getString();   //Get the request response payload
      Serial.println(payload);                     //Print the response payload
    }
    http.end();   //Close connection
  }

  int h = payload.substring(0,1).toInt();
  int m = payload.substring(3,4).toInt();
  int s = payload.substring(6,7).toInt();

  rtc.adjust(DateTime(1996, 8, 17, h, m, s));
  
  server.send(200, "text/plain", "Zeit adjust!");
}

void setup(void){
 
  Serial.begin(115200);
  WiFi.hostname(host_name);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  //rtc.begin(DateTime(F(__DATE__), F(__TIME__))) ;

  String payload = "";
  if (WiFi.status() == WL_CONNECTED) {              //Check WiFi connection status
    HTTPClient http;                                //HTTPClient object
    http.begin("http://192.168.178.66/date.php");   //request (raspberry)
    int httpCode = http.GET();                      
    if (httpCode > 0) {                             //Check returning code
      payload = http.getString();                   //Get response payload
      Serial.println(payload);                      //Print response payload
    }
    http.end();                                     //Close connection
  }

  int h = payload.substring(0,2).toInt();
  int m = payload.substring(3,5).toInt();
  int s = payload.substring(6,8).toInt();

  Serial.println("zeit:");
  Serial.println(h);
  Serial.println(m);
  Serial.println(s);

  rtc.adjust(DateTime(2018, 8, 17, h, m, s));
  
  server.on("/", handleRoot);
  server.on("/bri", get_brightness);
  server.on("/setbri", handle_brightness);
  server.on("/set", handle_display_number);
  server.on("/onClock", handle_on);
  server.on("/offClock", handle_off);
  server.on("/state", handle_state);
  server.on("/debug", handle_debug);
  server.onNotFound(handleNotFound);
  
  // setting start values
  //display brightness (1 to 7)
  display.setBrightness(7);
  //display.setSegments(SEG_DONE);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  DateTime now = rtc.now();
  int t = now.hour() * 100 + now.minute()+1;
  display.showNumberDec(t, true);
  server.handleClient();
  //delay(1); //delay for rtc too (by esp8266)
}
