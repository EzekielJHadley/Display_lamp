#include <stdio.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

//rest server struct
struct LED
{
  byte status;
} led_resource;

//wifi parameter
#define HTTP_REST_PORT    (80)
#define WIFI_RETRY_DELAY  (500)

const char* wifi_ssid = "Teletran2_(2.4)";
const char* wifi_passwd = "R0LL!0U7!!!";

ESP8266WebServer http_rest_server(HTTP_REST_PORT);

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
    "$.post(\"http://" + WiFi.localIP().toString() + "/led\", \"{id:1, gpio:2, status:\" + selectedValue + \"}\", function(data, status){alert(\"data: \" + data + \"\\nStatus: \" + status);});};</script></html>";

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
    Serial.println("error in parsing JSON body");
    http_rest_server.send(400);
  }
  else
  {
    http_rest_server.send(200);
    led_resource.status = (int)jsonBody["status"];
    digitalWrite(LED_BUILTIN, led_resource.status);
    
  }
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
   http_rest_server.handleClient();

}
