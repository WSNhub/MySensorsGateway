#ifndef INCLUDE_OPENHAB_MQTT_CONTROLLER_H_
#define INCLUDE_OPENHAB_MQTT_CONTROLLER_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <controller.h>
#include <mqtt.h>

class OpenHabMqttController : public Controller
{
  public:
    void begin();
    void notifyChange(String object, String value);
    void registerHttpHandlers(HttpServer &server);
    void registerCommandHandlers();

  private:
    void checkConnection();

  private:
    Timer checkTimer;
};

#endif //INCLUDE_OPENHAB_MQTT_CONTROLLER_H_
