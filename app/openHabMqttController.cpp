#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <openHabMqttController.h>
#include <AppSettings.h>

#if CONTROLLER_TYPE == CONTROLLER_TYPE_OPENHAB
OpenHabMqttController controller;
#endif

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
    if (AppSettings.wired || WifiStation.isConnected())
        checkMqttClient();
}

