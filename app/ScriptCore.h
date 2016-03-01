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
    ScriptCore()
    {
        addNative("function print(arg1)",
                  &ScriptCore::staticDebugHandler, NULL);
        addNative("function GetObjectValue(object)",
                  &ScriptCore::staticGetValueHandler,
                  NULL);
        addNative("function GetObjectIntValue(object)",
                  &ScriptCore::staticGetIntValueHandler,
                  NULL);
        addNative("function SetObjectValue(object, value)",
                  &ScriptCore::staticSetValueHandler,
                  NULL);
    }

private:
    static void staticDebugHandler(CScriptVar *v, void *userdata)
    {
        String a1 =  v->getParameter("arg1")->getString();
        Debug.println(a1);
    }

    static void staticSetValueHandler(CScriptVar *v, void *userdata)
    {
        String object =  v->getParameter("object")->getString();
        String value =  v->getParameter("value")->getString();

        if (object.startsWith("output"))
        {
            //Set the value
            Expansion.updateResource(object, value);
        }
        else if (object.startsWith("sensor"))
        {
            GW.setSensorValue(object, value);
        }
        else
        {
            Debug.println("ERROR: Inputs can not be updated from script");
        }
    }

    static void staticGetValueHandler(CScriptVar *v, void *userdata)
    {
        String object = v->getParameter("object")->getString();
        //get the value
        if (object.startsWith("output") || object.startsWith("input"))
        {
            v->getReturnVar()->setString(Expansion.getResourceValue(object));
        }
        else if (object.startsWith("sensor"))
        {
            v->getReturnVar()->setString(GW.getSensorValue(object));
        }
        else
        {
            v->getReturnVar()->setString("UnknownObjectError");
        }
    }

    static void staticGetIntValueHandler(CScriptVar *v, void *userdata)
    {
        String object = v->getParameter("object")->getString();
        //get the value
        if (object.startsWith("output") || object.startsWith("input"))
        {
            if (Expansion.getResourceValue(object).equals("on"))
                v->getReturnVar()->setInt(1);
            else
                v->getReturnVar()->setInt(0);
        }
        else if (object.startsWith("sensor"))
        {
            v->getReturnVar()->setInt(GW.getSensorValue(object).toInt());
        }
        else
        {
            v->getReturnVar()->setInt(0);
        }
    }


public:
    // Locking //
    void lock() { mutex.Lock(); };
    void unlock() { mutex.Unlock(); };

private:
    Mutex mutex;
};

extern ScriptCore ScriptingCore;

#endif /* INCLUDE_SCRIPTCORE_H_ */
