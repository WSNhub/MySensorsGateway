#ifndef INCLUDE_MY_MUTEX_H_
#define INCLUDE_MY_MUTEX_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "mutex.h"
#ifdef __cplusplus
}
#endif

class MyMutex
{
  public:
    MyMutex()
    {
        CreateMutux(&mutex);
    };
    void Lock()
    {
        while (!GetMutex(&mutex))
            delay(0);
    };
    void Unlock()
    {
        ReleaseMutex(&mutex);
    };
  private:
    mutex_t mutex;
};

#endif //INCLUDE_MY_MUTEX_H_

