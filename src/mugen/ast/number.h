#ifndef _paintown_ast_number_h_
#define _paintown_ast_number_h_

#include "Value.h"
#include <string>
#include <sstream>

namespace Ast{

class Number: public Value {
public:
    Number(double value):
    value(value){
    }

    using Value::operator>>;

    virtual const Value & operator>>(const Value *& v) const {
        v = this;
        return *this;
    }

    virtual const Value & operator>>(int & x) const {
        x = (int) value;
        return *this;
    }

    virtual void walk(Walker & walker) const {
        walker.onNumber(*this);
    }
    
    virtual Element * copy() const {
        return new Number(value);
    }
    
    virtual const Value & operator>>(double & x) const {
        x = value;
        return *this;
    }

    Token * serialize() const {
        Token * token = new Token();
        *token << "number" << value;
        return token;
    }
    
    virtual const Value & operator>>(bool & item) const {
        /* cast to int here? probably not because then 0.1 would become false
         * can an IEEE floating point value be exactly 0 in binary?
         */
        if (value == 0){
            item = false;
        } else {
            item = true;
        }
        return *this;
    }
    
    virtual std::string getType() const {
        return "number";
    }

    virtual std::string toString() const {
        std::ostringstream out;
        // out << '[' << value << ']';
        out << value;
        return out.str();
    }

    virtual ~Number(){
    }
protected:
    double value;
};

/* this is hack to make double jumping work. try not to use this class if you
 * can help it.
 */
class MutableNumber: public Number {
public:
    MutableNumber(double value):
    Number(value){
    }

    virtual ~MutableNumber(){
    }

    double get() const {
        return this->value;
    }

    void set(double v){
        this->value = v;
    }
};

}

#endif
