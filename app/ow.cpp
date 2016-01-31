#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <Libraries/OneWire/OneWire.h>
#include <AppSettings.h>
#include <ow.h>

OneWire ds(WORK_PIN);

void OW::onAjaxOwList(HttpRequest &request, HttpResponse &response)
{
    response.setAllowCrossDomainOrigin("*");
    response.setContentType(ContentType::JSON);
    response.sendString("{\"status\": true,\"available\": [");
    
    char rom[17];

    String separator = "";
    for (int i = 0; i < MAX_NUM_SENSORS; i++)
    {
        sprintf(rom, "%02x%02x%02x%02x%02x%02x%02x%02x",
                sensors[i].address[0], sensors[i].address[1],
                sensors[i].address[2], sensors[i].address[3],
                sensors[i].address[4], sensors[i].address[5],
                sensors[i].address[6], sensors[i].address[7]);

        response.sendString(separator +
                            "{\"id\": " + (i+1) +
                            ",\"rom\": \"" + rom +
                            "\"}");
        if (separator.equals(""))
            separator = ",";
    }
    response.sendString("]}");
}

void OW::onAjaxOwRemove(HttpRequest &request, HttpResponse &response)
{
    String error;

    JsonObjectStream* stream = new JsonObjectStream();
    JsonObject& json = stream->getRoot();

    String romToRemove = request.getPostParameter("rom");
    if (romToRemove.length() != 16)
    {
        error = "Invalid rom";
        goto error;
    }

    if (!romToRemove.equals(""))
    {
        int i = 0;
        char rom[17];

        for (i = 0; i < MAX_NUM_SENSORS; i++)
        {
            sprintf(rom, "%02x%02x%02x%02x%02x%02x%02x%02x",
                    sensors[i].address[0], sensors[i].address[1],
                    sensors[i].address[2], sensors[i].address[3],
                    sensors[i].address[4], sensors[i].address[5],
                    sensors[i].address[6], sensors[i].address[7]);
        
            if (strcmp(romToRemove.c_str(), rom) == 0)
            {
                //owAddress address;

                Debug.printf("Removing sensor %d with rom %s.\n", i, rom);
                for (int j = 0; j < 8; j++)
                {
                    sensors[i].address[j] = 0;
                    //address[j] = 0;
                }
                //AppSettings.setOwTemp(i, address);
                //AppSettings.save();
                break;
            }
        }

        if (i >= MAX_NUM_SENSORS)
        {
            error = "Rom not found";
            goto error;
        }
    }

    json["status"] = (bool)true;
    response.setAllowCrossDomainOrigin("*");
    response.sendJsonObject(stream);
    return;

error:
    json["status"] = (bool)false;
    json.set("error", error);
    response.setAllowCrossDomainOrigin("*");
    response.sendJsonObject(stream);
}

void OW::owReadData()
{
    byte data[12];

    for (int i = 0; i < MAX_NUM_SENSORS; i++)
    {
        if (sensors[i].address[0] != 0)
        {
            /* Fetch the sensor data */
            byte present = ds.reset();
            ds.select(sensors[i].address);    
            ds.write(0xBE); // Read Scratchpad

            for (int i = 0; i < 9; i++) // we need 9 bytes
            {
                data[i] = ds.read();
            }
  
            // convert the data to actual temperature
            unsigned int raw = ((data[1] << 8) | data[0]);
            float currentTemp;
        
            if (raw & 0xF800)
            {
                raw = raw & 0x07FF;
                raw -= 1;
                raw = ~raw;
                raw = raw & 0x07FF;
                currentTemp =  raw * -0.0625;  
            }
            else
            {
                currentTemp = raw * 0.0625;
            }

            currentTemp = (round(currentTemp*2)/2); 
            if (sensors[i].temperature != currentTemp)
            {
                sensors[i].temperature = currentTemp;
                if (changeDlg)
                    changeDlg(String("temperature-") + String(i+1),
                              String(sensors[i].temperature));
                Debug.printf("Sensor %d changed to %f\n", i, sensors[i].temperature);
            }

            /* Reschedule a measure */
            ds.reset();
            ds.select(sensors[i].address);
            ds.write(0x44,1); // start conversion, with parasite power on at the end
        }
    }
}

void OW::begin(OwChangeDelegate dlg)
{
    byte lastAddress[8] = {0};
    int  devicesFound   = 0;

    changeDlg = dlg;

    ds.begin(); // It's required for one-wire initialization!
    ds.reset_search();

    WDT.alive();

    /*
    for (int i = 0; i < MAX_NUM_SENSORS; i++)
    {
        owAddress address = AppSettings.getOwTemp(i);

        if (address[0] != 0)
        {
            Debug.printf("Used slot found at index %d.\n", i);
            Debug.printf("ROM = %02x %02x %02x %02x %02x %02x %02x %02x",
                   address[0], address[1], address[2], address[3],
                   address[4], address[5], address[6], address[7]);
            for (int j = 0; j < 8; j++)
            {
                sensors[i].address[j] = address[j];
            }
            devicesFound++;
        }
    }
    */

    while (ds.search(lastAddress))
    {
        Debug.print("Found ROM =");
        for (int i = 0; i < 8; i++)
        {
            Serial.write(' ');
            Debug.print(lastAddress[i], HEX);
        }

        if (OneWire::crc8(lastAddress, 7) != lastAddress[7])
        {
            Debug.println(" CRC is not valid!");
        }
        else if (lastAddress[0] != 0x10 &&
                 lastAddress[0] != 0x28 &&
                 lastAddress[0] != 0x22)
        {
            Debug.println(" Device is not a DS18x20 family device.");
        }
        else
        {
            Debug.println();

            // the first ROM byte indicates which chip
            switch (lastAddress[0])
            {
                case 0x10:
                    Debug.println("  Chip = DS18S20");  // or old DS1820
                    break;
                case 0x28:
                    Debug.println("  Chip = DS18B20");
                    break;
                case 0x22:
                    Debug.println("  Chip = DS1822");
                    break;
                default:
                    Debug.println("Device is not a DS18x20 family device.");
                    break;
	    }

            bool alreadyPresent = false;
            for (int i = 0; i < MAX_NUM_SENSORS; i++)
            {
                if (memcmp(sensors[i].address, lastAddress, 8) == 0)
                {
                    Debug.printf("Sensor is already in the list at index %d.\n", i);
                    alreadyPresent = true;
                    break;
                }
            }

            if (!alreadyPresent)
            {
                if (devicesFound >= MAX_NUM_SENSORS)
                {
                    Debug.println("No more room to store this sensor");
                }
                else
                {
                    for (int i = 0; i < MAX_NUM_SENSORS; i++)
                    {
                        if (sensors[i].address[0] == 0)
                        {
                            //owAddress address;
                            Debug.printf("Empty slot found at index %d.\n", i);
                            memcpy(sensors[i].address, lastAddress, 8);
                            
                            //for (int j = 0; j < 8; j++)
                            //{
                            //    address[j] = lastAddress[j];
                            //}
                            //AppSettings.setOwTemp(i, address);
                            //AppSettings.save();

                            devicesFound++;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (devicesFound)
    {
        for (int i = 0; i < MAX_NUM_SENSORS; i++)
        {
            if (sensors[i].address[0] != 0)
            {
                ds.reset();
                ds.select(sensors[i].address);
                ds.write(0x44,1); // start conversion, with parasite power on at the end
            }
        }

        procTimer.initializeMs(30000, TimerDelegate(&OW::owReadData, this)).start();
    }
    else
    {
        Debug.printf("No supported onewire devices found\n");
    }
}

