#ifndef CONFIGPARSER_H
#define CONFIGPARSER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <stdexcept>
#include <string>

class ConfigParser {
public:
    ConfigParser(std::unordered_map<std::string, std::string>* externalConfig);
    void parseFile(const std::string& filename);
    void printConfig() const;
private:
    std::unordered_map<std::string, std::string>* config;
    void parseLine(const std::string& line);
    void parseDirective(const std::string& directive);
    void addVariable(const std::string& variable);
    void setVariable(const std::string& assignment);
    std::string trim(const std::string& str);
};

#endif // CONFIGPARSER_H
