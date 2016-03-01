#include "Rule.h"
#include "ScriptCore.h"

Rule::Rule() : triggerObjects(1,1)
{
    //
}

#define RULES_FILE_NAME ".rules.conf"

RuleController::RuleController()
{
    //
}

void RuleController::begin()
{
    if (fileExist(RULES_FILE_NAME))
    {
        DynamicJsonBuffer jsonBuffer;
        int size = fileGetSize(RULES_FILE_NAME);
        char* jsonString = new char[size + 1];
        fileGetContent(RULES_FILE_NAME, jsonString, size + 1);
        JsonObject& root = jsonBuffer.parseObject(jsonString);
        JsonArray& rulesArr = root["rules"];

        for (int i = 0; i < rulesArr.size(); i++)
        {
            JsonObject& ruleObj = rulesArr[i];
            addRule(ruleObj["name"], ruleObj["script"]);
            JsonArray& triggersArr = ruleObj["triggers"];
            for (int j = 0; j < triggersArr.size(); j++)
            { 
                addTrigger(ruleObj["name"],triggersArr[j]);
            }
        }

        delete[] jsonString;
    }
}

void RuleController::store()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    JsonArray& rulesArr = jsonBuffer.createArray();
    root["rules"] = rulesArr;

    for (int r = 0; r < rules.count(); r++)
    {
        Rule *rule = rules.valueAt(r);
        JsonObject& ruleObj = jsonBuffer.createObject();
        ruleObj["name"] = rule->name;
        ruleObj["script"] = rule->script;
        JsonArray& triggersArr = jsonBuffer.createArray();
Debug.printf("Num triggers %d\n", rule->triggerObjects.count());
        for (int s = 0; s < rule->triggerObjects.count(); s++)
        {
Debug.printf("Found trigger %s\n", rule->triggerObjects[s].c_str());
            triggersArr.add(rule->triggerObjects[s]);
        }
        ruleObj["triggers"] = triggersArr;
        rulesArr.add(ruleObj);
    }
    String out;
    root.printTo(out);
    Debug.println(out);
    fileSetContent(RULES_FILE_NAME, out);
}

void RuleController::addRule(String name, String script)
{
    Debug.printf("RULES: add rule %s with script %s\n",
                 name.c_str(), script.c_str());
    Rule *r = new Rule();
    r->name = name;
    r->script = script;

    rules[name] = r;
}

void RuleController::addTrigger(String rule, String trigger)
{
    Debug.printf("RULES: add trigger %s to rule %s\n",
                 trigger.c_str(), rule.c_str());
    if (!rules.contains(rule))
    {
        Debug.println("Unknown rule");
        return;
    }

    Rule *r = rules[rule];
    if (!r->triggerObjects.contains(trigger))
    {
        Debug.println("--1--");
        r->triggerObjects.add(trigger);
    }
    if (!triggers[trigger].contains(r))
    {
        Debug.println("--2--");
        triggers[trigger].add(r);
    }
}

void RuleController::processTrigger(String trigger)
{
    Debug.printf("Processing trigger %s\n", trigger.c_str());
    
    if (!triggers.contains(trigger))
    {
        Debug.printf("Trigger not found\n");
        return;
    }

    for (int i = 0; i < triggers[trigger].size(); i++)
    {
        Rule *r = triggers[trigger][i];

        Debug.printf("Executing rule %s\n", r->name.c_str());
        ScriptingCore.execute(r->script);
    }
}

RuleController Rules;
