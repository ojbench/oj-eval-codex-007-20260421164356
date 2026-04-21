/*
 * File: program.cpp
 * -----------------
 * This file is a stub implementation of the program.h interface
 * in which none of the methods do anything beyond returning a
 * value of the correct type.  Your job is to fill in the bodies
 * of each of these methods with an implementation that satisfies
 * the performance guarantees specified in the assignment.
 */

#include "program.hpp"
#include "Utils/error.hpp"



Program::Program() = default;

Program::~Program() {
    clear();
}

void Program::clear() {
    for (auto &kv : parsed) {
        delete kv.second;
    }
    parsed.clear();
    sourceLines.clear();
    nextLineOverride = 0;
    endRequested = false;
}

void Program::addSourceLine(int lineNumber, const std::string &line) {
    // If line exists, replace text and delete parsed statement
    auto it = sourceLines.find(lineNumber);
    if (it != sourceLines.end()) {
        it->second = line;
        auto pit = parsed.find(lineNumber);
        if (pit != parsed.end()) {
            delete pit->second;
            parsed.erase(pit);
        }
    } else {
        sourceLines.emplace(lineNumber, line);
    }
}

void Program::removeSourceLine(int lineNumber) {
    auto it = sourceLines.find(lineNumber);
    if (it != sourceLines.end()) {
        sourceLines.erase(it);
    }
    auto pit = parsed.find(lineNumber);
    if (pit != parsed.end()) {
        delete pit->second;
        parsed.erase(pit);
    }
}

std::string Program::getSourceLine(int lineNumber) {
    auto it = sourceLines.find(lineNumber);
    if (it == sourceLines.end()) return "";
    return it->second;
}

void Program::setParsedStatement(int lineNumber, Statement *stmt) {
    if (sourceLines.find(lineNumber) == sourceLines.end()) {
        delete stmt;
        error("LINE NUMBER ERROR");
    }
    auto pit = parsed.find(lineNumber);
    if (pit != parsed.end()) {
        delete pit->second;
        pit->second = stmt;
    } else {
        parsed.emplace(lineNumber, stmt);
    }
}

//void Program::removeSourceLine(int lineNumber) {

Statement *Program::getParsedStatement(int lineNumber) {
   auto it = parsed.find(lineNumber);
   if (it == parsed.end()) return nullptr;
   return it->second;
}

int Program::getFirstLineNumber() {
    if (sourceLines.empty()) return -1;
    return sourceLines.begin()->first;
}

int Program::getNextLineNumber(int lineNumber) {
    auto it = sourceLines.upper_bound(lineNumber);
    if (it == sourceLines.end()) return -1;
    return it->first;
}

//more func to add
//todo

bool Program::hasLine(int lineNumber) {
    return sourceLines.find(lineNumber) != sourceLines.end();
}

void Program::setNextLineNumber(int lineNumber) {
    nextLineOverride = lineNumber;
}

int Program::consumeNextLineNumber() {
    int v = nextLineOverride;
    nextLineOverride = 0;
    return v;
}

void Program::requestEnd() {
    endRequested = true;
}

bool Program::consumeEndRequested() {
    bool v = endRequested;
    endRequested = false;
    return v;
}

