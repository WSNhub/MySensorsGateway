#include <SmingCore/SmingCore.h>
#include <ScriptCore.h>

ScriptCore ScriptingCore;

ScriptCore::ScriptCore()
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
    addNative("function ToggleObjectValue(object)",
              &ScriptCore::staticToggleValueHandler,
              NULL);
}

void ScriptCore::staticDebugHandler(CScriptVar *v, void *userdata)
{
    String a1 =  v->getParameter("arg1")->getString();
    Debug.println(a1);
}

void ScriptCore::staticSetValueHandler(CScriptVar *v, void *userdata)
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

void ScriptCore::staticGetValueHandler(CScriptVar *v, void *userdata)
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

void ScriptCore::staticGetIntValueHandler(CScriptVar *v, void *userdata)
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

void ScriptCore::staticToggleValueHandler(CScriptVar *v, void *userdata)
{
    String object =  v->getParameter("object")->getString();

    if (object.startsWith("output"))
    {
        //Toggle the value
        Expansion.toggleResourceValue(object);
    }
    else
    {
        Debug.println("ERROR: " + object +
                      " can not be toggled from script");
    }
}

