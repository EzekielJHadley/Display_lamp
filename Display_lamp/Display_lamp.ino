//Neopixel libraries
#include <Adafruit_NeoPixel.h>


//define the data pin and number of LEDs
#define NEO_Dout        (D5)
#define NEO_LED_COUNT   (60)


//global variables for neopixel 
uint8_t led_brightness = 50; //max 255
Adafruit_NeoPixel strip(NEO_LED_COUNT, NEO_Dout, NEO_GRB + NEO_KHZ800); //the last two are built in NEO pixel definitions


void setup() 
{
    //init the neo pixels
    strip.begin();
    strip.show(); //turns off all pixels if nothing is loaded in
    strip.setBrightness(led_brightness);

}

void loop() 
{
    uint32_t pixelHue = 0;
    while(true)
    {
        for(int i = 0; i < NEO_LED_COUNT; i++)
        {
            strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue + (1092*i))));
        }
        pixelHue += 40;
        strip.show();

        //delay(50);
        yield();
    }

}
