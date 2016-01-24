#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <openHabMqttController.h>

void OpenHabMqttController::begin()
{
    checkTimer.initializeMs(
        1000,
        TimerDelegate(&OpenHabMqttController::checkConnection,
	              this)).start(true);
}

void OpenHabMqttController::notifyChange(String object, String value)
{
    mqttPublishMessage(object, value);
}

void OpenHabMqttController::registerHttpHandlers(HttpServer &server)
{
    mqttRegisterHttpHandlers(server);
}

void OpenHabMqttController::registerCommandHandlers()
{
    //
}

void OpenHabMqttController::checkConnection()
{
    if (WifiStation.isConnected())
        checkMqttClient();
}

OpenHabMqttController controller;
