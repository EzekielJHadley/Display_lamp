//Neopixel libraries
//#include <Adafruit_NeoPixel.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include <stdio.h>

#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiUdp.h>

#include <ArduinoJson.h>
#include "wifi_credentials.h"


void rainbow_chase();
void set_color();
void post_led();
void all_led_off();
void disp_error();
void init_con_vs_bot();
void UpdateAnim(AnimationParam param);
void init_rnd_color();
void FadeToWhite(AnimationParam param);
void random_led(AnimationParam param);


//define the data pin and number of LEDs
#define NEO_Dout        (RX)
#define NEO_LED_COUNT   (60)
#define FRAME_RATE_ms   (50)


//rest server struct
struct LED
{
  uint8_t mode = 0;
  byte status;
  float hue = 0.25;
  float sat = 1.0;
  float bright = 0.5;
} led_resource;

enum PATTERN_NUM
{
  OFF = 0,
  CON_VS_BOT,
  RND_COLOR,

  ERROR,
  PATTERN_MAX
};

//wifi parameter
#define HTTP_REST_PORT    (80)
#define WIFI_RETRY_DELAY  (500)
#define SW_VERS           ("0.0.3")

const char* wifi_ssid = WIFI_SSID;
const char* wifi_passwd = WIFI_PASSWD;
const char* sw_vers = SW_VERS;

ESP8266WebServer http_rest_server(HTTP_REST_PORT);


//global variables for neopixel 
uint8_t led_brightness = 40; //max 255
//Adafruit_NeoPixel strip(NEO_LED_COUNT, NEO_Dout, NEO_GRB + NEO_KHZ800); //the last two are built in NEO pixel definitions
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NEO_LED_COUNT);
NeoPixelAnimator *animations;
NeoPixelAnimator con_vs_bot_animator(1);
NeoPixelAnimator rnd_color_animator(2);


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
  Serial.println(wifi_ssid);
  Serial.print("--- IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("--- Version: ");
  Serial.println(sw_vers);
}

void init_OTA()
{
  // Add optional callback notifiers
  ESPhttpUpdate.onStart(update_started);
  ESPhttpUpdate.onEnd(update_finished);
  ESPhttpUpdate.onProgress(update_progress);
  ESPhttpUpdate.onError(update_error);
  ESPhttpUpdate.rebootOnUpdate(true);
}

//this sets the 
void config_rest_server_routing()
{
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
  init_OTA();

  config_rest_server_routing();

  http_rest_server.begin();

  Serial.println("REST server started!");

  //set led pin mode
  pinMode(LED_BUILTIN, OUTPUT);

  //set the LED to a default value on boot
  led_resource.status=0;
  digitalWrite(LED_BUILTIN, led_resource.status);

  init_con_vs_bot();
  init_rnd_color();
  //set con_vs_bot as default animation
  animations = &con_vs_bot_animator;
}

//store the last frame draw
bool do_once = false;
//unsigned long last_draw = 0;

void loop() 
{
  if(not do_once)
  {
    WiFiClient client;
    ESPhttpUpdate.update(client, "192.168.3.159", 5000, "/arduino.bin", sw_vers);
    do_once=true;
  }

//  unsigned long current_time = millis();
  // if( strip.CanShow())//(current_time - last_draw) >= FRAME_RATE_ms) //draw the image ever FRAME_RATE_ms milliseconds
  // {
  //rainbow_chase();
  animations->UpdateAnimations();
  strip.Show();
//  last_draw = current_time;
  // }
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

// void Update_CON_VS_BOT(AnimationParam param)
// {
//     // apply a exponential curve to both front and back
//     float progress = NeoEase::ExponentialInOut(param.progress);
//     // lerp between Red and Green
//     RgbColor color = RgbColor::LinearBlend(RgbColor(255,0,0), RgbColor(0,255,0), progress);
//     // in this case, just apply the color to first pixel
//     strip.ClearTo(color);
//     if (param.state == AnimationState_Completed)
//     {
//       animations.RestartAnimation(param.index);
//     }
// }

void all_led_off()
{
  animations->Pause();
  strip.ClearTo(RgbColor(0,0,0));
}

void disp_error()
{
  animations->Pause();
  strip.ClearTo(RgbColor(255, 0, 0));
}

//set up the animations and define each update function bellow
void init_con_vs_bot()
{
  con_vs_bot_animator.StartAnimation(0, 1000, update_con_vs_bot);
}

void update_con_vs_bot(AnimationParam param)
{
  uint16_t rotate_distance = uint16_t(param.progress * NEO_LED_COUNT);
  //set half to purple
  strip.ClearTo(RgbColor(135, 18, 182), 0, uint16_t(NEO_LED_COUNT/2.0 - 1));
  //other half to red
  strip.ClearTo(RgbColor(190, 11, 36), uint16_t(NEO_LED_COUNT/2.0), NEO_LED_COUNT-1);
  strip.RotateLeft(rotate_distance);
  if (param.state == AnimationState_Completed)
  {
    animations->RestartAnimation(param.index);
  }
}

void init_rnd_color()
{
  rnd_color_animator.StartAnimation(0, 10, FadeToWhite);
  rnd_color_animator.StartAnimation(1, 100, random_led);
}

void FadeToWhite(AnimationParam param)
{
  if (param.state == AnimationState_Completed)
  {
    RgbColor color;
    for( uint16_t pixelIndex = 0; pixelIndex < NEO_LED_COUNT; pixelIndex++)
    {
      color = strip.GetPixelColor(pixelIndex);
      color.Lighten(5);
      strip.SetPixelColor(pixelIndex, color);
    }
    animations->RestartAnimation(param.index); //for reusability of the function, use the pointer
  }
}

void random_led(AnimationParam param)
{
  if( param.state == AnimationState_Started )
  {
    //set the seed
    randomSeed(millis());
  }

  if (param.state == AnimationState_Completed)
  {
    //get the position
    uint16_t pixelIndex = uint16_t(random(NEO_LED_COUNT));
    float hue = random(360)/360.0f;
    strip.SetPixelColor(pixelIndex, HslColor(hue, 1.0f, 0.3f));
    animations->RestartAnimation(param.index);
  }
}

void restart_animation()
{
  if(animations->IsPaused())
  {
    animations->Resume();
  }
  uint16_t channel_index = 0;
  animations->RestartAnimation(channel_index);
  while(animations->NextAvailableAnimation(&channel_index, channel_index)) //the pointer isn't overwritten until function returns
  {
    animations->RestartAnimation(channel_index);
  }

}

//{mode:PATTERN_NUM, args:{ mode arguments } }
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
    if(jsonBody.containsKey("mode"))
    {
      switch((uint8_t)jsonBody["mode"])
      {
        case OFF:
          all_led_off();
          break;
        case CON_VS_BOT:
          animations = &con_vs_bot_animator;
          restart_animation();
          break;
        case RND_COLOR:
          animations = &rnd_color_animator;
          restart_animation();
          break;
        case ERROR:
          disp_error();
          break;
        case PATTERN_MAX:
        default:
          //do nothing
          break;
      }
      //restart all animation channels
      http_rest_server.send(200);
    }
    else
    {
      http_rest_server.send(400);
    }    

    // http_rest_server.send(200);
    // led_resource.status = (int)jsonBody["status"];
    // digitalWrite(LED_BUILTIN, led_resource.status);

    // //parse the HSV value for one of the color inputs
    // char output[100];
    // sprintf(output,  "h:%f s:%f v:%f", (float)jsonBody["color0"]["h"], (float)jsonBody["color0"]["s"], (float)jsonBody["color0"]["v"]);
    // Serial.println(output);
    // led_resource.hue = (uint16_t)(0xFFFF * ((float)jsonBody["color0"]["h"])/360.0); //map hue, from 0-360 to a 16bit hex value
    // led_resource.sat = (uint8_t)(255.0 * (float)jsonBody["color0"]["s"]);
    // led_resource.bright = (uint8_t)(255.0 * (float)jsonBody["color0"]["v"]);
    // sprintf(output,  "h:%u s:%u v:%u",  led_resource.hue,  led_resource.sat,  led_resource.bright);
    // Serial.println(output);
  }
}

void update_started()
{
  //set all to off
  strip.ClearTo(RgbColor(0, 0, 0));
  Serial.println("started running update");
}

void update_finished()
{
  //set all to green
  strip.ClearTo(RgbColor(0, 255, 0));
  Serial.println("Update finished");
}


void update_progress(int cur, int total)
{
  uint16_t percent_done = uint16_t(cur * (1.0 * NEO_LED_COUNT)/total);
  if(strip.CanShow())
  {
    //load with a blue
    strip.ClearTo(RgbColor(0, 0, 128), 0, percent_done);
    strip.Show();
  }
  else
  {
    Serial.println("Cannot show");
  }
  Serial.printf("(%d)CALLBACK:  HTTP update process at %d of %d bytes (0x%X)...\n", millis(), cur, total, percent_done);
}

void update_error(int err) 
{
  //set all to red
  disp_error();
  Serial.printf("CALLBACK:  HTTP update fatal error code %d\n", err);
}