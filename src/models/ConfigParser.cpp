#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include "ConfigParser.h"

ConfigParser::ConfigParser(std::unordered_map<std::string, std::string>* externalConfig)
    : config(externalConfig) {

}

void ConfigParser::parseFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }

    std::string line;
    while (std::getline(file, line)) {
        parseLine(line);
    }

    file.close();
}

void ConfigParser::printConfig() const {
    for (const auto& [key, value] : *config) {
        std::cout << key << " = " << value << std::endl;
    }
}

void ConfigParser::parseLine(const std::string& line) {
    std::string trimmedLine = trim(line);
    if (trimmedLine.empty()) {
        return;
    }

    if (trimmedLine[0] == '#') {
        parseDirective(trimmedLine.substr(1));
    } else if (trimmedLine.substr(0, 4) == "add ") {
        addVariable(trimmedLine.substr(4));
    } else if (trimmedLine.substr(0, 4) == "set ") {
        setVariable(trimmedLine.substr(4));
    } else {
        throw std::runtime_error("Unknown line format: " + trimmedLine);
    }
}

void ConfigParser::parseDirective(const std::string& directive) {
    std::string trimmedDirective = trim(directive);
}

void ConfigParser::addVariable(const std::string& variable) {
    std::string trimmedVariable = trim(variable);
    if (config->find(trimmedVariable) != config->end()) {
    }
   (*config)[trimmedVariable] = "";
}

void ConfigParser::setVariable(const std::string& assignment) {
    std::istringstream iss(assignment);
    std::string variable, value;
    if (!(iss >> variable)) {
        throw std::runtime_error("Invalid set command format.");
    }
    if (!(iss >> value)) {
        throw std::runtime_error("Value not provided for variable " + variable);
    }
    if (config->find(variable) == config->end()) {
        throw std::runtime_error("Variable " + variable + " is not defined.");
    }
    (*config)[variable] = value;
}

std::string ConfigParser::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t");
    size_t end = str.find_last_not_of(" \t");
    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}
