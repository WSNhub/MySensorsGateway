#include <DeviceHandler.h>

DeviceHandler deviceHandler;

void ICACHE_FLASH_ATTR DeviceHandler::registerHttpHandlers(HttpServer &server) {
  server.addPath("/gpio", DeviceHandler::onGpio);
}

// TODO: QUESTION: Are Debug statements allowed here?
DeviceHandler::DeviceHandler(void) {
  //Debug.printf("DeviceHandler::DeviceHandler(void)\r\n");
  for (int pinNbr = 0; pinNbr < 17; ++pinNbr) {
    if (hardwarePinDirs[pinNbr].equals("0")) {
      //Debug.printf("DeviceHandler::DeviceHandler(void): %d: INPUT\r\n", pinNbr);
      pinMode(pinNbr, INPUT);
    }
    else if (hardwarePinDirs[pinNbr].equals("1")) {
      //Debug.printf("DeviceHandler::DeviceHandler(void): %d: OUTPUT\r\n", pinNbr);
      pinMode(pinNbr, OUTPUT);
    }
  }
}

void DeviceHandler::onGpio(HttpRequest &request, HttpResponse &response) {
  deviceHandler.onGpioMbr(request, response);
}

void DeviceHandler::onGpioMbr(HttpRequest &request, HttpResponse &response) {
    Debug.printf("DeviceHandler::onGpio()\r\n");

    if (!HTTP.isHttpClientAllowed(request, response))
        return;

    Debug.printf("DeviceHandler::onGpio(): isAjax: [%s]\r\n", (request.isAjax()?"true":"false"));

    String command = null;

    TemplateFileStream *tmpl = new TemplateFileStream("gpio.html");
    auto &vars = tmpl->variables();

    if (request.getRequestMethod() == RequestMethod::POST)
    {
        String pinVal = null;

        // Invert (~) the pin value to set it.
        // JOVA: TODO: Call setPortPin(pinNbr, setPortPin);
        // Simulate call here for the moment.
        for (int pinNbr = 0; pinNbr < 17; ++pinNbr) {
          pinVal = request.getPostParameter(String("N:GPIO:") + pinNbr);
          if (!pinVal.equals("")) {
            Debug.printf("onGpio: N:GPIO:%d: [%s]\r\n", pinNbr, pinVal.c_str()); // Current portPin value.
            if (pinVal.equals("0")) {
              pinVal = "1";
            }
            else {
              pinVal = "0";
            }
            //hardwarePinVals[pinNbr] = pinVal; // Set the port pin.
            digitalWrite(pinNbr, pinVal.toInt()); // Set the port pin.
          }
          else {
            Debug.printf("READ: pinVal.equals()\r\n");
            //pinVal = hardwarePinVals[pinNbr]; // Read the port pin.
            pinVal = digitalRead(pinNbr); // Read the port pin.
          }
          vars[String("V:GPIO:") + pinNbr] = pinVal;
          vars[String("D:GPIO:") + pinNbr] = hardwarePinDirs[pinNbr];
        }
    }
    else {
        String command = request.getQueryParameter("command");
        Debug.printf("DeviceHandler::onGpio(): command: [%s]\r\n", command.c_str());

        for (int pinNbr = 0; pinNbr < 17; ++pinNbr) {
          //vars[String("V:GPIO:") + pinNbr] = hardwarePinVals[pinNbr]; // Read the port pin.
          vars[String("V:GPIO:") + pinNbr] = digitalRead(pinNbr); // Read the port pin.
          vars[String("D:GPIO:") + pinNbr] = hardwarePinDirs[pinNbr];
        }
    }

    response.sendTemplate(tmpl); // will be automatically deleted
}

// GPIO 0..16 values.
String DeviceHandler::hardwarePinVals[] = { "0", "0", "1", "0", "0", "0", "0", "1", "1", "1", "1", "0", "1", "0", "1", "0", "0" };
// GPIO 0..16 direction (0 = input; 1 = output; 2 = Do not touch)
// GPIO  0  (D3)       (Flash)
// GPIO  1 (D10)       (UART TX during flash programming)
// GPIO  2  (D4) (NRF) (LED on ESP-12E module)
// GPIO  3  (D9)       (UART RX during flash programming)
// GPIO  4  (D2)       (SDA of MCC; ./Makefile:  USER_CFLAGS += "-DI2C_SDA_PIN=4")
// GPIO  5  (D1)       (SCL of MCC; ./Makefile:  USER_CFLAGS += "-DI2C_SCL_PIN=5")
// GPIO  6, 7, 8, 11: Flash chip SPI
// GPIO  9, 10: Flash chip SPI if QIO, else D11 (SD2), D12, (SD3)
// GPIO 12  (D6) (NRF)
// GPIO 13  (D7) (NRF)
// GPIO 14  (D5) (NRF)
// GPIO 15  (D8) (NRF)
// GPIO 16  (D0)       (LED on NodeMCU DevKit, Deep sleep wake-up)
// ================================== GPIO#  0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16
String DeviceHandler::hardwarePinDirs[] = { "2", "0", "1", "1", "1", "0", "2", "2", "2", "2", "2", "2", "1", "0", "1", "0", "1" };

