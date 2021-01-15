#include <iostream>
#include <fstream>
#include <random>
#include <string>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <ctime>
#include <cstdlib>
#include <stack>
#include <dirent.h>

using namespace std;

ofstream textFileTimes;
ofstream textFileSteps;
ofstream solutionFile;
int COUNTER = 0;
// const auto MAX_THREADS = thread::hardware_concurrency();

/* Namespace to defince variations of algorithms. */
namespace Algorithm {
    enum Version {
        DEFAULT, NO_PREP, NO_AUTARK, NO_HEURISTIC, HEU_LIT,
    };
    static const Version All[] = {DEFAULT, HEU_LIT, NO_AUTARK, NO_HEURISTIC, NO_PREP};
    static const Version Default[] = {DEFAULT};

    string getVersionName(enum Version algorithm) {
        switch (algorithm) {
            case NO_PREP:
                return "No preprocessing";
            case NO_AUTARK:
                return "No autark";
            case NO_HEURISTIC:
                return "No heuristic";
            case HEU_LIT:
                return "Heuristic Literals";
            case DEFAULT:
                return "Heuristic Variables";
            default:
                return "Something went wrong?!";
        }
    }
}

void printVector(const vector<int> &vector) { /* Prints a vector. */
    cout << "[ ";
    for (int j : vector) {
        cout << j << " ";
    }
    cout << "]";
}

void printVectorOfVectors(const vector<vector<int>> &vector) { /* Prints a vector of vectors. */
    for (auto &vec : vector) {
        printVector(vec);
    }
    cout << endl;
}

struct Data {
    vector<vector<int>> cnf;
    vector<int> cnf_unsat;
    unordered_set<int> assigned_vars;
    unordered_set<int> unassigned_vars;
    stack<vector<int>> resolutions;
    unordered_map<int, vector<int>> literal_to_clause;
    bool unsat = false;
    Algorithm::Version algorithm;

    /* Constructor for the data structure. */
    explicit Data(const vector<vector<int>> &cnf, Algorithm::Version algorithm) {
        this->cnf = cnf;
        this->algorithm = algorithm;
        for (int i = 0; i < cnf.size(); ++i) {
            cnf_unsat.push_back(i);
            for (auto v : cnf[i]) {
                this->unassigned_vars.insert(abs(v));
                this->literal_to_clause[v].push_back(i);
            }
        }
    }

    /* Returns true if unsat or sat. */
    bool canAbort() const {
        return unsat || cnf_unsat.empty();
    }

    /* Returns all clauses which are not yet satisfied. */
    vector<vector<int>> getUnsatClauses() {
        vector<vector<int>> temp;
        for (auto i: cnf_unsat)
            temp.push_back(cnf[i]);
        return temp;
    }

    /* Return the number of times a given literal occurs in unsatisfied clauses. */
    int get_literal_count(int v) {
        return this->literal_to_clause[v].size();
    }

    /* Removes a clause from the unsatisfied list, but updaes all parameters accordingly. */
    void discard_clause(int index) {
        cnf_unsat.erase(remove(cnf_unsat.begin(), cnf_unsat.end(), index), cnf_unsat.end());
        for (auto v : cnf[index]) {
            literal_to_clause[v].erase(remove(literal_to_clause[v].begin(),
                                              literal_to_clause[v].end(), index), literal_to_clause[v].end());

        }
    }

    /* Adds a literal to the solution list, i.e assigns a ground-truth to a variable. */
    void add_solution(int v) {
        unassigned_vars.erase(abs(v));
        assigned_vars.insert(v);
        //Remove satisfied clauses.
        vector<int> clauses(literal_to_clause[v]);
        for (auto i : clauses) {
            cnf_unsat.erase(remove(cnf_unsat.begin(), cnf_unsat.end(), i), cnf_unsat.end());
            for (int v2 : cnf[i])
                literal_to_clause[v2].erase(remove(literal_to_clause[v2].begin(),
                                                   literal_to_clause[v2].end(), i),
                                            literal_to_clause[v2].end());
        }
        //Remove negated literal from clauses.
        for (auto i : literal_to_clause[-v]) {
            cnf[i].erase(remove(cnf[i].begin(), cnf[i].end(), -v), cnf[i].end());
            if (cnf[i].empty())
                this->unsat = true; // UNSAT!!!!
        }
        literal_to_clause[-v].clear();
        literal_to_clause[v].clear();
    }

    /* Adds multiple literals to the solution list, i.e assigns ground-truths to variables. */
    void add_solutions(const unordered_set<int> &alpha) {
        for (auto v : alpha)
            add_solution(v);
    }

    /* Performs the resolution rule as states in the script for a variable. */
    void resolve(int literal) {
        int a = literal_to_clause[literal][0];
        int b = literal_to_clause[-literal][0];
        if (a == b) {
            add_solution(literal);
            return;
        }
        unassigned_vars.erase(abs(literal));
        cnf[a].insert(cnf[a].end(), cnf[b].begin(), cnf[b].end());
        discard_clause(b);
        //Re-reference literals.
        for (int v : cnf[b]) {
            literal_to_clause[v].erase(remove(literal_to_clause[v].begin(),
                                              literal_to_clause[v].end(), a), literal_to_clause[v].end());
            literal_to_clause[v].push_back(a);
        }
        cnf[b].erase(remove(cnf[b].begin(), cnf[b].end(), -literal), cnf[b].end());
        cnf[b].push_back(-literal);
        resolutions.push(cnf[b]);
        cnf[a].erase(remove(cnf[a].begin(), cnf[a].end(), literal), cnf[a].end());
        cnf[a].erase(remove(cnf[a].begin(), cnf[a].end(), -literal), cnf[a].end());
        literal_to_clause[-literal].clear();
        literal_to_clause[literal].clear();
        // Remove duplicates.
        sort(cnf[a].begin(), cnf[a].end());
        cnf[a].erase(unique(cnf[a].begin(), cnf[a].end()), cnf[a].end());
    }
};

/* Eliminates clauses that contain both, a variable and its negation, in the same clause. */
void eliminateTautologies(Data *data) {
    vector<int> cnf_(data->cnf_unsat);
    for (auto i : cnf_) {
        for (auto c : data->cnf[i]) {
            if (find(data->cnf[i].begin(), data->cnf[i].end(), -c) != data->cnf[i].end()) {
                data->add_solution(abs(c));
                break;
            }
        }
    }
}

/* Reads a diamacsCNF file from a given path. */
vector<vector<int>> loadDimacsCnf(const string &path) {
    // cout << "Loading Dimacs-file at \"" << path << "\"." << endl;
    ifstream file(path);
    string str;
    int n_literals = 0;
    vector<vector<int>> cnf;
    while (getline(file, str)) {
        if (str[0] != 'c' && str[0] != 'p') {
            vector<int> numbers;
            str = str.substr(0, str.size() - 1);
            stringstream iss(str);
            int number;
            while (iss >> number)
                numbers.push_back(number);
            cnf.push_back(numbers);
            n_literals += numbers.size();
        }
    }
    // cout << "Done! (Clauses: " << cnf.size() << " | Literals " << n_literals << ")." << endl;
    return cnf;
}

/* Transforms any CNF to 3-SAT. */
vector<vector<int>> to3SAT(const vector<vector<int>> &cnf) {
    cout << "Transforming CNF to 3-SAT..." << endl;
    vector<vector<int>> three_sat;
    int max_var = 0;
    for (const vector<int> &clause : cnf)
        for (int literal : clause)
            max_var = max(abs(literal), max_var);
    for (vector<int> clause : cnf) {
        if (clause.size() <= 3)
            three_sat.push_back(clause);
        else {
            vector<int> v{clause[0], clause[1], ++max_var};
            three_sat.push_back(v);
            for (int i = 2; i < clause.size() - 2; i++) {
                vector<int> v1{-max_var, clause[i], ++max_var};
                three_sat.push_back(v1);
            }
            vector<int> v2{-max_var, clause[clause.size() - 2], clause[clause.size() - 1]};
            three_sat.push_back(v2);
        }
    }
    cout << "Done! (Clauses old: " << cnf.size() << " | Clauses new: " << three_sat.size() << ")."
         << endl;
    return three_sat;
}

/* Removes clauses that contain only one literal and assigns those as solutions. */
void removeUnitClauses(Data *data) {
    // cout << "Determining and removing unit clauses and literals." << endl;
    vector<int> cnf_(data->cnf_unsat);
    for (auto i : cnf_)
        if (data->cnf[i].size() == 1)
            data->add_solution(data->cnf[i][0]);
}

/* Determines and removes pure literals from clauses, and respectively includes those as solutions. */
void removePureLiterals(Data *data) {
    // cout << "Detecting and removing pure literals." << endl;
    unordered_set<int> vars_(data->unassigned_vars);
    //unassigned_vars stores only the absolute variable values.
    for (auto v : vars_) {
        int a = data->get_literal_count(v);
        int b = data->get_literal_count(-v);
        if (b == 0)
            data->add_solution(v);
        else if (a == 0)
            data->add_solution(-v);
    }
}

/* Performs the resolution rule, as states in the script (which is similar to combining clauses). */
void performResolutionRule(Data *data) {
    unordered_set<int> vars_(data->unassigned_vars);
    for (auto literal : vars_) {
        if (data->get_literal_count(literal) == 1 && data->get_literal_count(-literal) == 1)
            data->resolve(literal);
    }
}

/* Checks for autark clauses, as stated in the script. */
bool isAutark(const vector<vector<int>> &cnf, unordered_set<int> alpha) {
    for (const vector<int> &clause : cnf) {
        bool conflicts = false;
        bool satisfies = false;
        for (int v : clause) {
            if (alpha.find(v) != alpha.end())
                satisfies = true;
            else if (alpha.find(-v) != alpha.end())
                conflicts = true;
        }
        if (conflicts && !satisfies)
            return false;
    }
    return true;
}

/* Removes clauses which are satisfied by a given assignment (alpha). */
vector<vector<int>>
removeSatisfiedClauses(const vector<vector<int>> &cnf, unordered_set<int> alpha) {
    vector<vector<int>> filtered_cnf;
    for (const vector<int> &clause : cnf) {
        bool remove = false;
        for (int v : clause) {
            if (alpha.find(v) != alpha.end()) {
                remove = true;
                break;
            }
        }
        if (!remove)
            filtered_cnf.push_back(clause);
    }
    // cout << "Done! (Clauses old: " << cnf.size() << " | Clauses new: " << filtered_cnf.size() << ")." << endl;
    return filtered_cnf;
}

/* Returns true if there exists a conflict in the solution, i.e same variables is assigned two different ground-truths . */
bool isFalsified(const vector<vector<int>> &cnf, unordered_set<int> alpha) {
    if (alpha.empty())
        return false;
    for (const vector<int> &clause : cnf) {
        for (int v : clause) {
            if (!(alpha.find(v) == alpha.end() && alpha.find(-v) != alpha.end())) {
                return false;
            }
        }
    }
    return true;
}

/* Returns the smallest of all clauses in a given cnf. */
vector<int> getSmallestClause(const vector<vector<int>> &cnf) {
    int best_v = -1;
    vector<int> best_k;
    for (const auto &clause : cnf)
        if ((best_v > clause.size() || best_v == -1) && !clause.empty()) {
            best_k = clause;
            best_v = clause.size();
        }
    return best_k;
}

/* Returns a set of all variables in a cnf. */
unordered_set<int> getVariables(const vector<vector<int>> &cnf, bool absolute = false) {
    unordered_set<int> vars;
    for (const auto &clause : cnf)
        for (auto c : clause) {
            if (absolute)
                vars.insert(abs(c));
            else if (vars.find(-c) == vars.end())
                vars.insert(c);
        }
    return vars;
}

/* Performs the subsumation rule. */
void removeSubsumedClauses(Data *data) {
    vector<int> cnf_unsat_(data->cnf_unsat);
    vector<vector<int>> cnf_(data->cnf);
    for (int i : cnf_unsat_)
        sort(cnf_[i].begin(), cnf_[i].end());
    for (auto large_clause : cnf_unsat_) {
        for (auto clause : cnf_unsat_) {
            if (cnf_[large_clause].size() <= cnf_[clause].size())
                continue;
            if (includes(cnf_[large_clause].begin(), cnf_[large_clause].end(),
                         cnf_[clause].begin(), cnf_[clause].end())) {
                data->discard_clause(large_clause);
            }
        }
    }
}

/* Sort unsat clauses, used for heuristics when there exist multiple "smallest" clauses of the same size.
 * Here, solely the number of literal (not variable) occurrence is of importance!
 * We first need to sort clauses, the the order within the clauses! */
vector<vector<int>> sortUnsatClauses_literals(Data *data, int order = -1) {
    vector<tuple<int, int>> cnf_ordered;
    for (int i : data->cnf_unsat) {
        int v = 0;
        for (auto item : data->cnf[i]) {
            v += data->get_literal_count(item);
        }
        v *= order;
        cnf_ordered.emplace_back(v, i);
    }
    sort(cnf_ordered.begin(), cnf_ordered.end());

    vector<vector<int>> cnf_;
    for (auto p :cnf_ordered) {
        vector<int> clause_ = data->cnf[get<1>(p)];
        sort(clause_.begin(), clause_.end(), [&data](int x, int y) {
            int a = data->get_literal_count(x);
            int b = data->get_literal_count(y);
            return a < b;
        });
        cnf_.push_back(clause_);
    }
    return cnf_;
}

/* Sort unsat clauses, used for heuristics when there exist multiple "smallest" clauses of the same size.
 * Here, the number of variable (not literal) occurrence is of importance!
 * We first need to sort clauses, the the order within the clauses! */
vector<vector<int>> sortUnsatClauses_variables(Data *data, int order = -1) {
    vector<tuple<int, int, int>> cnf_ordered;
    for (int i : data->cnf_unsat) {
        int v = 0;
        int v2 = 0;
        for (auto item : data->cnf[i]) {
            v += data->get_literal_count(item) + data->get_literal_count(-item);
            v2 += data->get_literal_count(item) * data->get_literal_count(-item);
        }
        v *= order;
        v2 *= order;
        cnf_ordered.emplace_back(v, v2, i);
    }
    sort(cnf_ordered.begin(), cnf_ordered.end());

    vector<vector<int>> cnf_;
    for (auto p :cnf_ordered) {
        vector<int> clause_ = data->cnf[get<2>(p)];
        sort(clause_.begin(), clause_.end(), [&data](int x, int y) {
            int a = data->get_literal_count(x);
            int b = data->get_literal_count(y);
            int a2 = data->get_literal_count(-x);
            int b2 = data->get_literal_count(-y);
            if (a + a2 != b + b2)
                return a + a2 > b + b2;
            else
                return a * a2 > b * b2;
        });
        cnf_.push_back(clause_);
    }
    return cnf_;
}

/* Main logic of the sat solver; used for recursion. */
Data solveSAT(Data data) {
    if (data.canAbort())
        return data;
    COUNTER++;
    //cout << "SAT-Solving via the \"MonienSpeckenmeyer\" algorithm." << endl;
    /// PRE-FILTERING:
    if (data.algorithm != Algorithm::Version::NO_PREP) {
        ////////////////////////// REMOVE PURE LITERALS //////////////////////////
        eliminateTautologies(&data);
        ////////////////////////// REMOVE PURE LITERALS //////////////////////////
        removePureLiterals(&data);
        ////////////////////////// REMOVE UNIT CLAUSES ///////////////////////////
        removeUnitClauses(&data);
        ////////////////////////// PERFORM RESOLUTION RULE ///////////////////////
        performResolutionRule(&data);
        ////////////////////////// REMOVE SUBSUMED THREE CLAUSES ///////////////////////
        removeSubsumedClauses(&data);
    }
    ////////////////////////// CORE ALGORITHM  //////////////////////////
    if (data.canAbort())
        return data;
    vector<vector<int>> cnf;
    // HEURISTICS !!!!!!!!!!!!!!! HEURISTICS !!!!!!!!!!!!!!!
    if (data.algorithm == Algorithm::Version::NO_HEURISTIC)
        cnf = data.getUnsatClauses();
    else if (data.algorithm == Algorithm::Version::HEU_LIT)
        cnf = sortUnsatClauses_literals(&data);
    else
        cnf = sortUnsatClauses_variables(&data);
    vector<int> next_clause = getSmallestClause(cnf);
    vector<unordered_set<int>> ys;
    for (int i = 0; i < next_clause.size(); ++i) {
        unordered_set<int> assignment_new = {};
        assignment_new.insert(next_clause[i]);
        for (int j = 0; j < i; ++j)
            assignment_new.insert(-next_clause[j]);
        if (isAutark(cnf, assignment_new) && data.algorithm != Algorithm::Version::NO_AUTARK) {
            data.add_solutions(assignment_new);
            return solveSAT(data);
        }
        ys.push_back(assignment_new);
    }
    /////////////////////// CONTINUE ALGORITHM //////////////////////////
    for (const auto &y : ys) {
        Data d = data;
        d.add_solutions(y);
        if (d.unsat)
            continue;
        d = solveSAT(d);
        if (!d.unsat && d.cnf_unsat.empty())
            return d;
    }
    data.unsat = true;
    data.assigned_vars.clear();
    return data; //UNSAT
}

/* Starts the recursive sat-solver calls and sotres data accordingly in files. */
int solve_dimacs(const string &path, Algorithm::Version algorithm) {
    bool sat;
    clock_t tStart = clock();
    srand(unsigned(time(nullptr)));
    cout << "Path: " << path << endl;
    cout << "Algorithm: " << Algorithm::getVersionName(algorithm) << endl;
    vector<vector<int>> cnf = loadDimacsCnf(path);
    const unordered_set<int> &orig_vars = getVariables(cnf);
    //cout << "Clauses: " << cnf.size() << " | " << "Vars: " << orig_vars.size() << endl;
    //cnf = to3SAT(cnf); //Worsens performance!
    Data data = Data(cnf, algorithm);
    /////////
    COUNTER = 0;
    data = solveSAT(data);
    ///
    vector<int> solution;
    if (!data.unsat) {
        for (int v : data.assigned_vars)
            if (orig_vars.find(v) != orig_vars.end() || orig_vars.find(-v) != orig_vars.end())
                solution.push_back(v);
        while (!data.resolutions.empty()) {
            auto vec = data.resolutions.top();
            int literal = vec.back();
            unordered_set<int> alpha(solution.begin(), solution.end());
            if (vec.size() > 1) {
                if (removeSatisfiedClauses({vec}, alpha).empty())
                    solution.push_back(-literal);
                else
                    solution.push_back(literal);
            }
            data.resolutions.pop();
        }
        cout << "SOLVED! Solution is: ";
        sort(solution.begin(), solution.end(), [](int x, int y) { return abs(x) < abs(y); });
        printVector(solution);
        sat = true;

    } else {
        cout << "Formula is UNSAT!" << endl;
        sat = false;
    }

    if (algorithm == Algorithm::DEFAULT) {
        int beginIdx = path.rfind('/');
        std::string filename = path.substr(beginIdx + 1);
        solutionFile.open(filename);
        if (sat) {
            solutionFile << "s SATISFIABLE" << "\n";
            solutionFile << "v ";
            for (int j : solution) {
                solutionFile << j << " ";
            }
            solutionFile << 0;
        } else {
            solutionFile << "s UNSATISFIABLE" << "\n";
        }

        solutionFile.close();
    }

    long time = (clock() - tStart);
    textFileTimes << "," << time;

    unordered_set<int> solution_check(solution.begin(), solution.end());
    cout << "[Steps: " << COUNTER << "] ";
    textFileSteps << "," << COUNTER;

    printf("[Execution time: %.2fs]\n", (double) (clock() - tStart) / CLOCKS_PER_SEC);
    cout << "=====================================" << endl;
    return removeSatisfiedClauses(cnf, solution_check).empty() || data.unsat;
}

/* Helper function to read in all files in a given directory. */
vector<string> get_test_files(const char *directory) {
    vector<string> paths;
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory)) != nullptr) {
        while ((ent = readdir(dir)) != nullptr) {
            string path(ent->d_name);
            if (path.size() < 3)
                continue;
            string d(directory);
            d.append("/" + path);
            paths.emplace_back(d);
        }
        closedir(dir);
    }
    return paths;
}

/* Main loop; loads in all /test files and starts the solving process. */
int main() {
    textFileTimes.open("times.csv");
    textFileSteps.open("steps.csv");
    textFileTimes << "Algorithm";
    textFileSteps << "Algorithm";
    vector<string> paths = get_test_files("../inputs/test/sat");
    vector<string> paths2 = get_test_files("../inputs/test/unsat");
    paths.insert(paths.end(), paths2.begin(), paths2.end());
    //paths = {"../inputs/sat/uf50-04.cnf"};
    bool correct = true;

    for (int i = 0; i < paths.size(); ++i) {
        textFileTimes << "," << i;
        textFileSteps << "," << i;
    }

    for (const auto algorithm : Algorithm::All) {
        textFileTimes << "\n" << Algorithm::getVersionName(algorithm);
        textFileSteps << "\n" << Algorithm::getVersionName(algorithm);
        for (const auto &path : paths)
            correct = correct && solve_dimacs(path, algorithm);
    }

    textFileTimes.close();
    textFileSteps.close();
    if(!correct)
        std::cout << "SOLUTION NOT CORRECT: SOMETHING WENT WRONG!";
    else
        std::cout << "SOLUTION CORRECT & CHECKED!";
    return !correct;
}