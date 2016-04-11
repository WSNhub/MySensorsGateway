#ifndef INCLUDE_SCRIPTCORE_H_
#define INCLUDE_SCRIPTCORE_H_

#include "TinyJS/TinyJS.h"
#include "TinyJS/TinyJS_MathFunctions.h"
#include <Mutex.h>
#include "IOExpansion.h"
#include "MyGateway.h"
#include <SmingCore/Debug.h>

class ScriptCore : public CTinyJS
{
public:
    ScriptCore();

private:
    static void staticDebugHandler(CScriptVar *v, void *userdata);
    static void staticSetValueHandler(CScriptVar *v, void *userdata);
    static void staticGetValueHandler(CScriptVar *v, void *userdata);
    static void staticGetIntValueHandler(CScriptVar *v, void *userdata);
    static void staticToggleValueHandler(CScriptVar *v, void *userdata);

public:
    // Locking //
    void lock() { mutex.Lock(); };
    void unlock() { mutex.Unlock(); };

private:
    Mutex mutex;
};

extern ScriptCore ScriptingCore;

#endif /* INCLUDE_SCRIPTCORE_H_ */
