#include "serialize.h"
#include "util/token.h"
#include "compiler.h"

using std::vector;
using std::string;

namespace Mugen{

Token * serialize(const RuntimeValue & value){
    Token * token = new Token();
    switch (value.getType()){
        case RuntimeValue::Invalid: {
            break;
        }
        case RuntimeValue::Bool: {
            *token << "bool" << value.getBoolValue();
            break;
        }
        case RuntimeValue::String: {
            *token << "string" << value.getStringValue();
            break;
        }
        case RuntimeValue::Double: {
            *token << "double" << value.getDoubleValue();
            break;
        }
        case RuntimeValue::ListOfString: {
            *token << "list-string";
            for (vector<string>::const_iterator it = value.strings_value.begin(); it != value.strings_value.end(); it++){
                *token << *it;
            }
            break;
        }
        case RuntimeValue::RangeType: {
            *token << "range" << value.range.low << value.range.high;
            break;
        }
        case RuntimeValue::StateType: {
            *token << "state" <<
                value.attribute.standing <<
                value.attribute.crouching <<
                value.attribute.lying <<
                value.attribute.aerial;
            break;
        }
        case RuntimeValue::AttackAttribute: {
            *token << "attack";
            for (vector<AttackType::Attribute>::const_iterator it = value.attackAttributes.begin(); it != value.attackAttributes.end(); it++){
                *token << *it;
            }
            break;
        }
        case RuntimeValue::ListOfInt: {
            *token << "ints";
            for (vector<int>::const_iterator it = value.ints_value.begin(); it != value.ints_value.end(); it++){
                *token << *it;
            }

            break;
        }
    }

    return token;
}

Token * serialize(const AttackType::Attribute data){
    return NULL;
}

Token * serialize(const AttackType::Animation data){
    return NULL;
}

Token * serialize(const AttackType::Ground data){
    return NULL;
}

Token * serialize(const TransType data){
    return NULL;
}

Token * serialize(const CharacterId & data){
    return NULL;
}

Token * serialize(const std::vector<CharacterId> & data){
    return NULL;
}

Token * serialize(const std::string & data){
    return NULL;
}

}
