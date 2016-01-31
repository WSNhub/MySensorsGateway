#ifndef INCLUDE_OW_H_
#define INCLUDE_OW_H_

#include <SmingCore/SmingCore.h>
#include <Libraries/OneWire/OneWire.h>

#define MAX_NUM_SENSORS 8
#define WORK_PIN 0 // GPIO0

typedef struct
{
    byte  address[8];
    float temperature;
} DS18B20;

typedef Delegate<void(String, String)> OwChangeDelegate;

class OW
{
  public:
    void begin(OwChangeDelegate dlg = NULL);
    
  private:
    void owReadData();
    void onAjaxOwList(HttpRequest &request, HttpResponse &response);
    void onAjaxOwRemove(HttpRequest &request, HttpResponse &response);

  private:
    OwChangeDelegate changeDlg;
    DS18B20          sensors[MAX_NUM_SENSORS];
    Timer            procTimer;
};

#endif /* INCLUDE_OW_H_ */
