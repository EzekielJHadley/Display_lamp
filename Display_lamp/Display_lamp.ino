//Neopixel libraries
//#include <Adafruit_NeoPixel.h>
#include <NeoPixelBus.h>
#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>


void rainbow_chase();
void set_color();
void send_html_page();
void post_led();

//define the data pin and number of LEDs
#define NEO_Dout        (RX)
#define NEO_LED_COUNT   (60)


//rest server struct
struct LED
{
  uint8_t mode = 0;
  byte status;
  float hue = 1.0;
  float sat = 1.0;
  float bright = 0.5;
} led_resource;

//wifi parameter
#define HTTP_REST_PORT    (80)
#define WIFI_RETRY_DELAY  (500)

const char* wifi_ssid = "Teletran2_(2.4)";
const char* wifi_passwd = "R0LL!0U7!!!";

ESP8266WebServer http_rest_server(HTTP_REST_PORT);


//global variables for neopixel 
uint8_t led_brightness = 50; //max 255
//Adafruit_NeoPixel strip(NEO_LED_COUNT, NEO_Dout, NEO_GRB + NEO_KHZ800); //the last two are built in NEO pixel definitions
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NEO_LED_COUNT);

void init_wifi()
{
  Serial.println("");
  Serial.println("Connecting to WiFi AP........");

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_passwd);
  while(WiFi.status() != WL_CONNECTED)
  {
    delay(WIFI_RETRY_DELAY);
    Serial.print("^");
  }

  Serial.print("Connected to ");
  Serial.print(wifi_ssid);
  Serial.print("--- IP: ");
  Serial.println(WiFi.localIP());
}

//this sets the 
void config_rest_server_routing()
{
  http_rest_server.on("/", HTTP_GET, send_html_page);
  http_rest_server.on("/led", HTTP_POST, post_led);
  http_rest_server.on("/led", HTTP_PUT, post_led);//i odn't intend to differentiate between PUTs and POSTs
}


void setup() 
{
  Serial.begin(115200);
  
  //init the neo pixels
  strip.Begin();
  strip.Show(); //turns off all pixels if nothing is loaded in
  //strip.setBrightness(led_brightness);

  init_wifi();

  config_rest_server_routing();

  http_rest_server.begin();

  Serial.println("REST server started!");

  //set led pin mode
  pinMode(LED_BUILTIN, OUTPUT);

  //set the LED to a default value on boot
  led_resource.status=0;
  digitalWrite(LED_BUILTIN, led_resource.status);
}

void loop() 
{
  //rainbow_chase();
  set_color();
  http_rest_server.handleClient();
  yield();
}

void rainbow_chase()
{
  for(int i = 0; i < NEO_LED_COUNT; i++)
  {
      strip.SetPixelColor(i, HsbColor(led_resource.hue, led_resource.sat, led_resource.bright));
      //strip.gamma32(strip.ColorHSV(led_resource.hue + (1092*i))));
  }
  led_resource.hue += 40;
  strip.Show();
  
  //delay(50);
  yield();
}

void set_color()
{
  //strip.fill(strip.gamma32(strip.ColorHSV(led_resource.hue, led_resource.sat, led_resource.value)));
  for(int i = 0; i < NEO_LED_COUNT; i++)
  {
    yield();
    strip.SetPixelColor(i, HsbColor(led_resource.hue, led_resource.sat, led_resource.bright));
    //strip.gamma32(strip.ColorHSV(led_resource.hue, led_resource.sat, led_resource.value))); //
  }
  yield();
  strip.Show();
}

//servers the html page that can control this device
void send_html_page()
{
  //turn off CORs
  http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");
  String htmlPage = 
    String("<!DOCTYPE html><html><head>") +
    "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js\"></script><body>" +
    "<br><p>Turn LED on or off:</p>" +
    "<input type=\"radio\" name=\"choice\" value=\"0\"" + ((led_resource.status==0)?"checked":"") + "> ON" +
    "<input type=\"radio\" name=\"choice\" value=\"1\"" + ((led_resource.status==1)?"checked":"") + "> OFF" +
    "<br><button onclick=\"POST_STATE()\">POST</button></body>" +
    "<script>function POST_STATE(){" +
    "var selectedValue = $(\'input[name=\"choice\"]:checked\').attr(\"value\");" +
    "$.post(\"http://" + WiFi.localIP().toString() + "/led\", \"{gpio:2, status:\" + selectedValue + \"}\", function(data, status){alert(\"data: \" + data + \"\\nStatus: \" + status);});};</script></html>";

  http_rest_server.send(200, "text/html", htmlPage);
}

void post_led()
{
  StaticJsonDocument<500> jsonBody;
  String post_body = http_rest_server.arg("plain");
  Serial.println(post_body);

  //convert the message into something we can use
  DeserializationError error = deserializeJson(jsonBody, post_body);

  http_rest_server.sendHeader("Access-Control-Allow-Origin", "*");

  if(error)
  {
    Serial.print("error in parsing JSON body: ");
    Serial.println(error.c_str());
    http_rest_server.send(400);
  }
  else
  {
    http_rest_server.send(200);
    led_resource.status = (int)jsonBody["status"];
    digitalWrite(LED_BUILTIN, led_resource.status);

    //parse the HSV value for one of the color inputs
    char output[100];
    sprintf(output,  "h:%f s:%f v:%f", (float)jsonBody["color0"]["h"], (float)jsonBody["color0"]["s"], (float)jsonBody["color0"]["v"]);
    Serial.println(output);
    led_resource.hue = (uint16_t)(0xFFFF * ((float)jsonBody["color0"]["h"])/360.0); //map hue, from 0-360 to a 16bit hex value
    led_resource.sat = (uint8_t)(255.0 * (float)jsonBody["color0"]["s"]);
    led_resource.bright = (uint8_t)(255.0 * (float)jsonBody["color0"]["v"]);
    sprintf(output,  "h:%u s:%u v:%u",  led_resource.hue,  led_resource.sat,  led_resource.bright);
    Serial.println(output);
  }
}
