#ifndef INCLUDE_RULE_H_
#define INCLUDE_RULE_H_

#include <SmingCore.h>
#include <SmingCore/Debug.h>

class Rule
{
  public:
    Rule();

  public:
    String            name;
    Vector<String>    triggerObjects;
    String            script;
};

class RuleController
{
  public:
    RuleController();
    void begin();
    void store();
    void addRule(String name, String script);
    void addTrigger(String rule, String trigger);
    void processTrigger(String trigger);

  private:
    HashMap<String, Rule*>         rules;
    HashMap<String, Vector<Rule*>> triggers;    
};

extern RuleController Rules;

#endif /* INCLUDE_RULE_H_ */
