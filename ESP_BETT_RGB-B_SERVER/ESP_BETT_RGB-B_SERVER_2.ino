#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ChainableLED.h>

#define NUM_LEDS 1
#define LED D0 // Led in NodeMCU at pin GPIO16 (D0).

#define max(a,b) ((a)>(b)?(a):(b))

String hexString = "000000"; //Define inititial color here (hex value), 080100 would be a calm warmtone i.e.

int state;

int r;
int g;
int b;

float R;
float G;
float B;

int x;
int V;

/* https://github.com/pjpmarques/ChainableLED
class ChainableLED {
public:
  ChainableLED(byte clk_pin, byte data_pin, byte number_of_leds);

  void init();
  void setColorRGB(byte led, byte red, byte green, byte blue);
  void setColorHSB(byte led, float hue, float saturation, float brightness);
}
*/

ChainableLED leds(12, 13, NUM_LEDS);

/*Button für on/off*/
volatile unsigned long alteZeit=0, entprellZeit=20;
/*Ende Button*/

///// WiFi SETTINGS - Replace with your values /////////////////
const char* ssid = "Linus Netzwerk";
const char* password = "*********";
const char* host_name = "ESP-BETT-DHCP-76";
////////////////////////////////////////////////////////////////////

ESP8266WebServer server(8080);

void handleRoot() {
  server.send(200, "text/plain", "Hello from esp8266!");
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void allOff() {
  state = 0;
  leds.setColorRGB(0,0,0,0);
}

//Write requested hex-color to the pins (10bit pwm)
void setHex() {
  state = 1;
  long number = (long) strtol( &hexString[0], NULL, 16);
  r = number >> 16;
  g = number >> 8 & 0xFF;
  b = number & 0xFF;
  leds.setColorRGB(0,r,g,b);
}

//Compute current BRIGHTNESS value
void getV() {
  //R = roundf(r/10.23);  //for 10bit pwm, was (r/2.55); //10.23
  //G = roundf(g/10.23);  //for 10bit pwm, was (g/2.55); //10.23
  //B = roundf(b/10.23);  //for 10bit pwm, was (b/2.55); //10.23
  //x = max(R,G);
  //V = max(x, B);
  x = max(r,g);
  V = max(x, b);
  V = V*4;
}

//For serial debugging only
void showValues() {
  Serial.print("Status on/off: " + String(state));
  Serial.print("RGB color: " + String(r) +" , " + String(g) +" , " + String(b)); 
  Serial.print("Hex color: " + hexString);
  getV();
  Serial.print("Brightness: " + String(V));
  Serial.println();
}

void setup(void){
  pinMode(LED, OUTPUT);
  digitalWrite(LED, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.hostname(host_name);
  WiFi.begin(ssid, password);
  Serial.println("");
  setHex();
  getV();
  state=0;
  leds.init();
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
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

  server.on("/", handleRoot);

  server.on("/test", [](){
    server.send(200, "text/plain", "Hi there here is the ESP_BETT");
  });

  server.on("/debug", [](){
    digitalWrite(LED, 1);
    server.send(200, "text/plain", "Host:\t"+WiFi.hostname()+"\n"+
                                   "WiFi:\t"+ssid+"\t"+
                                   "IP:\t"+WiFi.localIP()+"\n"+
                                   "URI:\t"+server.uri()+"\n"+
                                   "----------------------------\n"+
                                   "Status:\t"+String(state)+"\n"+
                                   "Color:\t"+hexString+"\n"+
                                   "Brightness:\t"+String(V)+"\n"
                                   );
  });
  
  server.on("/bright", [](){
    getV();
    server.send(200, "text/plain", ""+String(V));
  });

  server.on("/color", [](){
    server.send(200, "text/plain", ""+hexString);
  });

  server.on("/status", [](){
    server.send(200, "text/plain", String(state));
  });

  server.on("/set", [](){
    hexString = "";
    hexString = server.arg(0);
    setHex();
    server.send(200, "text/plain", ""+hexString);
  });

  server.on("/onLED", [](){
    setHex();
    server.send(200, "text/plain", String(state));
  });
  
  server.on("/offLED", [](){
    allOff();
    server.send(200, "text/plain", String(state));
  });

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
  delay(1);
}
