/*
 * File: statement.h
 * -----------------
 * This file defines the Statement abstract type.  In
 * the finished version, this file will also specify subclasses
 * for each of the statement types.  As you design your own
 * version of this class, you should pay careful attention to
 * the exp.h interface, which is an excellent model for
 * the Statement class hierarchy.
 */

#ifndef _statement_h
#define _statement_h

#include <string>
#include <sstream>
#include "evalstate.hpp"
#include "exp.hpp"
#include "Utils/tokenScanner.hpp"
#include <iostream>
#include "parser.hpp"
#include <vector>
#include "Utils/error.hpp"
#include "Utils/strlib.hpp"

class Program;

inline bool isKeywordName(const std::string &name) {
    std::string up = toUpperCase(name);
    return up == "REM" || up == "LET" || up == "PRINT" || up == "INPUT" || up == "END" ||
           up == "GOTO" || up == "IF" || up == "THEN" || up == "RUN" || up == "LIST" ||
           up == "CLEAR" || up == "QUIT" || up == "HELP";
}

/*
 * Class: Statement
 * ----------------
 * This class is used to represent a statement in a program.
 * The model for this class is Expression in the exp.h interface.
 * Like Expression, Statement is an abstract class with subclasses
 * for each of the statement and command types required for the
 * BASIC interpreter.
 */

class Statement {

public:

/*
 * Constructor: Statement
 * ----------------------
 * The base class constructor is empty.  Each subclass must provide
 * its own constructor.
 */

    Statement();

/*
 * Destructor: ~Statement
 * Usage: delete stmt;
 * -------------------
 * The destructor deallocates the storage for this expression.
 * It must be declared virtual to ensure that the correct subclass
 * destructor is called when deleting a statement.
 */

    virtual ~Statement();

/*
 * Method: execute
 * Usage: stmt->execute(state);
 * ----------------------------
 * This method executes a BASIC statement.  Each of the subclasses
 * defines its own execute method that implements the necessary
 * operations.  As was true for the expression evaluator, this
 * method takes an EvalState object for looking up variables or
 * controlling the operation of the interpreter.
 */

    virtual void execute(EvalState &state, Program &program) = 0;

};


/*
 * The remainder of this file must consists of subclass
 * definitions for the individual statement forms.  Each of
 * those subclasses must define a constructor that parses a
 * statement from a scanner and a method called execute,
 * which executes that statement.  If the private data for
 * a subclass includes data allocated on the heap (such as
 * an Expression object), the class implementation must also
 * specify its own destructor method to free that memory.
 */

// REM
class RemStatement : public Statement {
public:
    explicit RemStatement(TokenScanner &scanner) {
        // Consume the rest tokens for comment; no validation needed
        while (scanner.hasMoreTokens()) scanner.nextToken();
    }
    void execute(EvalState &state, Program &program) override {(void) state; (void) program;}
};

// LET
class LetStatement : public Statement {
public:
    explicit LetStatement(TokenScanner &scanner) {
        // Expect: IDENT = EXP
        // Read identifier
        std::string var = scanner.nextToken();
        if (var == "" || scanner.getTokenType(var) != WORD) error("SYNTAX ERROR");
        if (isKeywordName(var)) error("SYNTAX ERROR");
        ident = var;
        // '='
        std::string eq = scanner.nextToken();
        if (eq != "=") error("SYNTAX ERROR");
        // Parse expression
        expr = readE(scanner);
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    }
    ~LetStatement() override { delete expr; }
    void execute(EvalState &state, Program &program) override {
        (void) program;
        // Use CompoundExp("=") semantics
        IdentifierExp lhs(ident);
        CompoundExp assign("=", new IdentifierExp(ident), expr);
        // Note: assign takes ownership of expr; prevent double delete
        expr = nullptr;
        assign.eval(state);
    }
private:
    std::string ident;
    Expression *expr{nullptr};
};

// PRINT
class PrintStatement : public Statement {
public:
    explicit PrintStatement(TokenScanner &scanner) {
        expr = parseExp(scanner);
    }
    ~PrintStatement() override { delete expr; }
    void execute(EvalState &state, Program &program) override {
        (void) program;
        int v = expr->eval(state);
        std::cout << v << std::endl;
    }
private:
    Expression *expr{nullptr};
};

// INPUT
class InputStatement : public Statement {
public:
    explicit InputStatement(TokenScanner &scanner) {
        std::string var = scanner.nextToken();
        if (var == "" || scanner.getTokenType(var) != WORD) error("SYNTAX ERROR");
        if (isKeywordName(var)) error("SYNTAX ERROR");
        ident = var;
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    }
    void execute(EvalState &state, Program &program) override {
        (void) program;
        while (true) {
            std::cout << " ? ";
            std::string line;
            if (!std::getline(std::cin, line)) {
                // Treat as invalid number and continue; demo appears to keep prompting
                continue;
            }
            TokenScanner sc;
            sc.ignoreWhitespace();
            sc.scanNumbers();
            sc.setInput(line);
            std::string tok = sc.nextToken();
            if (tok == "") {
                std::cout << "INVALID NUMBER" << std::endl;
                continue;
            }
            TokenType tt = sc.getTokenType(tok);
            if (tt != NUMBER) {
                std::cout << "INVALID NUMBER" << std::endl;
                continue;
            }
            if (sc.hasMoreTokens()) {
                std::cout << "INVALID NUMBER" << std::endl;
                continue;
            }
            int val = stringToInteger(tok);
            state.setValue(ident, val);
            break;
        }
    }
private:
    std::string ident;
};

// END
class EndStatement : public Statement {
public:
    explicit EndStatement(TokenScanner &scanner) {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    }
    void execute(EvalState &state, Program &program) override;
};

// GOTO
class GotoStatement : public Statement {
public:
    explicit GotoStatement(TokenScanner &scanner) {
        std::string tok = scanner.nextToken();
        if (tok == "" || scanner.getTokenType(tok) != NUMBER) error("SYNTAX ERROR");
        target = stringToInteger(tok);
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    }
    void execute(EvalState &state, Program &program) override;
private:
    int target{0};
};

// IF ... THEN
class IfStatement : public Statement {
public:
    explicit IfStatement(TokenScanner &scanner) {
        // Split tokens around relational operator and THEN
        std::vector<std::string> lhsToks, rhsToks;
        bool foundOp = false;
        while (true) {
            std::string tok = scanner.nextToken();
            if (tok == "") error("SYNTAX ERROR");
            std::string up = toUpperCase(tok);
            if (!foundOp && (tok == "=" || tok == "<" || tok == ">")) {
                op = tok;
                foundOp = true;
                continue;
            }
            if (up == "THEN") break;
            if (!foundOp) lhsToks.push_back(tok);
            else rhsToks.push_back(tok);
        }
        if (!foundOp || lhsToks.empty() || rhsToks.empty()) error("SYNTAX ERROR");

        auto joinVec = [](const std::vector<std::string> &v) {
            std::string s;
            for (size_t i = 0; i < v.size(); ++i) {
                if (i) s += ' ';
                s += v[i];
            }
            return s;
        };
        TokenScanner ls; ls.ignoreWhitespace(); ls.scanNumbers(); ls.setInput(joinVec(lhsToks));
        TokenScanner rs; rs.ignoreWhitespace(); rs.scanNumbers(); rs.setInput(joinVec(rhsToks));
        lhs = parseExp(ls);
        rhs = parseExp(rs);

        std::string lineTok = scanner.nextToken();
        if (lineTok == "" || scanner.getTokenType(lineTok) != NUMBER) error("SYNTAX ERROR");
        target = stringToInteger(lineTok);
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
    }
    ~IfStatement() override { delete lhs; delete rhs; }
    void execute(EvalState &state, Program &program) override;
private:
    Expression *lhs{nullptr};
    Expression *rhs{nullptr};
    std::string op;
    int target{0};
};

#endif
