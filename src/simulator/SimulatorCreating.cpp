
#include "TaskSchedulerSimulator.h"
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <mutex>
#include <map>
#include <cstring>
#include <regex>
#include <iostream>
#include <unordered_map>
#include <string>
#include <set>
#include <random>
#include <iterator>

std::string trim(const std::string &s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

int Cost_List::Load_Config(){
    try {
        ConfigParser parser(&CostListConfig);
        parser.parseFile("config.dat");
        //parser.printConfig();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

std::vector<int> parseNumbers(const std::string& input) {
    std::vector<int> numbers;
    std::stringstream ss(input);
    std::string token;
    while (std::getline(ss, token, '+')) {
        std::stringstream numStream(token);
        int number;
        if (numStream >> number) {
            numbers.push_back(number);
        }
    }
    return numbers;
}

int Cost_List::Load_From_File(const std::string& filename) {
    Load_Config();
    clear();
    clearNUM();
    tasks_amount = 0;
    std::ifstream file(filename);
    if (!file.is_open()) {
        return -1;
    }
    std::cout << " WCZYTANO";
    std::string line;
    int section = -1;
    int task_id,weight,to,tasks;
    int HW_cost,HW_type;
    std::vector<std::vector<int>> times_matrix;
    std::vector<std::vector<int>> costs_matrix;
    std::map<int,std::set<int>> taskToSubTask;
    std::vector<int> row_times;
    std::vector<int> row_costs;
    Graf loaded;
    int HW_ID_counter = 0;
    int taskIDCounter =0;
    while (getline(file, line)) {
        if (line.find("@tasks") != std::string::npos) {
            section = 0;
            //std::cout << "Sekcja @tasks" << std::endl;
        } else if (line.find("@proc") != std::string::npos) {
            section = 1;
            //std::cout << "Sekcja @proc" << std::endl;
        } else if (line.find("@times") != std::string::npos) {
            section = 2;
            taskIDCounter = 0;
        } else if (line.find("@cost") != std::string::npos) {
            section = 3;
            taskIDCounter = 0;
            std::cout << "Sekcja @cost" << std::endl;
        } else if (line.find("@comm") != std::string::npos) {
            section = 4;
            std::cout << "Sekcja @comm" << std::endl;
        } else if (line.find("@conditions") != std::string::npos) {
            section = 5;
            //std::cout << "Sekcja @comm" << std::endl;
        } else {
            if (section != -1) {
            }
            if (section == 0) {
                std::stringstream ss(line);
                std::string task_number;
                std::smatch match;
                std::regex task_id_regex("([A-Za-z]+)(\\d+)");
                
                if (ss >> task_number && std::regex_match(task_number, match, task_id_regex)) {
                    std::string letters = match.str(1); // Litery przed numerem
                    std::string number = match.str(2);  // Numer zadania
                    
                    int task_id = std::stoi(number); // Konwersja numeru zadania na int
                    if (letters[0] == 'U') {
                        unpredictedTasks.insert(task_id);
                    }
                    if (letters[0] == 'C') {
                        conditionalTasks.insert(task_id);
                    }
                    
                    std::string remaining_line;
                    std::getline(ss, remaining_line);
                    
                    // Usuwanie fragmentów w nawiasach kwadratowych
                    std::regex square_bracket_regex("\\[[^\\]]*\\]");
                    remaining_line = std::regex_replace(remaining_line, square_bracket_regex, "");
                    
                    std::stringstream remaining_ss(remaining_line);
                    std::string token;

                    while (remaining_ss >> token) {
                        std::regex edge_regex("(\\d+)\\((\\d+)\\)");
                        std::smatch edge_match;
                        if (std::regex_match(token, edge_match, edge_regex)) {
                            int to = std::stoi(edge_match.str(1));
                            int weight = std::stoi(edge_match.str(2));
                            if (weight == 0) weight = 1;
                            loaded.addEdge(task_id, to, weight);
                        }
                    }
                }
            }
            
            if(section == 1){
                
                char c;
                std::stringstream ss(line);
                
                while(ss >> HW_cost >> to >> HW_type){
                    Hardware hw = Hardware(HW_type,HW_cost,HW_ID_counter);
                    Hardwares.push_back(hw);
                    HW_ID_counter++;
                }
            }
            if(section == 2){ //time
                std::stringstream ss(line);
                if (line.find('[') != std::string::npos && line.find(']') != std::string::npos) {
                    std::string::size_type start = 0;
                    std::string::size_type end = 0;
                    int SubTaskCounter = 0;
                    int HWid = 0;
                    while ((start = line.find('[', end)) != std::string::npos) {
                        extendedTasks.insert(taskIDCounter);
                        end = line.find(']', start);
                        if (end == std::string::npos) {
                            throw std::runtime_error("Mismatched brackets in input line.");
                        }
                        
                        std::string group = line.substr(start + 1, end - start - 1);

                        std::vector<int> subTasksTimes = parseNumbers(group);
                        SubTaskCounter = 0;
                        for(int i : subTasksTimes){
                            sumTasksTable.setSubTaskTime(taskIDCounter,SubTaskCounter,HWid,i);
                            SubTaskCounter ++;
                        }
                        HWid++;
                    }
                    std::vector<int> emptyVec;
                    for(int i = 0; i<HWid;i++){
                        emptyVec.push_back(1);
                    }
                    times_matrix.push_back(emptyVec);

                } else {
                    int value;
                    std::vector<int> row_times;
                    while (ss >> value) {
                        row_times.push_back(value);
                    }
                    
                    times_matrix.push_back(row_times);
                }
            } // cost
            if(section ==3 ){
                std::stringstream ss(line);
                if (line.find('[') != std::string::npos && line.find(']') != std::string::npos) {
                    std::string::size_type start = 0;
                    std::string::size_type end = 0;
                    int SubTaskCounter = 0;
                    int HWid = 0;
                    while ((start = line.find('[', end)) != std::string::npos) {
                        end = line.find(']', start);
                        if (end == std::string::npos) {
                            throw std::runtime_error("Mismatched brackets in input line.");
                        }
                        std::string group = line.substr(start + 1, end - start - 1);
                        std::vector<int> subTasksCosts = parseNumbers(group);
                        SubTaskCounter = 0;
                        for(int i : subTasksCosts){
                            sumTasksTable.setSubTaskCost(taskIDCounter,SubTaskCounter,HWid,i);
                            SubTaskCounter ++;
                        }
                        HWid++;
                    }
                    std::vector<int> emptyVec;
                    for(int i = 0; i<HWid;i++){
                        emptyVec.push_back(1);
                    }
                    costs_matrix.push_back(emptyVec);

                } else {
                    int value;
                    std::vector<int> row_times;
                    while (ss >> value) {
                        row_times.push_back(value);
                    }
                    
                    costs_matrix.push_back(row_times);
                }
                
            }
            if(section == 4){
                char c;
                int chanNum;
                int ComCost;
                int BandWidth;
                int connection;
                int i =0;
                std::stringstream ss(line);
                while(ss >> c >> c >> c >> c >> chanNum >> ComCost >> BandWidth){
                    COM c = COM(ComCost,BandWidth,chanNum);
                    while(ss >> connection){
                        if(connection == 1){
                            c.add_Hardware(&Hardwares[i]);
                        }
                        i++;
                    }
                    Channels.push_back(c);
                }
                times.setTimesMatrix(times_matrix);
                times.setCostsMatrix(costs_matrix);
                TaskGraph = loaded;
                // std::ofstream matrix_file("matrix.dat");
                // if (!matrix_file.is_open()) {
                //     return -1;
                // }
                // TaskGraph.printMatrix(matrix_file);
                
            }
            if(section == 5) {
                conditions.clear();
                std::regex condition_regex(R"(C(\d+)\(([^)]+)\))");
                std::smatch match;
                
                //std::cout << "Processing line: " << line << std::endl;

                line.erase(line.begin(), std::find_if_not(line.begin(), line.end(), [](int ch) {
                    return std::isspace(ch);
                }));
                line.erase(std::find_if_not(line.rbegin(), line.rend(), [](int ch) {
                    return std::isspace(ch);
                }).base(), line.end());

                if (std::regex_match(line, match, condition_regex)) {
                    int condition_id = std::stoi(match.str(1));
                    std::string condition = match.str(2);
                    conditions[condition_id] = condition;
                    std::cout << "Condition ID: " << condition_id << " Condition: " << condition << std::endl;

                    std::regex variable_regex(R"(([A-Za-z_]\w*))");
                    std::sregex_iterator var_it(condition.begin(), condition.end(), variable_regex);
                    std::sregex_iterator end_it;

                    bool all_variables_present = std::all_of(var_it, end_it, [&](const auto& match) {
                        std::string variable = match.str();
                        std::string trimmedVariable = trim(variable);
                        if (CostListConfig.find(trimmedVariable) == CostListConfig.end()) {
                            std::cerr << "Error: Variable " << trimmedVariable << " not found in config." << std::endl;
                            return false;
                        }
                        std::string value = CostListConfig[trimmedVariable];
                        std::cout << "Variable: " << trimmedVariable << " Value: " << value << std::endl;
                        return !value.empty();
                    });

                    std::string variable, op, value_str;
                    std::istringstream iss(condition);
                    iss >> variable >> op >> value_str;
                    conditionTaskMap[condition_id] = {variable, op, std::stoi(value_str)};
                    int condition_met = evaluateCondition(condition_id);

                    std::cout << "Condition execution: ";

                    if(condition_met == 1){
                        std::cout << "Execute\n";
                    }
                    else if(condition_met == 0){
                        std::cout << "Skip\n";
                    }
                    else{
                        std::cout << "Cannot determine\n";
                    }
                } else {
                    std::cout << "Line did not match the pattern: " << line << std::endl;
                }


            }
            taskIDCounter++;
        }
    }
    std::cout << "\nWCZYTANO GRAF\n";
    file.close();
    return 1;
}

void Cost_List::createRandomTasksGraph() {
    Load_Config();
    if (tasks_amount <= 1) {
        std::cerr << "Błędna liczba zadań";
        return;
    }
    std::set<int> visited;
    Graf G;
    if (with_cost) {
        G.addEdge(0, 1, getRand(SCALE));
    } else {
        G.addEdge(0, 1);
    }
    visited.insert(0);
    for (int i : visited) {
        for (int j = 0; j < tasks_amount; j++) {
            if (randomBool(tasks_amount / 6) && !visited.count(j)) {
                if (with_cost) {
                    G.addEdge(i, j, getRand(SCALE));
                } else {
                    G.addEdge(i, j);
                }
                visited.insert(j);
            }
        }
    }

    for (int i = 0; i < tasks_amount; ++i) {
        if (visited.find(i) == visited.end()) {
            //std::cout << "NIE DODANO " << i << std::endl;
            int idx = rand() % visited.size();
            auto it = visited.begin();
            for (int k = 0; k < idx; k++) {
                it++;
            }
            int random_visited_task = *it;
            //std::cout << "RANDOM VISITED = " << random_visited_task << "\n";

            if (!G.checkEdge(random_visited_task, i)) {
                if (with_cost) {
                    G.addEdge(random_visited_task, i, getRand(SCALE));
                } else {
                    G.addEdge(random_visited_task, i);
                }
                visited.insert(i);
            }
        }
    }
    //std::cout << "tutut***" << std::endl;
    visited.clear();
    TaskGraph = G;
    progress.resize(TaskGraph.getVerticesSize(), -2);
    //std::cout << "SKOŃCZYŁEM" << std::endl;
    return;
}

void Cost_List::createRandomConditionalTasksGraph() {
    for(int t=0; t<tasks_amount;t++){
        if(t != 0 && randomBool(tasks_amount / 10)){
            makeConditional(t);
        }
    }
}

void Cost_List::makeConditional(int Task_ID) {

    conditionalTasks.insert(Task_ID);

    std::set<std::string> possibleVariables;
    for (const auto& pair : CostListConfig) {
        possibleVariables.insert(pair.first);
    }
    possibleVariables.insert("CRITICAL_TIME");
    possibleVariables.insert("TOTAL_COST");
    possibleVariables.insert("HC_INSTANCES");
    possibleVariables.insert("PE_INSTANCES");
    possibleVariables.insert("CURR_TIME");

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, possibleVariables.size() - 1);

    auto it = possibleVariables.begin();
    std::advance(it, dis(gen));
    std::string randomElement = *it;

    std::vector<std::string> operators = {"=", "<", ">", "<=", ">="};

    std::uniform_int_distribution<> dis1(0, operators.size() - 1);

    std::string randomOperator = operators[dis1(gen)];
    
    conditionTaskMap[Task_ID] = {randomElement, randomOperator, getRand(500)};

}