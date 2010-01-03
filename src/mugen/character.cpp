#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <string>
#include <cstring>
#include <vector>
#include <ostream>
#include <sstream>
#include <iostream>

// To aid in filesize
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "util/funcs.h"
#include "util/bitmap.h"
#include "util/file-system.h"

#include "mugen_animation.h"
#include "mugen_item.h"
#include "mugen_item_content.h"
#include "mugen_section.h"
#include "character.h"
#include "mugen_sound.h"
#include "mugen_reader.h"
#include "mugen_sprite.h"
#include "mugen_util.h"
#include "globals.h"
#include "state.h"

#include "input/input-map.h"
#include "input/input-manager.h"

#include "parser/all.h"
#include "ast/all.h"

using namespace std;

namespace Mugen{

namespace PaintownUtil = ::Util;

StateController::StateController(const string & name):
type(Unknown),
name(name){
}

StateController::~StateController(){
    for (map<int, vector<Ast::Value*> >::iterator it = triggers.begin(); it != triggers.end(); it++){
        vector<Ast::Value*> values = (*it).second;
        for (vector<Ast::Value*>::iterator value_it = values.begin(); value_it != values.end(); value_it++){
            Ast::Value * value = *value_it;
            delete value;
        }
    }

    delete value;
}

void StateController::setValue(Ast::Value * value){
    this->value = value;
}

void StateController::addTriggerAll(Ast::Value * trigger){
    triggers[-1].push_back(trigger);
}

void StateController::addTrigger(int number, Ast::Value * trigger){
    triggers[number].push_back(trigger);
}

struct RuntimeValue{
    enum Type{
        Bool,
        String,
        Integer,
        Double,
        ListOfString,
    };

    RuntimeValue():
    type(Bool),
    bool_value(false){
    }

    RuntimeValue(bool b):
    type(Bool),
    bool_value(b){
    }

    RuntimeValue(const string & str):
    type(String),
    string_value(str){
    }

    RuntimeValue(const vector<string> & strings):
    type(ListOfString),
    strings_value(strings){
    }

    inline bool isBool() const {
        return type == Bool;
    }

    inline bool getBoolValue() const {
        return bool_value;
    }

    Type type;
    string string_value;
    bool bool_value;
    vector<string> strings_value;
};

bool StateController::canTrigger(const Ast::Value * expression, const vector<string> & commands) const {
    /* a meta-circular evaluator! */
    class Evaluator: public Ast::Walker {
    public:
        Evaluator(const vector<string> & commands):
        commands(commands){
        }

        RuntimeValue result;
        const vector<string> & commands;

        RuntimeValue same(const RuntimeValue & value1, const RuntimeValue & value2){
            switch (value1.type){
                case RuntimeValue::ListOfString : {
                    switch (value2.type){
                        case RuntimeValue::String : {
                            const vector<string> & strings = value1.strings_value;
                            /*
                            if (strings.size() > 0){
                                for (vector<string>::const_iterator it = strings.begin(); it != strings.end(); it++){
                                    Global::debug(0) << "Command: " << *it << endl;
                                }
                            }
                            */
                            for (vector<string>::const_iterator it = strings.begin(); it != strings.end(); it++){
                                const string & check = *it;
                                if (check == value2.string_value){
                                    return RuntimeValue(true);
                                }
                            }
                        }
                    }
                }
            }
            
            return RuntimeValue();
        }

        RuntimeValue evaluate(const Ast::Value * value){
            Evaluator eval(commands);
            value->walk(eval);
            return eval.result;
        }

        RuntimeValue evalIdentifier(const Ast::Identifier & identifier){
            if (identifier == "command"){
                return RuntimeValue(commands);
            }

            return RuntimeValue();
        }

        virtual void onIdenfitier(const Ast::Identifier & identifier){
            result = evalIdentifier(identifier);
        }

        RuntimeValue evalString(const Ast::String & string_value){
            string out;
            string_value >> out;
            return RuntimeValue(out);
        }
          
        virtual void onString(const Ast::String & string){
            result = evalString(string);
        }

        virtual RuntimeValue evalExpressionInfix(const Ast::ExpressionInfix & expression){
            Global::debug(1) << "Evaluate expression " << expression.toString() << endl;
            using namespace Ast;
            switch (expression.getExpressionType()){
                case ExpressionInfix::Or : {
                    break;
                }
                case ExpressionInfix::XOr : {
                    break;
                }
                case ExpressionInfix::And : {
                    break;
                }
                case ExpressionInfix::BitwiseOr : {
                    break;
                }
                case ExpressionInfix::BitwiseXOr : {
                    break;
                }
                case ExpressionInfix::BitwiseAnd : {
                    break;
                }
                case ExpressionInfix::Assignment : {
                    break;
                }
                case ExpressionInfix::Equals : {
                    return same(evaluate(expression.getLeft()), evaluate(expression.getRight()));
                    break;
                }
                case ExpressionInfix::Unequals : {
                    break;
                }
                case ExpressionInfix::GreaterThanEquals : {
                    break;
                }
                case ExpressionInfix::GreaterThan : {
                    break;
                }
                case ExpressionInfix::LessThanEquals : {
                    break;
                }
                case ExpressionInfix::LessThan : {
                    break;
                }
                case ExpressionInfix::Add : {
                    break;
                }
                case ExpressionInfix::Subtract : {
                    break;
                }
                case ExpressionInfix::Multiply : {
                    break;
                }
                case ExpressionInfix::Divide : {
                    break;
                }
                case ExpressionInfix::Modulo : {
                    break;
                }
                case ExpressionInfix::Power : {
                    break;
                }
            }

            return RuntimeValue();
        }

        virtual void onExpressionInfix(const Ast::ExpressionInfix & expression){
            result = evalExpressionInfix(expression);
        }
    };
    Evaluator walker(commands);
    expression->walk(walker);

    return walker.result.isBool() && walker.result.getBoolValue() == true;
}

bool StateController::canTrigger(const vector<Ast::Value*> & expressions, const vector<string> & commands) const {
    for (vector<Ast::Value*>::const_iterator it = expressions.begin(); it != expressions.end(); it++){
        const Ast::Value * value = *it;
        if (!canTrigger(value, commands)){
            return false;
        }
    }
    return true;
}

vector<int> StateController::sortTriggers() const {
    vector<int> out;

    for (map<int, vector<Ast::Value*> >::const_iterator it = triggers.begin(); it != triggers.end(); it++){
        int number = it->first;
        /* ignore triggerall (-1) */
        if (number != -1){
            out.push_back(number);
        }
    }

    sort(out.begin(), out.end());

    return out;
}

bool StateController::canTrigger(const vector<string> & commands) const {
    if (triggers.find(-1) != triggers.end()){
        vector<Ast::Value*> values = triggers.find(-1)->second;
        /* if the triggerall fails then no triggers will work */
        if (!canTrigger(values, commands)){
            return false;
        }
    }

    vector<int> keys = sortTriggers();
    for (vector<int>::iterator it = keys.begin(); it != keys.end(); it++){
        vector<Ast::Value*> values = triggers.find(*it)->second;
        /* if a trigger succeeds then stop processing and just return true */
        if (canTrigger(values, commands)){
            return true;
        }
    }

    return false;
}

void StateController::activate(Character & guy) const {
    Global::debug(1) << "Activate controller " << name << endl;
    switch (getType()){
        case AfterImage : {
            break;
        }
        case AfterImageTime : {
            break;
        }
        case AllPalFX : {
            break;
        }
        case AngleAdd : {
            break;
        }
        case AngleDraw : {
            break;
        }
        case AngleMul : {
            break;
        }
        case AngleSet : {
            break;
        }
        case AppendToClipboard : {
            break;
        }
        case AssertSpecial : {
            break;
        }
        case AttackDist : {
            break;
        }
        case AttackMulSet : {
            break;
        }
        case BGPalFX : {
            break;
        }
        case BindToParent : {
            break;
        }
        case BindToRoot : {
            break;
        }
        case BindToTarget : {
            break;
        }
        case ChangeAnim : {
            break;
        }
        case ChangeAnim2 : {
            break;
        }
        case ChangeState : {
            int state;
            *value >> state;
            guy.changeState(state);
            break;
        }
        case ClearClipboard : {
            break;
        }
        case CtrlSet : {
            break;
        }
        case DefenceMulSet : {
            break;
        }
        case DestroySelf : {
            break;
        }
        case DisplayToClipboard : {
            break;
        }
        case EnvColor : {
            break;
        }
        case EnvShake : {
            break;
        }
        case Explod : {
            break;
        }
        case ExplodBindTime : {
            break;
        }
        case ForceFeedback : {
            break;
        }
        case FallEnvShake : {
            break;
        }
        case GameMakeAnim : {
            break;
        }
        case Gravity : {
            break;
        }
        case Helper : {
            break;
        }
        case HitAdd : {
            break;
        }
        case HitBy : {
            break;
        }
        case HitDef : {
            break;
        }
        case HitFallDamage : {
            break;
        }
        case HitFallSet : {
            break;
        }
        case HitFallVel : {
            break;
        }
        case HitOverride : {
            break;
        }
        case HitVelSet : {
            break;
        }
        case LifeAdd : {
            break;
        }
        case LifeSet : {
            break;
        }
        case MakeDust : {
            break;
        }
        case ModifyExplod : {
            break;
        }
        case MoveHitReset : {
            break;
        }
        case NotHitBy : {
            break;
        }
        case Null : {
            break;
        }
        case Offset : {
            break;
        }
        case PalFX : {
            break;
        }
        case ParentVarAdd : {
            break;
        }
        case ParentVarSet : {
            break;
        }
        case Pause : {
            break;
        }
        case PlayerPush : {
            break;
        }
        case PlaySnd : {
            break;
        }
        case PosAdd : {
            break;
        }
        case PosFreeze : {
            break;
        }
        case PosSet : {
            break;
        }
        case PowerAdd : {
            break;
        }
        case PowerSet : {
            break;
        }
        case Projectile : {
            break;
        }
        case RemoveExplod : {
            break;
        }
        case ReversalDef : {
            break;
        }
        case ScreenBound : {
            break;
        }
        case SelfState : {
            break;
        }
        case SprPriority : {
            break;
        }
        case StateTypeSet : {
            break;
        }
        case SndPan : {
            break;
        }
        case StopSnd : {
            break;
        }
        case SuperPause : {
            break;
        }
        case TargetBind : {
            break;
        }
        case TargetDrop : {
            break;
        }
        case TargetFacing : {
            break;
        }
        case TargetLifeAdd : {
            break;
        }
        case TargetPowerAdd : {
            break;
        }
        case TargetState : {
            break;
        }
        case TargetVelAdd : {
            break;
        }
        case TargetVelSet : {
            break;
        }
        case Trans : {
            break;
        }
        case Turn : {
            break;
        }
        case VarAdd : {
            break;
        }
        case VarRandom : {
            break;
        }
        case VarRangeSet : {
            break;
        }
        case VarSet : {
            break;
        }
        case VelAdd : {
            break;
        }
        case VelMul : {
            break;
        }
        case VelSet : {
            break;
        }
        case Width : {
            break;
        }
        case Unknown : {
            break;
        }
    }
}

State::State():
animation(-1){
}

void State::addController(StateController * controller){
    controllers.push_back(controller);
}

void State::transitionTo(Character & who){
    if (animation != -1){
        who.setAnimation(animation);
    }
}

State::~State(){
    for (vector<StateController*>::iterator it = controllers.begin(); it != controllers.end(); it++){
        delete (*it);
    }
}
    
Command::Command(std::string name, Ast::KeyList * keys, int maxTime, int bufferTime):
name(name),
keys(keys),
maxTime(maxTime),
bufferTime(bufferTime),
ticks(0),
holdKey(-1),
current(keys->getKeys().begin()),
holder(0),
successTime(0),
needRelease(0){
}

bool Command::handle(InputMap<Keys>::Output keys){
    class KeyWalker: public Ast::Walker{
    public:
        KeyWalker(InputMap<Keys>::Output & keys, const InputMap<Keys>::Output & oldKeys, int & holdKey, const Ast::Key *& holder, const Ast::Key *& needRelease):
        ok(false),
        fail(false),
        holdKey(holdKey),
        keys(keys),
        oldKeys(keys),
        holder(holder),
        needRelease(needRelease){
        }
        
        bool ok;
        bool fail;
        int & holdKey;
        InputMap<Keys>::Output & keys;
        const InputMap<Keys>::Output & oldKeys;
        const Ast::Key *& holder;
        const Ast::Key *& needRelease;

        virtual void onKeySingle(const Ast::KeySingle & key){
            if (key == "a"){
                ok = keys[A];
            } else if (key == "b"){
                ok = keys[B];
            } else if (key == "c"){
                ok = keys[C];
            } else if (key == "x"){
                ok = keys[X];
            } else if (key == "y"){
                ok = keys[Y];
            } else if (key == "z"){
                ok = keys[Z];
            } else if (key == "B"){
                ok = keys[Back];
            } else if (key == "DB"){
                ok = keys[Back] && keys[Down];
            } else if (key == "D"){
                ok = keys[Down];
            } else if (key == "DF"){
                ok = keys[Forward] && keys[Down];
            } else if (key == "F"){
                ok = keys[Forward];
            } else if (key == "UF"){
                ok = keys[Forward] && keys[Up];
            } else if (key == "U"){
                ok = keys[Up];
            } else if (key == "UB"){
                ok = keys[Back] && keys[Up];
            }
            if (ok){
                needRelease = &key;
            }
        }

        bool sameKeys(const InputMap<Keys>::Output & map1, const InputMap<Keys>::Output & map2 ){
            return map1 == map2;
        }

        virtual void onKeyModifier(const Ast::KeyModifier & key){
            needRelease = NULL;
            switch (key.getModifierType()){
                case Ast::KeyModifier::MustBeHeldDown : {
                    key.getKey()->walk(*this);
                    if (ok){
                        holder = key.getKey();
                    }
                    break;
                }
                case Ast::KeyModifier::Release : {
                    if (key.getExtra() > 0){
                        if (holdKey > 0){
                            int fake = -1;
                            const Ast::Key * fakeKey;
                            KeyWalker walker(keys, oldKeys, fake, holder, fakeKey);
                            key.getKey()->walk(walker);
                            if (walker.ok){
                                holdKey -= 1;
                            } else {
                                fail = true;
                            }
                        } else if (holdKey == 0){
                            int fake = -1;
                            const Ast::Key * fakeKey;
                            KeyWalker walker(keys, oldKeys, fake, holder, fakeKey);
                            key.getKey()->walk(walker);
                            /* if ok is true then the key is still being pressed */
                            if (!walker.ok){
                                ok = true;
                            }
                        } else if (holdKey == -1){
                            holdKey = key.getExtra();
                        }
                    } else {
                        int fake = -1;
                        KeyWalker walker(keys, oldKeys, fake, holder, needRelease);
                        key.getKey()->walk(walker);
                        if (!walker.ok){
                            ok = true;
                        }
                    }
                    break;
                }
                case Ast::KeyModifier::Direction : {
                    key.getKey()->walk(*this);
                    break;
                }
                case Ast::KeyModifier::Only : {
                    key.getKey()->walk(*this);
                    if (!ok){
                        if (!sameKeys(keys, oldKeys)){
                            fail = true;
                        }
                    }
                    break;
                }
            }
        }

        virtual void onKeyCombined(const Ast::KeyCombined & key){
            int fake = -1;
            KeyWalker left(keys, oldKeys, fake, holder, needRelease);
            KeyWalker right(keys, oldKeys, fake, holder, needRelease);
            key.getKey1()->walk(left);
            key.getKey2()->walk(right);
            ok = left.ok && right.ok;
            if (ok){
                needRelease = &key;
            }
        }
    };
    
    if (successTime > 0){
        successTime -= 1;
        Global::debug(0) << "Pressed " << name << endl;
        return true;
    }

    bool use = true;
    if (needRelease != NULL){
        const Ast::Key * fake;
        KeyWalker walker(keys, oldKeys, holdKey, holder, fake);
        needRelease->walk(walker);
        Global::debug(1) << "Waiting for key " << needRelease->toString() << " to be released: " << walker.ok << endl;
        if (walker.ok){
            use = false;
        } else {
            needRelease = NULL;
        }
    }

    bool fail = false;
    bool ok = false;

    if (use){
        KeyWalker walker(keys, oldKeys, holdKey, holder, needRelease);
        (*current)->walk(walker);

        ok = walker.ok;
        fail = walker.fail;
        if (holder != 0){
            holder->walk(walker);
            ok &= walker.ok;
            fail |= walker.fail;
        }
    }

    oldKeys = keys;
    ticks += 1;
    if (ticks > maxTime){
        fail = true;
        Global::debug(2) << name << " ran out of time" << endl;
    }

    if (fail){
        current = this->keys->getKeys().begin();
        ticks = 0;
        needRelease = NULL;
        holdKey = -1;
        holder = 0;
    } else if (ok){
        current++;
        holdKey = -1;
        if (current == this->keys->getKeys().end()){
            /* success! */
            current = this->keys->getKeys().begin();
            ticks = 0;
            needRelease = NULL;
            holder = 0;
            successTime = bufferTime - 1;
            Global::debug(1) << "Pressed " << name << endl;
            return true;
        }
    }

    return false;
}

Command::~Command(){
    delete keys;
}

Character::Character( const string & s ):
ObjectAttack(0){
    this->location = s;
    initialize();
}

Character::Character( const char * location ):
ObjectAttack(0){
    this->location = std::string(location);
    initialize();
}

Character::Character( const string & s, int alliance ):
ObjectAttack(alliance){
    this->location = s;
    initialize();
}

Character::Character( const string & s, const int x, const int y, int alliance ):
ObjectAttack(x,y,alliance){
    this->location = s;
    initialize();
}

Character::Character( const Character & copy ):
ObjectAttack(copy){
}

Character::~Character(){
     // Get rid of sprites
    for (std::map< unsigned int, std::map< unsigned int, MugenSprite * > >::iterator i = sprites.begin() ; i != sprites.end() ; ++i ){
      for( std::map< unsigned int, MugenSprite * >::iterator j = i->second.begin() ; j != i->second.end() ; ++j ){
	  if( j->second )delete j->second;
      }
    }
    
     // Get rid of bitmaps
    for( std::map< unsigned int, std::map< unsigned int, Bitmap * > >::iterator i = bitmaps.begin() ; i != bitmaps.end() ; ++i ){
      for( std::map< unsigned int, Bitmap * >::iterator j = i->second.begin() ; j != i->second.end() ; ++j ){
	  if( j->second )delete j->second;
      }
    }
    
    // Get rid of animation lists;
    for( std::map< int, MugenAnimation * >::iterator i = animations.begin() ; i != animations.end() ; ++i ){
	if( i->second )delete i->second;
    }
    
    // Get rid of sounds
    for( std::map< unsigned int, std::map< unsigned int, MugenSound * > >::iterator i = sounds.begin() ; i != sounds.end() ; ++i ){
      for( std::map< unsigned int, MugenSound * >::iterator j = i->second.begin() ; j != i->second.end() ; ++j ){
	  if( j->second )delete j->second;
      }
    }

    for (vector<Command*>::iterator it = commands.begin(); it != commands.end(); it++){
        delete (*it);
    }
}

void Character::initialize(){
    currentState = Standing;
    currentAnimation = Standing;

    input.set(Keyboard::Key_UP, 0, false, Command::Up);
    input.set(Keyboard::Key_DOWN, 0, false, Command::Down);
    input.set(Keyboard::Key_RIGHT, 0, false, Command::Forward);
    input.set(Keyboard::Key_LEFT, 0, false, Command::Back);

    input.set(Keyboard::Key_A, 0, false, Command::A);
    input.set(Keyboard::Key_S, 0, false, Command::B);
    input.set(Keyboard::Key_D, 0, false, Command::C);
    input.set(Keyboard::Key_Z, 0, false, Command::X);
    input.set(Keyboard::Key_X, 0, false, Command::Y);
    input.set(Keyboard::Key_C, 0, false, Command::Z);
}
    
void Character::addCommand(Command * command){
    commands.push_back(command);
    /* todo */
}

void Character::loadCmdFile(const string & path){
    string full = Filesystem::find("mugen/chars/" + location + "/" + PaintownUtil::trim(path));
    try{
        int defaultTime = 15;
        int defaultBufferTime = 1;

        Ast::AstParse parsed((list<Ast::Section*>*) Mugen::Cmd::main(full));
        for (Ast::AstParse::section_iterator section_it = parsed.getSections()->begin(); section_it != parsed.getSections()->end(); section_it++){
            Ast::Section * section = *section_it;
            std::string head = section->getName();
            /* this should really be head = Mugen::Util::fixCase(head) */
            Util::fixCase(head);

            if (head == "command"){
                class CommandWalker: public Ast::Walker {
                public:
                    CommandWalker(Character & self, const int defaultTime, const int defaultBufferTime):
                        self(self),
                        time(defaultTime),
                        bufferTime(defaultBufferTime),
                        key(0){
                        }

                    Character & self;
                    int time;
                    int bufferTime;
                    string name;
                    Ast::Key * key;

                    virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                        if (simple == "name"){
                            simple >> name;
                        } else if (simple == "command"){
                            key = (Ast::Key*) simple.getValue()->copy();
                        } else if (simple == "time"){
                            simple >> time;
                        } else if (simple == "buffer.time"){
                            simple >> bufferTime;
                            /* Time that the command will be buffered for. If the command is done
                             * successfully, then it will be valid for this time. The simplest
                             * case is to set this to 1. That means that the command is valid
                             * only in the same tick it is performed. With a higher value, such
                             * as 3 or 4, you can get a "looser" feel to the command. The result
                             * is that combos can become easier to do because you can perform
                             * the command early.
                             */
                        }
                    }

                    virtual ~CommandWalker(){
                        if (name == ""){
                            throw MugenException("No name given for command");
                        }

                        if (key == 0){
                            throw MugenException("No key sequence given for command");
                        }

                        /* parser guarantees the key will be a KeyList */
                        self.addCommand(new Command(name, (Ast::KeyList*) key, time, bufferTime));
                    }
                };

                CommandWalker walker(*this, defaultTime, defaultBufferTime);
                section->walk(walker);
            } else if (head == "defaults"){
                class DefaultWalker: public Ast::Walker {
                public:
                    DefaultWalker(int & time, int & buffer):
                        time(time),
                        buffer(buffer){
                        }

                    int & time;
                    int & buffer;

                    virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                        if (simple == "command.time"){
                            simple >> time;
                        } else if (simple == "command.buffer.time"){
                            simple >> buffer;
                        }
                    }
                };

                DefaultWalker walker(defaultTime, defaultBufferTime);
                section->walk(walker);
            }

            /* [Defaults]
             * ; Default value for the "time" parameter of a Command. Minimum 1.
             * command.time = 15
             *
             * ; Default value for the "buffer.time" parameter of a Command. Minimum 1,
             * ; maximum 30.
             * command.buffer.time = 1
             */
        }
    } catch (const Mugen::Cmd::ParseException & e){
        Global::debug(0) << "Could not parse " << path << endl;
        Global::debug(0) << e.getReason() << endl;
    }
}

static bool isStateDefSection(string name){
    Util::fixCase(name);
    return PaintownUtil::matchRegex(name, "state ") ||
           PaintownUtil::matchRegex(name, "statedef ");
}
    
void Character::setConstant(std::string name, const vector<double> & values){
    constants[name] = Constant(values);
}

void Character::setConstant(std::string name, double value){
    constants[name] = Constant(value);
}
        
void Character::changeState(int stateNumber){
    Global::debug(1) << "Change to state " << stateNumber << endl;
    currentState = stateNumber;
    if (states[currentState] != 0){
        State * state = states[currentState];
        state->transitionTo(*this);
    }
}

void Character::loadCnsFile(const string & path){
    string full = Filesystem::find("mugen/chars/" + location + "/" + PaintownUtil::trim(path));
    try{
        /* cns can use the Cmd parser */
        Ast::AstParse parsed((list<Ast::Section*>*) Mugen::Cmd::main(full));
        for (Ast::AstParse::section_iterator section_it = parsed.getSections()->begin(); section_it != parsed.getSections()->end(); section_it++){
            Ast::Section * section = *section_it;
            std::string head = section->getName();
            /* this should really be head = Mugen::Util::fixCase(head) */
            Util::fixCase(head);
            if (!isStateDefSection(head)){
                class AttributeWalker: public Ast::Walker {
                public:
                    AttributeWalker(Character & who):
                    self(who){
                    }

                    Character & self;

                    virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                        string name = simple.idString();
                        if (simple.getValue() != 0 && simple.getValue()->hasMultiple()){
                            vector<double> values;
                            simple >> values;
                            self.setConstant(name, values);
                        } else {
                            double value;
                            simple >> value;
                            self.setConstant(name, value);
                        }
                    }
                };

                AttributeWalker walker(*this);
                section->walk(walker);
            }
        }
    } catch (const Mugen::Cmd::ParseException & e){
        Global::debug(0) << "Could not parse " << path << endl;
        Global::debug(0) << e.getReason() << endl;
    } catch (const Ast::Exception & e){
        Global::debug(0) << "Could not parse " << path << endl;
        Global::debug(0) << e.getReason() << endl;
    }
}

void Character::loadStateFile(const std::string & base, const string & path){
    string full = Filesystem::find(base + "/" + PaintownUtil::trim(path));
    try{
        /* st can use the Cmd parser */
        Ast::AstParse parsed((list<Ast::Section*>*) Mugen::Cmd::main(full));
        for (Ast::AstParse::section_iterator section_it = parsed.getSections()->begin(); section_it != parsed.getSections()->end(); section_it++){
            Ast::Section * section = *section_it;
            std::string head = section->getName();
            /* this should really be head = Mugen::Util::fixCase(head) */
            Util::fixCase(head);
            if (PaintownUtil::matchRegex(head, "statedef")){
                int state = atoi(PaintownUtil::captureRegex(head, "statedef *(-?[0-9]+)", 0).c_str());
                class StateWalker: public Ast::Walker {
                public:
                    StateWalker(State * definition):
                    definition(definition){
                    }

                    State * definition;
                
                    virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                        if (simple == "type"){
                            string type;
                            simple >> type;
                            if (type == "S"){
                                definition->setType(State::Standing);
                            } else if (type == "C"){
                                definition->setType(State::Crouching);
                            } else if (type == "A"){
                                definition->setType(State::Air);
                            } else if (type == "L"){
                                definition->setType(State::LyingDown);
                            } else if (type == "U"){
                                definition->setType(State::Unchanged);
                            } else {
                                ostringstream out;
                                out << "Unknown statedef type: '" << type << "'";
                                throw MugenException(out.str());
                            }
                        } else if (simple == "movetype"){
                        } else if (simple == "physics"){
                        } else if (simple == "anim"){
                            int animation;
                            simple >> animation;
                            definition->setAnimation(animation);
                        } else if (simple == "velset"){
                        } else if (simple == "ctrl"){
                        } else if (simple == "poweradd"){
                        } else if (simple == "juggle"){
                        } else if (simple == "facep2"){
                        } else if (simple == "hitdefpersist"){
                        } else if (simple == "movehitpersist"){
                        } else if (simple == "hitcountpersist"){
                        } else if (simple == "sprpriority"){
                        }
                    }
                };

                State * definition = new State();
                StateWalker walker(definition);
                section->walk(walker);
                if (states[state] != 0){
                    Global::debug(1) << "Overriding state " << state << endl;
                    delete states[state];
                }
                Global::debug(1) << "Adding state definition " << state << endl;
                states[state] = definition;
            } else if (PaintownUtil::matchRegex(head, "state ")){
                int state = atoi(PaintownUtil::captureRegex(head, "state *(-?[0-9]+)", 0).c_str());
                string name = PaintownUtil::captureRegex(head, "state *-?[0-9]+ *, *(.*)", 0);

                class StateControllerWalker: public Ast::Walker {
                public:
                    StateControllerWalker(StateController * controller):
                        controller(controller){
                        }

                    StateController * controller;

                    virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                        if (simple == "type"){
                            string type;
                            simple >> type;
                            Mugen::Util::fixCase(type);
                            map<string, StateController::Type> types;
                            types["afterimage"] = StateController::AfterImage;
                            types["afterimagetime"] = StateController::AfterImageTime;
                            types["allpalfx"] = StateController::AllPalFX;
                            types["angleadd"] = StateController::AngleAdd;
                            types["angledraw"] = StateController::AngleDraw;
                            types["anglemul"] = StateController::AngleMul;
                            types["angleset"] = StateController::AngleSet;
                            types["appendtoclipboard"] = StateController::AppendToClipboard;
                            types["assertspecial"] = StateController::AssertSpecial;
                            types["attackdist"] = StateController::AttackDist;
                            types["attackmulset"] = StateController::AttackMulSet;
                            types["bgpalfx"] = StateController::BGPalFX;
                            types["bindtoparent"] = StateController::BindToParent;
                            types["bindtoroot"] = StateController::BindToRoot;
                            types["bindtotarget"] = StateController::BindToTarget;
                            types["changeanim"] = StateController::ChangeAnim;
                            types["changeanim2"] = StateController::ChangeAnim2;
                            types["changestate"] = StateController::ChangeState;
                            types["clearclipboard"] = StateController::ClearClipboard;
                            types["ctrlset"] = StateController::CtrlSet;
                            types["defencemulset"] = StateController::DefenceMulSet;
                            types["destroyself"] = StateController::DestroySelf;
                            types["displaytoclipboard"] = StateController::DisplayToClipboard;
                            types["envcolor"] = StateController::EnvColor;
                            types["envshake"] = StateController::EnvShake;
                            types["explod"] = StateController::Explod;
                            types["explodbindtime"] = StateController::ExplodBindTime;
                            types["forcefeedback"] = StateController::ForceFeedback;
                            types["fallenvshake"] = StateController::FallEnvShake;
                            types["gamemakeanim"] = StateController::GameMakeAnim;
                            types["gravity"] = StateController::Gravity;
                            types["helper"] = StateController::Helper;
                            types["hitadd"] = StateController::HitAdd;
                            types["hitby"] = StateController::HitBy;
                            types["hitdef"] = StateController::HitDef;
                            types["hitfalldamage"] = StateController::HitFallDamage;
                            types["hitfallset"] = StateController::HitFallSet;
                            types["hitfallvel"] = StateController::HitFallVel;
                            types["hitoverride"] = StateController::HitOverride;
                            types["hitvelset"] = StateController::HitVelSet;
                            types["lifeadd"] = StateController::LifeAdd;
                            types["lifeset"] = StateController::LifeSet;
                            types["makedust"] = StateController::MakeDust;
                            types["modifyexplod"] = StateController::ModifyExplod;
                            types["movehitreset"] = StateController::MoveHitReset;
                            types["nothitby"] = StateController::NotHitBy;
                            types["null"] = StateController::Null;
                            types["offset"] = StateController::Offset;
                            types["palfx"] = StateController::PalFX;
                            types["parentvaradd"] = StateController::ParentVarAdd;
                            types["parentvarset"] = StateController::ParentVarSet;
                            types["pause"] = StateController::Pause;
                            types["playerpush"] = StateController::PlayerPush;
                            types["playsnd"] = StateController::PlaySnd;
                            types["posadd"] = StateController::PosAdd;
                            types["posfreeze"] = StateController::PosFreeze;
                            types["posset"] = StateController::PosSet;
                            types["poweradd"] = StateController::PowerAdd;
                            types["powerset"] = StateController::PowerSet;
                            types["projectile"] = StateController::Projectile;
                            types["removeexplod"] = StateController::RemoveExplod;
                            types["reversaldef"] = StateController::ReversalDef;
                            types["screenbound"] = StateController::ScreenBound;
                            types["selfstate"] = StateController::SelfState;
                            types["sprpriority"] = StateController::SprPriority;
                            types["statetypeset"] = StateController::StateTypeSet;
                            types["sndpan"] = StateController::SndPan;
                            types["stopsnd"] = StateController::StopSnd;
                            types["superpause"] = StateController::SuperPause;
                            types["targetbind"] = StateController::TargetBind;
                            types["targetdrop"] = StateController::TargetDrop;
                            types["targetfacing"] = StateController::TargetFacing;
                            types["targetlifeadd"] = StateController::TargetLifeAdd;
                            types["targetpoweradd"] = StateController::TargetPowerAdd;
                            types["targetstate"] = StateController::TargetState;
                            types["targetveladd"] = StateController::TargetVelAdd;
                            types["targetvelset"] = StateController::TargetVelSet;
                            types["trans"] = StateController::Trans;
                            types["turn"] = StateController::Turn;
                            types["varadd"] = StateController::VarAdd;
                            types["varrandom"] = StateController::VarRandom;
                            types["varrangeset"] = StateController::VarRangeSet;
                            types["varset"] = StateController::VarSet;
                            types["veladd"] = StateController::VelAdd;
                            types["velmul"] = StateController::VelMul;
                            types["velset"] = StateController::VelSet;
                            types["width"] = StateController::Width;

                            if (types.find(type) != types.end()){
                                map<string, StateController::Type>::iterator what = types.find(type);
                                controller->setType((*what).second);
                            }
                        } else if (simple == "value"){
                            controller->setValue((Ast::Value*) simple.getValue()->copy());
                        } else if (simple == "triggerall"){
                            controller->addTriggerAll((Ast::Value*) simple.getValue()->copy());
                        } else if (PaintownUtil::matchRegex(simple.idString(), "trigger[0-9]+")){
                            int trigger = atoi(PaintownUtil::captureRegex(simple.idString(), "trigger([0-9]+)", 0).c_str());
                            controller->addTrigger(trigger, (Ast::Value*) simple.getValue()->copy());
                        }
                    }
                };

                StateController * controller = new StateController(name);
                StateControllerWalker walker(controller);
                section->walk(walker);

                if (states[state] == 0){
                    ostringstream out;
                    out << "No StateDef for state " << state << " [" << name << "]";
                    delete controller;
                    throw MugenException(out.str());
                }

                states[state]->addController(controller);
                
                Global::debug(1) << "Adding state controller '" << name << "' to state " << state << endl;
            }
        }
    } catch (const Mugen::Cmd::ParseException & e){
        Global::debug(0) << "Could not parse " << path << endl;
        Global::debug(0) << e.getReason() << endl;
    }
}

/* a container for a directory and a file */
struct Location{
    Location(string base, string file):
        base(base), file(file){
        }

    string base;
    string file;
};

void Character::load(){
    // Lets look for our def since some people think that all file systems are case insensitive
    baseDir = Filesystem::find("mugen/chars/" + location + "/");
    Global::debug(1) << baseDir << endl;
    std::string realstr = Mugen::Util::stripDir(location);
    const std::string ourDefFile = Mugen::Util::fixFileName(baseDir, std::string(realstr + ".def"));
    
    if (ourDefFile.empty()){
        throw MugenException( "Cannot locate player definition file for: " + location );
    }
     
    Ast::AstParse parsed((list<Ast::Section*>*) Mugen::Def::main(ourDefFile));
    /* Extract info for our first section of our stage */
    for (Ast::AstParse::section_iterator section_it = parsed.getSections()->begin(); section_it != parsed.getSections()->end(); section_it++){
        Ast::Section * section = *section_it;
	std::string head = section->getName();
        /* this should really be head = Mugen::Util::fixCase(head) */
	Mugen::Util::fixCase(head);

        if (head == "info"){
            class InfoWalker: public Ast::Walker {
            public:
                InfoWalker(Character & who):
                self(who){
                }

                Character & self;

                virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                    if (simple == "name"){
                        simple >> self.name;
                    } else if (simple == "displayname"){
                        simple >> self.displayName;
                    } else if (simple == "versiondate"){
                        simple >> self.versionDate;
                    } else if (simple == "mugenversion"){
                        simple >> self.mugenVersion;
                    } else if (simple == "author"){
                        simple >> self.author;
                    } else if (simple == "pal.defaults"){
                        vector<int> numbers;
                        simple >> numbers;
                        for (vector<int>::iterator it = numbers.begin(); it != numbers.end(); it++){
                            self.palDefaults.push_back((*it) - 1);
                        }
                        // Global::debug(1) << "Pal" << self.palDefaults.size() << ": " << num << endl;
                    } else throw MugenException("Unhandled option in Info Section: " + simple.toString());
                }
            };

            InfoWalker walker(*this);
            Ast::Section * section = *section_it;
            section->walk(walker);
        } else if (head == "files"){
            class FilesWalker: public Ast::Walker {
            public:
                FilesWalker(Character & self, const string & location):
                location(location),
                self(self){
                }

                vector<Location> stateFiles;
                const string & location;

                Character & self;
                virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                    if (simple == "cmd"){
                        simple >> self.cmdFile;
                        self.loadCmdFile(self.cmdFile);
                    } else if (simple == "cns"){
                        simple >> self.constantsFile;
                        self.loadCnsFile(self.constantsFile);
                    } else if (PaintownUtil::matchRegex(simple.idString(), "st[0-9]+")){
                        int num = atoi(PaintownUtil::captureRegex(simple.idString(), "st([0-9]+)", 0).c_str());
                        if (num >= 0 && num <= 12){
                            string path;
                            simple >> path;
                            stateFiles.push_back(Location("mugen/chars/" + location, path));
                            // simple >> self.stFile[num];
                        }
                    } else if (simple == "stcommon"){
                        string path;
                        simple >> path;
                        stateFiles.insert(stateFiles.begin(), Location("mugen/data/", path));
                    } else if (simple == "st"){
                        string path;
                        simple >> path;
                        stateFiles.push_back(Location("mugen/chars/" + location, path));
                    } else if (simple == "sprite"){
                        simple >> self.sffFile;
                    } else if (simple == "anim"){
                        simple >> self.airFile;
                    } else if (simple == "sound"){
                        simple >> self.sndFile;
                        Mugen::Util::readSounds( Mugen::Util::fixFileName(self.baseDir, self.sndFile ), self.sounds );
                    } else if (PaintownUtil::matchRegex(simple.idString(), "pal[0-9]+")){
                        int num = atoi(PaintownUtil::captureRegex(simple.idString(), "pal([0-9]+)", 0).c_str());
                        string what;
                        simple >> what;
                        self.palFile[num] = what;
                    } else {
                        throw MugenException("Unhandled option in Files Section: " + simple.toString());
                    }
                }
            };

            FilesWalker walker(*this, location);
            Ast::Section * section = *section_it;
            section->walk(walker);

            for (vector<Location>::iterator it = walker.stateFiles.begin(); it != walker.stateFiles.end(); it++){
                Location & where = *it;
                try{
                    loadStateFile(where.base, where.file);
                } catch (const MugenException & e){
                    ostringstream out;
                    out << "Problem loading state file " << where.file << ": " << e.getReason();
                    throw MugenException(out.str());
                }
            }

            /*
            if (commonStateFile != ""){
                loadStateFile("mugen/data/", commonStateFile);
            }
            if (stateFile != ""){
                loadStateFile("mugen/chars/" + location, stateFile);
            }
            if (
            */

	} else if (head == "arcade"){
            class ArcadeWalker: public Ast::Walker {
            public:
                ArcadeWalker(Character & self):
                self(self){
                }

                Character & self;

                virtual void onAttributeSimple(const Ast::AttributeSimple & simple){
                    if (simple == "intro.storyboard"){
                        simple >> self.introFile;
                    } else if (simple == "ending.storyboard"){
                        simple >> self.endingFile;
                    } else {
                        throw MugenException("Unhandled option in Arcade Section: " + simple.toString());
                    }
                }
            };
            
            ArcadeWalker walker(*this);
            Ast::Section * section = *section_it;
            section->walk(walker);
	}
    }

    // Current palette
    if (palDefaults.empty()){
	// Correct the palette defaults
	for (unsigned int i = 0; i < palFile.size(); ++i){
	    palDefaults.push_back(i);
	}
    }
    if (palDefaults.size() < palFile.size()){
	bool setPals[palFile.size()];
	for( unsigned int i =0;i<palFile.size();++i){
	    setPals[i] = false;
	}
	// Set the ones already set
	for (unsigned int i = 0; i < palDefaults.size(); ++i){
	    setPals[palDefaults[i]] = true;
	}
	// now add the others
	for( unsigned int i =0;i<palFile.size();++i){
	    if(!setPals[i]){
		palDefaults.push_back(i);
	    }
	}
    }

    currentPalette = 0;
    Global::debug(1) << "Current pal: " << currentPalette << " | Palette File: " << palFile[palDefaults[currentPalette]] << endl;
    Global::debug(1) << "Reading Sff (sprite) Data..." << endl; 
    /* Sprites */
    Mugen::Util::readSprites( Mugen::Util::fixFileName(baseDir, sffFile), Mugen::Util::fixFileName(baseDir, palFile[palDefaults[currentPalette]]), sprites );
    Global::debug(1) << "Reading Air (animation) Data..." << endl;
    /* Animations */
    bundleAnimations();
}

// Render sprite
void Character::renderSprite(const int x, const int y, const unsigned int group, const unsigned int image, Bitmap *bmp , const int flip, const double scalex, const double scaley ){
    MugenSprite *sprite = sprites[group][image];
    if (sprite){
	Bitmap *bitmap = sprite->getBitmap();//bitmaps[group][image];
	/*if (!bitmap){
	    bitmap = new Bitmap(Bitmap::memoryPCX((unsigned char*) sprite->pcx, sprite->newlength));
	    bitmaps[group][image] = bitmap;
	}*/
	const int width = (int)(bitmap->getWidth() * scalex);
	const int height =(int)(bitmap->getHeight() * scaley);
	if (flip == 1){
	    bitmap->drawStretched(x,y, width, height, *bmp);
	} else if (flip == -1){
	    // temp bitmap to flip and crap
	    Bitmap temp = Bitmap::temporaryBitmap(bitmap->getWidth(), bitmap->getHeight());
	    temp.fill(Bitmap::MaskColor);
	    bitmap->drawHFlip(0,0,temp);
	    temp.drawStretched(x-width,y, width, height, *bmp);
	}
    }
}

void Character::nextPalette(){
    if (currentPalette < palDefaults.size()-1){
	currentPalette++;
    } else currentPalette = 0;
    Global::debug(1) << "Current pal: " << currentPalette << " | Location: " << palDefaults[currentPalette] << " | Palette File: " << palFile[palDefaults[currentPalette]] << endl;
   /*
    // Now replace the palettes
    unsigned char pal[768];
    if (Mugen::Util::readPalette(Mugen::Util::fixFileName(baseDir, palFile[palDefaults[currentPalette]]),pal)){
	for( std::map< unsigned int, std::map< unsigned int, MugenSprite * > >::iterator i = sprites.begin() ; i != sprites.end() ; ++i ){
	    for( std::map< unsigned int, MugenSprite * >::iterator j = i->second.begin() ; j != i->second.end() ; ++j ){
		if( j->second ){
		    MugenSprite *sprite = j->second;
		    if ( sprite->samePalette){
			memcpy( sprite->pcx + (sprite->reallength), pal, 768);
		    } else {
			if (!(sprite->groupNumber == 9000 && sprite->imageNumber == 1)){
			    memcpy( sprite->pcx + (sprite->reallength)-768, pal, 768);
			} 
		    }
		}
	    }
	}
	// reload with new palette
	for( std::map< int, MugenAnimation * >::iterator i = animations.begin() ; i != animations.end() ; ++i ){
	    if( i->second )i->second->reloadBitmaps();
	}
    }
    */
}

void Character::priorPalette(){
    if (currentPalette > 0){
	currentPalette--;
    } else currentPalette = palDefaults.size() -1;
    Global::debug(1) << "Current pal: " << currentPalette << " | Palette File: " << palFile[palDefaults[currentPalette]] << endl;
    // Now replace the palettes
    /*unsigned char pal[768];
    if (Mugen::Util::readPalette(Mugen::Util::fixFileName(baseDir, palFile[palDefaults[currentPalette]]),pal)){
	for( std::map< unsigned int, std::map< unsigned int, MugenSprite * > >::iterator i = sprites.begin() ; i != sprites.end() ; ++i ){
	    for( std::map< unsigned int, MugenSprite * >::iterator j = i->second.begin() ; j != i->second.end() ; ++j ){
		if( j->second ){
		    MugenSprite *sprite = j->second;
		    if ( sprite->samePalette){
			memcpy( sprite->pcx + (sprite->reallength), pal, 768);
		    } else {
			if (!(sprite->groupNumber == 9000 && sprite->imageNumber == 1)){
			    memcpy( sprite->pcx + (sprite->reallength)-768, pal, 768);
			} 
		    }
		}
	    }
	}
	// Get rid of animation lists;
	for( std::map< int, MugenAnimation * >::iterator i = animations.begin() ; i != animations.end() ; ++i ){
	    if( i->second )i->second->reloadBitmaps();
	}
    }*/
}

/* parse animations.
 * badly named and doesn't return anything.. maybe return an std::map ?
 */
void Character::bundleAnimations(){
    Ast::AstParse parsed((list<Ast::Section*>*) Mugen::Air::main(Mugen::Util::fixFileName(baseDir, airFile)));
    Global::debug(1, __FILE__) << "Parsing animations. Number of sections is " << parsed.getSections()->size() << endl;
    
    for (Ast::AstParse::section_iterator section_it = parsed.getSections()->begin(); section_it != parsed.getSections()->end(); section_it++){
        Ast::Section * section = *section_it;
        std::string head = section->getName();
        Global::debug(1, __FILE__) << "Animation section '" << head << "'" << endl;
	Mugen::Util::fixCase(head);
        int number;
        if (PaintownUtil::matchRegex(head, "begin action [0-9]+")){
            number = atoi(PaintownUtil::captureRegex(head, "begin action ([0-9]+)", 0).c_str());
            Global::debug(1, __FILE__) << "Parse animation " << number << endl;
            animations[number] = Mugen::Util::getAnimation(section, sprites);
        }
    }
}

MugenAnimation * Character::getCurrentAnimation() const {
    typedef std::map< int, MugenAnimation * > Animations;
    Animations::const_iterator it = getAnimations().find(currentAnimation);
    if (it != getAnimations().end()){
        MugenAnimation * animation = (*it).second;
        return animation;
    }
    return NULL;
}

/* returns all the commands that are currently active */
vector<string> Character::doInput(InputMap<Command::Keys>::Output output){
    vector<string> out;

    for (vector<Command*>::iterator it = commands.begin(); it != commands.end(); it++){
        Command * command = *it;
        if (command->handle(output)){
            out.push_back(command->getName());
        }
    }

    return out;
}

/* Inherited members */
void Character::act(std::vector<Object*, std::allocator<Object*> >*, World*, std::vector<Object*, std::allocator<Object*> >*){
    MugenAnimation * animation = getCurrentAnimation();
    if (animation != 0){
        animation->logic();
    }

    vector<string> active = doInput(InputManager::getMap(input));
    doStates(active, -3);
    doStates(active, -2);
    doStates(active, -1);
    doStates(active, currentState);
}

void Character::doStates(const vector<string> & active, int stateNumber){
    if (states[stateNumber] != 0){
        State * state = states[stateNumber];
        for (vector<StateController*>::const_iterator it = state->getControllers().begin(); it != state->getControllers().end(); it++){
            const StateController * controller = *it;
            if (controller->canTrigger(active)){
                controller->activate(*this);
            }
        }
    }
}

void Character::draw(Bitmap * work, int x_position){
    MugenAnimation * animation = getCurrentAnimation();
    if (animation != 0){
        /* FIXME: change these numbers */
        animation->render(260, 230, *work, 0, 0);
    }
}                      

void Character::grabbed(Object*){
}

void Character::unGrab(){
}

bool Character::isGrabbed(){
    return false;
}

Object* Character::copy(){
    return this;
}

const std::string& Character::getAttackName(){
    return getName();
}

bool Character::collision(ObjectAttack*){
    return false;
}

int Character::getDamage() const{
    return 0;
}

bool Character::isCollidable(Object*){
    return true;
}

bool Character::isGettable(){
    return false;
}

bool Character::isGrabbable(Object*){
    return true;
}

bool Character::isAttacking(){
    return false;
}

int Character::getWidth() const{
    return groundfront;
}

int Character::getHeight() const{
    return height;
}

Network::Message Character::getCreateMessage(){
    return Network::Message();
}

void Character::getAttackCoords(int&, int&){
}

double Character::minZDistance() const{
    return 0;
}

void Character::attacked(World*, Object*, std::vector<Object*, std::allocator<Object*> >&){
}

}
