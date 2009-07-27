#!/usr/bin/env python

# Packrat PEG (parsing expression grammar) generator
# http://pdos.csail.mit.edu/~baford/packrat/

next_var = 0
def nextVar():
    global next_var;
    next_var += 1;
    return next_var

def newResult():
    return "result_%d" % nextVar()

def newOut():
    return "out_%d" % nextVar()

def indent(s):
    space = '    '
    return s.replace('\n', '\n%s' % space)

start_code = """
class Result{
public:
    Result(const int position):
    position(position),
    isError(false){
    }

    inline const int getPosition() const {
        return position;
    }

    inline bool error(){
        return isError;
    }

    inline void nextPosition(){
        position += 1;
    }

    void setError(){
        isError = true;
    }

    void setValue(void * value){
        this->values.push_back(value);
    }

    const int matches() const {
        return this->values.size();
    }

    const std::vector<void *> & getValues() const {
        return values;
    }

    void addResult(const Result & result){
        const std::vector<void *> & his = result.getValues();
        this->values.insert(this->values.end(), his.begin(), his.end());
        this->position = result.getPosition();
    }

private:
    int position;
    bool isError;
    std::vector<void *> values;
};

class Stream{
public:
    Stream(const std::string & filename){
        stream.open(filename.c_str());
    }

    char get(const int position){
        char z;
        stream.seekg(position, std::ios_base::beg);
        stream >> z;
        return z;
    }

    void update(const Result & result){
    }

    bool hasResult(const int position){
        return false;
    }

    Result result(const int position){
        return Result(-1);
    }

private:
    std::ifstream stream;
};

Result errorResult(-1);
"""

class Pattern:
    def __init__(self):
        pass

    def generate(self, result, stream, failure):
        pass

class PatternNot(Pattern):
    def __init__(self, next):
        Pattern.__init__(self)
        self.next = next

    def generate_python(self, result, stream, failure):
        my_result = newResult()
        my_fail = lambda : "raise PegError"
        data = """
%s = Result(%s);
try:
    %s
except PegError:
    %s
        """ % (my_result, result, indent(self.next.generate_python(my_result, stream, my_fail).strip()), failure())

        return data


    def generate(self, result, stream, failure):
        not_label = "not_%d" % nextVar()
        my_result = newResult()
        my_fail = lambda : "goto %s;" % not_label
        data = """
Result %s(%s);
%s
%s
%s:
        """ % (my_result, result, self.next.generate(my_result, stream, my_fail).strip(), failure(), not_label)

        return data

class PatternRule(Pattern):
    def __init__(self, rule):
        Pattern.__init__(self)
        self.rule = rule

    def generate_python(self, result, stream, failure):
        data = """
%s = rule_%s(%s, %s.getPosition())
if %s == None:
    %s
""" % (result, self.rule, stream, result, result, failure())

        return data

    def generate(self, result, stream, failure):
        data = """
%s = rule_%s(%s, %s.getPosition());
if (%s.error()){
    %s
}
""" % (result, self.rule, stream, result, result, failure())

        return data

class PatternSequence(Pattern):
    def __init__(self, patterns):
        Pattern.__init__(self)
        self.patterns = patterns

    def generate_python(self, result, stream, failure):
        data = ""
        for pattern in self.patterns:
            my_result = newResult()
            data += """
%s = Result(%s.getPosition())
%s
%s.addResult(%s);
""" % (my_result, result, pattern.generate_python(my_result, stream, failure), result, my_result)

        return data

    def generate(self, result, stream, failure):
        data = ""
        for pattern in self.patterns:
            my_result = newResult()
            data += """
{
Result %s(%s.getPosition());
%s
%s.addResult(%s);
}
""" % (my_result, result, pattern.generate(my_result, stream, failure), result, my_result)
        return data

class PatternRepeatOnce(Pattern):
    def __init__(self, next):
        Pattern.__init__(self)
        self.next = next

    def generate_python(self, result, stream, failure):
        loop_done = "loop_%d" % nextVar()
        my_fail = lambda : "raise PegError"
        my_result = newResult()
        data = """
try:
    while True:
        %s = Result(%s.getPosition());
        %s
        %s.addResult(%s);
except PegError:
    if %s.matches() == 0:
        %s
        """ % (my_result, result, indent(indent(self.next.generate_python(my_result, stream, my_fail).strip())), result, my_result, result, failure())

        return data

    def generate(self, result, stream, failure):
        loop_done = "loop_%d" % nextVar()
        my_fail = lambda : "goto %s;" % loop_done
        my_result = newResult()
        data = """
do{
    Result %s(%s.getPosition());
    %s
    %s.addResult(%s);
} while (true);
%s:
if (%s.matches() == 0){
    %s
}
        """ % (my_result, result, indent(self.next.generate(my_result, stream, my_fail).strip()), result, my_result, loop_done, result, failure())

        return data

class PatternAction(Pattern):
    def __init__(self, before, code):
        Pattern.__init__(self)
        self.before = before
        self.code = code

    def generate_python(self, result, stream, failure):
        data = """
%s
if True:
    value = None
    %s
    %s.setValue(value)
""" % (self.before.generate_python(result, stream, failure).strip(), indent(self.code.strip()), result)

        return data

    def generate(self, result, stream, failure):
        data = """
%s
{
    void * value = 0;
    %s
    %s.setValue(value);
}
        """ % (self.before.generate(result, stream, failure).strip(), indent(self.code.strip()), result)

        return data

class PatternRepeatMany(Pattern):
    def __init__(self, next):
        Pattern.__init__(self)
        self.next = next

    def generate_python(self, result, stream, failure):
        my_fail = lambda : "raise PegError"
        my_result = newResult()
        data = """
try:
    while True:
        %s = Result(%s.getPosition());
        %s
        %s.addResult(%s);
except PegError:
    pass
        """ % (my_result, result, indent(indent(self.next.generate_python(my_result, stream, my_fail).strip())), result, my_result)

        return data


    def generate(self, result, stream, failure):
        loop_done = "loop_%d" % nextVar()
        my_fail = lambda : "goto %s;" % loop_done
        my_result = newResult()
        data = """
do{
    Result %s(%s.getPosition());
    %s
    %s.addResult(%s);
} while (true);
%s:
        """ % (my_result, result, indent(self.next.generate(my_result, stream, my_fail).strip()), result, my_result, loop_done)
        return data

class PatternOr(Pattern):
    def __init__(self, patterns):
        Pattern.__init__(self)
        self.patterns = patterns

    def generate(self, result, stream, failure):
        data = ""
        success = "success_%d" % nextVar()
        for pattern in self.patterns:
            out = "or_%d" % nextVar()
            my_result = newResult()
            fail = lambda : "goto %s;" % out
            if pattern == self.patterns[-1]:
                fail = failure
            data += """
{
Result %s(%s.getPosition());
%s
%s = %s;
}
goto %s;
%s:
""" % (my_result, result, pattern.generate(my_result, stream, fail).strip(), result, my_result, success, out)
        data += "%s:\n" % success
        return data

class PatternVerbatim(Pattern):
    def __init__(self, letters):
        Pattern.__init__(self)
        self.letters = letters

    def generate_python(self, result, stream, failure):
        data = """
for letter in '%s':
    if letter == %s.get(%s.getPosition()):
        %s.nextPosition()
    else:
        %s
""" % (self.letters, stream, result, result, failure())
        return data


    def generate(self, result, stream, failure):
        data = """
%s = "%s";
        """ % (result, self.letters)

        data = ""

        for letter in self.letters[::-1]:
            newdata = """
if (%s.get(%s.getPosition()) == '%s'){
    %s.nextPosition();
    %s
} else {
    %s
}
""" % (stream, result, letter, result, indent(data), failure())
            data = newdata

        # return data

        data = """
for (int i = 0; i < %d; i++){
    if ("%s"[i] == %s.get(%s.getPosition())){
        %s.nextPosition();
    } else {
        %s
    }
}
""" % (len(self.letters), self.letters, stream, result, result, failure())
        return data

class Rule:
    def __init__(self, name, patterns):
        self.name = name
        self.patterns = patterns

    def generate_python(self):
        def newPattern(pattern, stream, position):
            result = newResult()

            def fail():
                return "raise PegError"
            data = """
try:
    %s = Result(%s)
    %s
    %s.update(%s)
    return %s
except PegError:
    pass
            """ % (result, position, indent(pattern.generate_python(result, stream, fail).strip()), stream, result, result)
            return data

        stream = "stream"
        position = "position"
        data = """
def rule_%s(%s, %s):
    %s
    return None
""" % (self.name, stream, position, indent('\n'.join([newPattern(pattern, stream, position).strip() for pattern in self.patterns])))

        return data

    def generate(self):
        def newPattern(pattern, stream, position):
            result = newResult()
            out = newOut()
            def failure():
                return "goto %s;" % out
            data = """
Result %s(%s);
%s
%s.update(%s);
return %s;
%s:
            """ % (result, position, pattern.generate(result, stream, failure).strip(), stream, result, result, out)
            return data

        stream = "stream"
        position = "position"
        data = """
Result rule_%s(Stream & %s, const int %s){
    if (%s.hasResult(%s)){
        return %s.result(%s);
    }
    %s
    return errorResult;
}
        """ % (self.name, stream, position, stream, position, stream, position, indent('\n'.join([newPattern(pattern, stream, position).strip() for pattern in self.patterns])))

        return data
    
class Peg:
    def __init__(self, namespace, start, rules):
        self.namespace = namespace
        self.start = start
        self.rules = rules

    def generate_python(self):
        data = """
import peg

%s

def parse(file):
    stream = Stream(file)
    done = rule_%s(stream, 0)
    if (done.error()):
        print "Error parsing " + file
    else:
        return done.getValues()
""" % ('\n'.join([rule.generate_python() for rule in self.rules]), self.start)

        return data

    def generate(self):
        def prototype(rule):
            return "Result rule_%s(Stream &, const int);" % rule.name

        data = """
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

namespace %s{
    %s

    %s

    %s

std::vector<void *> main(const std::string & filename){
    Stream stream(filename);
    errorResult.setError();
    Result done = rule_%s(stream, 0);
    if (done.error()){
        std::cout << "Could not parse" << std::endl;
    }
    return done.getValues();
}

}
        """ % (self.namespace, start_code, indent('\n'.join([prototype(rule) for rule in self.rules])), '\n'.join([rule.generate() for rule in self.rules]), self.start)

        return data

def generate(peg):
    print peg.generate()

def test():
    s_code = """
printf("parsed cheese\\n");
value = (void *) 2;
"""
    rules = [
        Rule("s", [PatternNot(PatternVerbatim("hello")), PatternAction(PatternVerbatim("cheese"), s_code), PatternRepeatOnce(PatternVerbatim("once"))]),
        Rule("blah", [PatternRepeatMany(PatternRule("s"))]),
        Rule("or", [PatternOr([PatternVerbatim("joe"), PatternVerbatim("bob"), PatternVerbatim("sally")])]),
        Rule("all", [PatternSequence([PatternVerbatim("abc"), PatternVerbatim("def"), PatternVerbatim("ghi")])]),
    ]
    peg = Peg("Peg", "s", rules)
    generate(peg)

def create_peg(name, peg):
    import imp
    module = imp.new_module(name)
    exec peg.generate_python() in module.__dict__
    return module.parse

def test2():
    start_code_abc = """
std::cout << "Parsed abc!" << std::endl;
"""
    start_code_def = """
std::cout << "Parsed def!" << std::endl;
"""
    rules = [
        Rule("start", [
            PatternAction(PatternSequence([PatternRule("a"),PatternRule("b"), PatternRule("c")]), start_code_abc),
            PatternAction(PatternSequence([PatternRule("d"),PatternRule("e"), PatternRule("f")]), start_code_def),
        ]),
        Rule("a", [PatternVerbatim("a")]),
        Rule("b", [PatternVerbatim("b")]),
        Rule("c", [PatternVerbatim("c")]),

        Rule("d", [PatternVerbatim("d")]),
        Rule("e", [PatternVerbatim("e")]),
        Rule("f", [PatternVerbatim("f")]),
    ]

    peg = Peg("Peg", "start", rules)
    generate(peg)

def make_peg_parser():
    start_code_abc = """
print "parsed abc"
"""
    s_code = """
print "s code"
"""
    rules = [
        Rule("start", [
            PatternAction(PatternSequence([PatternRule("a"),PatternRule("b"), PatternRule("c")]), start_code_abc),
            ]),
        Rule("s", [
            PatternNot(PatternVerbatim("hello")), PatternAction(PatternVerbatim("cheese"), s_code),
            PatternRepeatOnce(PatternVerbatim("once"))]),
        Rule("blah", [PatternRepeatMany(PatternRule("s"))]),
        ]
    peg = Peg("*peg*", "start", rules)
    print peg.generate_python()

# test()
# test2()

make_peg_parser()
