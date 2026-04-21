/*
 * File: Basic.cpp
 * ---------------
 * This file is the starter project for the BASIC interpreter.
 */

#include <cctype>
#include <iostream>
#include <string>
#include "exp.hpp"
#include "parser.hpp"
#include "program.hpp"
#include "Utils/error.hpp"
#include "Utils/tokenScanner.hpp"
#include "Utils/strlib.hpp"


/* Function prototypes */

void processLine(std::string line, Program &program, EvalState &state);

/* Main program */

static bool g_quit = false;

int main() {
    EvalState state;
    Program program;
    //cout << "Stub implementation of BASIC" << endl;
    while (!g_quit) {
        try {
            std::string input;
            if (!std::getline(std::cin, input)) {
                break;
            }
            if (input.empty())
                continue;
            processLine(input, program, state);
        } catch (ErrorException &ex) {
            std::cout << ex.getMessage() << std::endl;
        }
    }
    return 0;
}

/*
 * Function: processLine
 * Usage: processLine(line, program, state);
 * -----------------------------------------
 * Processes a single line entered by the user.  In this version of
 * implementation, the program reads a line, parses it as an expression,
 * and then prints the result.  In your implementation, you will
 * need to replace this method with one that can respond correctly
 * when the user enters a program line (which begins with a number)
 * or one of the BASIC commands, such as LIST or RUN.
 */

void processLine(std::string line, Program &program, EvalState &state) {
    TokenScanner scanner;
    scanner.ignoreWhitespace();
    scanner.scanNumbers();
    scanner.setInput(line);

    std::string first = scanner.nextToken();
    if (first == "") return;
    TokenType tt = scanner.getTokenType(first);
    if (tt == NUMBER) {
        // Numbered program line
        int lineNumber = stringToInteger(first);
        std::string rest;
        // Reconstruct remaining text exactly as input after first token
        // Since TokenScanner loses exact spacing, store original line as-is
        // Requirement: preserve leading zeros in LIST, so use the exact original input
        // For deletion: if no rest tokens
        if (!scanner.hasMoreTokens()) {
            program.removeSourceLine(lineNumber);
            return;
        }
        // Store source line exactly as given (including original line number formatting)
        program.addSourceLine(lineNumber, line);

        // Parse statement type from rest
        TokenScanner restScanner;
        restScanner.ignoreWhitespace();
        restScanner.scanNumbers();
        // Determine rest of the line text after the first space
        size_t sp = line.find(' ');
        std::string restText = (sp == std::string::npos) ? std::string() : line.substr(sp + 1);
        restScanner.setInput(restText);
        std::string cmd = toUpperCase(restScanner.nextToken());
        Statement *stmt = nullptr;
        if (cmd == "REM") {
            stmt = new RemStatement(restScanner);
        } else if (cmd == "LET") {
            stmt = new LetStatement(restScanner);
        } else if (cmd == "PRINT") {
            stmt = new PrintStatement(restScanner);
        } else if (cmd == "INPUT") {
            stmt = new InputStatement(restScanner);
        } else if (cmd == "END") {
            stmt = new EndStatement(restScanner);
        } else if (cmd == "GOTO") {
            stmt = new GotoStatement(restScanner);
        } else if (cmd == "IF") {
            stmt = new IfStatement(restScanner);
        } else {
            error("SYNTAX ERROR");
        }
        program.setParsedStatement(lineNumber, stmt);
        return;
    }

    // Immediate commands
    std::string cmd = toUpperCase(first);
    if (cmd == "REM") {
        while (scanner.hasMoreTokens()) scanner.nextToken();
        return;
    } else if (cmd == "LET") {
        LetStatement stmt(scanner);
        stmt.execute(state, program);
        return;
    } else if (cmd == "PRINT") {
        PrintStatement stmt(scanner);
        stmt.execute(state, program);
        return;
    } else if (cmd == "INPUT") {
        InputStatement stmt(scanner);
        stmt.execute(state, program);
        return;
    } else if (cmd == "END") {
        // In immediate mode, spec demo prints SYNTAX ERROR
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        error("SYNTAX ERROR");
        return;
    } else if (cmd == "GOTO") {
        // Immediate GOTO is syntax error per demo
        error("SYNTAX ERROR");
        return;
    } else if (cmd == "IF") {
        // Immediate IF-THEN is part of program mode only
        error("SYNTAX ERROR");
        return;
    } else if (cmd == "LIST") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        // Print stored program in ascending order, preserving original source line text
        int ln = program.getFirstLineNumber();
        while (ln != -1) {
            std::string src = program.getSourceLine(ln);
            std::cout << src << std::endl;
            ln = program.getNextLineNumber(ln);
        }
        return;
    } else if (cmd == "CLEAR") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        program.clear();
        state.Clear();
        return;
    } else if (cmd == "RUN") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        // Execute program from first line
        int ln = program.getFirstLineNumber();
        while (ln != -1) {
            Statement *stmt = program.getParsedStatement(ln);
            if (stmt == nullptr) {
                // Parse on demand if missing
                std::string full = program.getSourceLine(ln);
                size_t sp2 = full.find(' ');
                std::string rest = (sp2 == std::string::npos) ? std::string() : full.substr(sp2 + 1);
                TokenScanner rs;
                rs.ignoreWhitespace();
                rs.scanNumbers();
                rs.setInput(rest);
                std::string rcmd = toUpperCase(rs.nextToken());
                if (rcmd == "REM") stmt = new RemStatement(rs);
                else if (rcmd == "LET") stmt = new LetStatement(rs);
                else if (rcmd == "PRINT") stmt = new PrintStatement(rs);
                else if (rcmd == "INPUT") stmt = new InputStatement(rs);
                else if (rcmd == "END") stmt = new EndStatement(rs);
                else if (rcmd == "GOTO") stmt = new GotoStatement(rs);
                else if (rcmd == "IF") stmt = new IfStatement(rs);
                else error("SYNTAX ERROR");
                program.setParsedStatement(ln, stmt);
            }
            stmt->execute(state, program);
            if (program.consumeEndRequested()) break;
            int nextOverride = program.consumeNextLineNumber();
            if (nextOverride != 0) {
                if (!program.hasLine(nextOverride)) error("LINE NUMBER ERROR");
                ln = nextOverride;
            } else {
                ln = program.getNextLineNumber(ln);
            }
        }
        return;
    } else if (cmd == "QUIT") {
        if (scanner.hasMoreTokens()) error("SYNTAX ERROR");
        g_quit = true;
        return;
    } else if (cmd == "HELP") {
        // optional; not tested
        return;
    }

    error("SYNTAX ERROR");
}
