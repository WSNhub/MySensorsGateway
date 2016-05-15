#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <globals.h>

#include <AppSettings.h>
#include "MyDisplay.h"
#include "mqtt.h"
#include "Network.h"
#include "AppSettings.h"

MyDisplay Display;

#if DISPLAY_TYPE == DISPLAY_TYPE_SSD1306
#include <Libraries/Adafruit_SSD1306/Adafruit_SSD1306.h>
Adafruit_SSD1306 display(-1); // reset Pin required but later ignored if set to False
#elif DISPLAY_TYPE == DISPLAY_TYPE_20X4
#include <Libraries/LiquidCrystal/LiquidCrystal_I2C.h>
#define I2C_LCD_ADDR 0x27
LiquidCrystal_I2C lcd(I2C_LCD_ADDR, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
#endif

void MyDisplay::update()
{
    Wire.lock();

#if DISPLAY_TYPE == DISPLAY_TYPE_SSD1306
    display.clearDisplay();
    // text display tests
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println("MySensors gateway");
    display.setTextSize(1);
    display.setCursor(0,9);
    if (WifiStation.isConnected())
    {
        display.print("AP  :");
        display.println(Network.getClientIP().toString());
    } 
    else
    {
        display.setTextColor(BLACK, WHITE); // 'inverted' text
        display.println("connecting ...");
        display.setTextColor(WHITE);
    }
    display.setCursor(0,18);
    if (isMqttConfigured())
    {
        display.print("MQTT:");
        display.println(MqttServer());
    }
    else
    {
        display.setTextColor(BLACK, WHITE); // 'inverted' text
        display.println("configure MQTT !");
        display.setTextColor(WHITE);
    }

    display.setCursor(0,27);
    display.println(SystemClock.getSystemTimeString().c_str());
    display.setCursor(0,36);
    display.print("HEAP :");
    display.setTextColor(BLACK, WHITE); // 'inverted' text
    display.println(system_get_free_heap_size());

    display.setTextColor(WHITE);

    //display.setTextColor(BLACK, WHITE); // 'inverted' text
    //display.setTextSize(3);
    display.display();
#elif DISPLAY_TYPE == DISPLAY_TYPE_20X4
    lcd.setCursor(0, 0);
    lcd.print((char *)"MySensors gateway   ");
    lcd.setCursor(0, 1);
    lcd.print((char *)build_git_sha);
    for (int i=0; i<(20-strlen((char *)build_git_sha)); i++)
        lcd.print(" ");
    lcd.setCursor(0, 2);
    lcd.print("HEAP :");
    String heap(system_get_free_heap_size());
    lcd.print(heap.c_str());
    for (int i=0; i<20-6-heap.length(); i++)
        lcd.print(" ");
    lcd.setCursor(0, 3);
    lcd.print("IP   :");
    String ip = Network.getClientIP().toString();
    lcd.print(ip.c_str());
    for (int i=0; i<20-6-ip.length(); i++)
        lcd.print(" ");
#endif

    Wire.unlock();
}

void MyDisplay::begin()
{
    byte error;

    Wire.lock();

#if DISPLAY_TYPE == DISPLAY_TYPE_SSD1306
    Wire.beginTransmission(0x3c);
    error = Wire.endTransmission();

    if (error == 0)
    {
        displayFound = TRUE;
        Debug.printf("Found OLED at %x\n", 0x3c);
        // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)`
        // initialize with the I2C addr 0x3D (for the 128x64)
        // bool:reset set to TRUE or FALSE depending on you display
        display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, FALSE);
        // display.begin(SSD1306_SWITCHCAPVCC);
        display.display();
    }
#elif DISPLAY_TYPE == DISPLAY_TYPE_20X4
    Wire.beginTransmission(I2C_LCD_ADDR);
    error = Wire.endTransmission();

    if (error == 0)
    {
        displayFound = TRUE;
        Debug.printf("Found LCD at %x\n", I2C_LCD_ADDR);
        lcd.begin(20, 4);
        lcd.setCursor(0, 0);
        lcd.print((char *)"MySensors gateway   ");
    }
    else
    {
        Debug.printf("LCD not found at %x\n", I2C_LCD_ADDR);
    }        
#else
    Debug.println("No display available");
    error = 0xff;
#endif

    Wire.unlock();

    if (displayFound)
    {
        displayTimer.initializeMs(1000, TimerDelegate(&MyDisplay::update, this)).start(true);
    }
}
