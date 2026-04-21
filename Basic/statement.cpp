/*
 * File: statement.cpp
 * -------------------
 * This file implements the constructor and destructor for
 * the Statement class itself.  Your implementation must do
 * the same for the subclasses you define for each of the
 * BASIC statements.
 */

#include "statement.hpp"
#include "program.hpp"


/* Implementation of the Statement class */

int stringToInt(std::string str);

Statement::Statement() = default;

Statement::~Statement() = default;

// Implement deferred execute methods to avoid circular include issues

void EndStatement::execute(EvalState &state, Program &program) {
    (void) state;
    program.requestEnd();
}

void GotoStatement::execute(EvalState &state, Program &program) {
    (void) state;
    if (!program.hasLine(target)) error("LINE NUMBER ERROR");
    program.setNextLineNumber(target);
}

void IfStatement::execute(EvalState &state, Program &program) {
    int lv = lhs->eval(state);
    int rv = rhs->eval(state);
    bool cond = false;
    if (op == "=") cond = (lv == rv);
    else if (op == "<") cond = (lv < rv);
    else if (op == ">") cond = (lv > rv);
    if (cond) {
        if (!program.hasLine(target)) error("LINE NUMBER ERROR");
        program.setNextLineNumber(target);
    }
}
