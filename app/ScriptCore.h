/*
 * ScriptCore.h
 *
 *  Created on: 26 џэт. 2016 у.
 *      Author: Anakod
 */

#ifndef INCLUDE_SCRIPTCORE_H_
#define INCLUDE_SCRIPTCORE_H_

#include "TinyJS.h"
//#include "TinyJS_Functions.h"
#include "TinyJS_MathFunctions.h"
#include <Mutex.h>
#include "IOExpansion.h"
#include <SmingCore/Debug.h>

class ScriptCore : public CTinyJS
{
public:
    ScriptCore()
    {
        addNative("function GetObjectValue(object)",
                  &ScriptCore::staticGetValueHandler,
                  NULL);
        addNative("function SetObjectValue(object, value)",
                  &ScriptCore::staticSetValueHandler,
                  NULL);
    }

private:
    static void staticSetValueHandler(CScriptVar *v, void *userdata)
    {
        String object =  v->getParameter("object")->getString();
        String value =  v->getParameter("value")->getString();

        if (object.startsWith("output"))
        {
            //Set the value
            Expansion.updateResource(object, value);
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
        v->getReturnVar()->setString(Expansion.getResourceValue(object));
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
