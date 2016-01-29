#ifndef INCLUDE_CLOUD_CONTROLLER_H_
#define INCLUDE_CLOUD_CONTROLLER_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <controller.h>

typedef enum state
{
    CloudControllerStateInit = 0,
    CloudControllerStateWaitingWifi,
    CloudControllerStateWifiConnected,
    CloudControllerStateActivated,
    CloudControllerStateMqttConnecting,
    CloudControllerStateMqttConnected,
    CloudControllerStateMqttSubscribed,
} CloudControllerState;

class CloudController : public Controller
{
  public:
    void begin();
    void notifyChange(String object, String value);
    void registerHttpHandlers(HttpServer &server);
    void registerCommandHandlers();

  private:
    void checkConnection();
    void mqttPublishIdAndVersion();
    void startMqttClient();
    void checkMqttClient();
    void stopMqttClient();
    void onMessageReceived(String topic, String message);
    void oneSecondTimerHandler();
    void activate();
    void onActivateDataSent(HttpClient& client, bool successful);
  private:
    HttpClient            activation;
    CloudControllerState  state;
    int                   incarnation = 0;
    int                   numChecks = 0;
    Timer                 checkTimer;
    MqttClient           *mqtt = NULL;
    char                 *mqtt_thing_topic = NULL;
    char                  clientId[33];
};

#endif //INCLUDE_CLOUD_CONTROLLER_H_
