#include "wio.h"
#include "suli2.h"
#include "Main.h"
#include "grove_oled_12864.h"

int pinsda = 4;
int pinscl = 5;
GroveOLED12864 *oled;

float celsius_degree = 0.0f;

uint32_t last_time;


void setup()
{
    oled = new GroveOLED12864(pinsda, pinscl);
    oled->write_clear();
    oled->write_string(0,0,"Temp: ");
    last_time = millis();
    Serial1.println("setup done");
}

void loop()
{
    uint32_t t = millis();
    if (t - last_time > 2000)
    {
        last_time = t;
        GroveTempHumD1_ins->read_temperature(&celsius_degree);
        oled->write_float(0, 6, celsius_degree, 2);
        Serial1.print("celsius_degree: ");
        Serial1.println(celsius_degree);
    }
}


/* The following is an exmaple for ULB */
/*int var1 = 10;
float f = 123.45;
String s;
uint32_t time;

void setup()
{
    s = "this is a string...";
    wio.registerVar("var1", var1);
    wio.registerVar("var2", f);
    wio.registerVar("var3", s);
    time = millis();
}

void loop()
{
    if (millis() - time > 1000)
    {
        time = millis();
        Serial1.println(var1);
    }
}*/
