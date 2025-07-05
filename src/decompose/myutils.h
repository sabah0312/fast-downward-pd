//
// Created by sabah on 12/21/22.
//

#ifndef FAST_DOWNWARD_MYUTILS_H
#define FAST_DOWNWARD_MYUTILS_H


#include <fstream>
#include <sstream>
#include <regex>
//#include "../plan_manager.h"
//#include "../task_proxy.h"
//#include "orderings.h"
#include "string.h"


using namespace std;
// int myID;

//extern class Utils{
//    static bool has_elm(vector<int> *v, int i ){
//        if (count(v->begin(), v->end(), i) > 0) return true;
//        return false;
//    }
//};

//namespace m_tasks{
//    extern shared_ptr<AbstractTask> task;
//    extern void initialize();
//}
string static trim(const string &s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }

    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));

    return string(start, end + 1);
}

vector<string> static get_tokens(string str, char delim){
    vector <string> tokens;
    stringstream check1(str);
    string intermediate;
    while(getline(check1, intermediate, delim))
        tokens.push_back(intermediate);
    return tokens;
}

string static replace_string(string s, string r, string w){
    return std::regex_replace(s, std::regex(r), w);
}

bool static is_in_vector(int i, vector<int> *v){
    if (count(v->begin(), v->end(), i) > 0) return true;
    return false;
}

void static delete_element_from_vector(vector<int> *v, int elem){
    auto it = find(v->begin(), v->end(), elem);
    if (it != v->end())
        v->erase(it);
}

tuple<string, string, string, string> static extract_necessary_path(string domainfile, string problemfile, string planfile){
    string domain_path = "";
    vector<string> tokens = get_tokens(domainfile, '/');
    string domain_name = get_tokens(tokens.back(), '.').front();

    tokens.erase(tokens.begin()+ tokens.size() - 1);
    for(string str: tokens){
        domain_path = domain_path + "/" + str;
    }
    string problem_name = get_tokens(get_tokens(problemfile, '/').back(), '.').front();
//    string problem_path = domain_file_path + "/" +problem_name;
    string plan_name = get_tokens(planfile, '/').back();
//    string problem_plan_path = problem_path + "/" +plan_name;
    vector<string> domain_tokens = get_tokens(domain_path, '/');
    domain_name = domain_tokens[domain_tokens.size() - 1];
    string contest = domain_tokens[domain_tokens.size() - 2];
    return tuple<string, string, string, string>(contest, domain_name, problem_name, plan_name);
}

void static write_log_file(string s){
    ofstream logout;
    logout.open ( "log.txt",  ios_base::app);
    logout << s;
    logout.close();
}

static void write_in_file(string file_name, string s){
    ofstream blockout;
    blockout.open ( file_name,  ios_base::app);
    blockout<< s;
    blockout.close();
}

bool static is_equal(EffectProxy e1, EffectProxy e2){
    if(e1.get_fact() != e2.get_fact())
        return false;
    vector<FactProxy> facts;

    for(int i=0; i <e1.get_conditions().size(); i++){
        facts.push_back(e1.get_conditions()[i]);
    }

    for(int j=0; j < e2.get_conditions().size(); j++){
        FactProxy f = e2.get_conditions()[j];
        bool found = false;
        for(int i=0; i<facts.size(); i++){
            if(f.get_pair() == facts[i].get_pair()){
                found = true;
                facts.erase(facts.begin()+i);
                break;
            }
        }
        if(!found) return false;
    }
    if(facts.empty()) return true;
    return false;
}

bool static is_equal(FactProxy factProxy1, FactProxy factProxy){
    return factProxy.get_pair() == factProxy1.get_pair();
}

bool static is_fact_of_same_var_diff_val(FactProxy f1, FactProxy f2){
    if(f1.get_variable().get_id() == f2.get_variable().get_id() && f1.get_value() != f2.get_value())
        return true;
    return false;
}

class testplan{


public:
    int n =15;
    int id;
    map<pair<int, int>, bool> op_orderings;
    map<pair<int, int>, bool> nontransitive_orderings;
    map<pair<int, int>, bool> nonconcurrency_constraints;
    testplan(){

    }
    testplan(testplan const &plan){
        n = plan.n;
        op_orderings = plan.op_orderings;
        nontransitive_orderings = plan.nontransitive_orderings;
        nonconcurrency_constraints = plan.nonconcurrency_constraints;
    }
    testplan(int n1, map<pair<int, int>, bool> op_orderings1, map<pair<int, int>, bool> nontransitive_orderings1,
    map<pair<int, int>, bool> nonconcurrency_constraints1){
        n= n1;
        op_orderings= op_orderings1;
        nonconcurrency_constraints = nonconcurrency_constraints1;
        nontransitive_orderings = nontransitive_orderings1;
    }

    void insert_ordering(int i, int j, bool b){
        pair<int,int> p(i,j);
        if(op_orderings.count(p) == 0){
            op_orderings.insert(pair<pair<int, int>, bool>(p, true));
        }
        op_orderings.at(p) = b;
    }
    void insert_nonconcurrency(int i, int j, bool b){
        pair<int,int> p(i,j);
        if(nonconcurrency_constraints.count(p) == 0){
            nonconcurrency_constraints.insert(pair<pair<int, int>, bool>(p, true));
        }
        nonconcurrency_constraints.at(p) = b;
    }
    void insert_nontransitive_orderings(int i, int j, bool b){
        pair<int,int> p(i,j);
        if(nontransitive_orderings.count(p) == 0){
            nontransitive_orderings.insert(pair<pair<int, int>, bool>(p, true));
        }
        nontransitive_orderings.at(p) = b;
    }
    bool has_ordering(int i, int j){
        pair<int,int> p(i,j);
        if(op_orderings.count(p) == 0)
            return false;
        return op_orderings.at(pair<int,int>(i,j));
    }
    bool has_nonconcurrency(int i, int j){
        pair<int,int> p(i,j);
        if(nonconcurrency_constraints.count(p) == 0)
            return false;
        return nonconcurrency_constraints.at(pair<int,int>(i,j));
    }

    bool is_nontransitive(int i, int j){
        pair<int,int> p(i,j);
        if(nontransitive_orderings.count(p) == 0)
            return false;
        return nontransitive_orderings.at(pair<int,int>(i,j));
    }
    void initiate(){
        int rand1, numofOrderings, numofnonconcurrency;
        for(int i=1; i<= n;i++){
            insert_ordering(0, i, true);
            insert_ordering(i, n+1, true);
        }

//I made a song. Mummy mummy mummy. mummy mummy love you.u u u u umma
    int n_comb = n*(n-1)/2;

        rand1 = rand();
        while (rand1 == 0){
            rand1 = rand();
        }
        numofOrderings = rand1 % n_comb;
        cout<< numofOrderings << endl;
        for(int i=0; i< numofOrderings;i++){
            int op_i = rand() % n+1;
            int op_j = rand() % n+1;
            if(op_i == op_j) continue;
            if(op_i < op_j)
                insert_ordering(op_i, op_j, true);
            else
              insert_ordering(op_j, op_i, true);
        }

        cout << "insert ordering done" << endl;
        find_transitive_ordering();
//        cout << "find transitive ordering done" << endl;
        rand1 = rand();
        while (rand1 == 0){
            rand1 = rand();
        }
//        cout << "rand: "<< rand1;
        if(numofOrderings == 0){
            numofnonconcurrency = 0;
        } else
            numofnonconcurrency = rand1 % numofOrderings;
        cout<< "non-con: " << numofnonconcurrency << endl;
        for(int i=0; i< numofnonconcurrency;i++){
            int op_i = rand() % n+1;
            int op_j = rand() % n+1;
            if(op_i == op_j || op_i == 0 || op_j == 0 || op_i == n+1 || op_j == n+1)
                continue;
            if(op_i > op_j){
                int temp = op_i;
                op_i = op_j;
                op_j = temp;
            }
            if(is_ordered(op_i, op_j)) continue;
            insert_nonconcurrency(op_i, op_j, true);
        }
        cout << "iteration done" << endl;

//        DPPL();

    }

    bool is_ordered(int i, int j){
        if(has_ordering(i, j)) return true;
        if(is_transitively_ordered(i, j)) return true;
        return false;
    }

    bool is_transitively_ordered(int i, int j){
        for (int k = 0; k < n+1 ; k++){
            if(has_ordering(i,k)){
                if(has_ordering(k,j)) return true;
                if (is_transitively_ordered(k,j))
                    return true;
            }
        }
        return false;
    }


    void find_transitive_ordering () {
        for(auto itr: nontransitive_orderings){
            nontransitive_orderings.at(itr.first) = false;
        }
        for (int i = 0; i <= n + 1; i++) {
            for (int j = 0; j <= n + 1; j++) {
                    if(has_ordering(i,j)){
                        if(is_transitively_ordered(i,j))
                            continue;
                        insert_nontransitive_orderings(i, j, true);
                    }
            }
        }
    }

    void print_nontransitive_orderings () {
        cout << "--------------------ID:"<< id << " Orderings -------------------" << endl;
        for (int i = 0; i <= n + 1; i++) {
            for (int j = 0; j <= n + 1; j++) {
                if (has_ordering(i, j)) {

                    if (is_nontransitive(i, j))
                        cout << i << " > " << j << endl;

                }
            }
        }
    }

    void print_orderings (){
        cout << "--------------------"<< " Orderings -------------------"<< endl;
        for (int i=0; i<= n+1; i++){
            for(int j=0;j<= n+1;j++){
                if(has_ordering(i, j)){
                    cout << i << " > " << j << " ";
                    if(is_nontransitive(i,j)) cout << "0\n";
                    else
                        cout<<"1\n";

                }
            }
        }
        cout << "--------------------"<< " Orderings -------------------"<<endl;
        cout << "--------------------"<< " Nonconcurrency Orderings -------------------"<< endl;
        for (int i=0; i<= n+1; i++){
            for(int j=0;j<= n+1;j++){
                if(has_nonconcurrency(i,j)){
                    cout << i << " > " << j << endl;
                }
            }
        }
        cout << "--------------------"<< " Nonconcurrency Orderings  -------------------"<<endl;
    }
    int DPPL(){
        vector<int>r(n+2,0);
        vector<int> queue;
        vector<bool> visited(n+2, false);
        queue.push_back(0);
        while (!queue.empty()){
            int a = queue.front();
            visited.at(a) = true;
            queue.erase(queue.begin());
            for(int i = 0; i<= n+1; i++){
                if(a == i) continue;
//                if(i > n+1) break;
                if(is_ordered(a, i)){
                    r.at(i) = max(r.at(i), r.at(a)+1);
                    if (is_nontransitive(a, i) && !visited.at(i) && std::find(queue.begin(), queue.end(),i) == queue.end()){
                        bool transitive = false;
                        for(int b: queue){
                            if(is_ordered(b, i)){
                                transitive = true;
                                break;
                            }
                        }
                        if(!transitive)
                            queue.push_back(i);
                    }

                }
            }

        }

//        for(int i=0; i<= level;i++){
//            cout << "level " << i << ": ";
//            for (auto j: l.at(i)) {
//                cout << j << " ";
//            }
//            cout<< endl;
//        }
        return r.at(n+1) - 1;
    }

    int find_minimum(){

        for (int i=0; i<= n+1; i++){
            for(int j=0;j<= n+1;j++){
                if (i == j) continue;
                if(has_nonconcurrency(i, j) & !is_ordered(i,j) & !is_ordered(j, i)){
                    testplan testplan1(n, op_orderings,
                                       nontransitive_orderings, nonconcurrency_constraints);
                    testplan testplan2(n, op_orderings,
                                       nontransitive_orderings, nonconcurrency_constraints);
                    testplan1.id = id+1;
                    testplan2.id = id+2;
                    testplan1.insert_ordering(i, j, true);
                    testplan2.insert_ordering(j, i, true);
                    testplan1.insert_nonconcurrency(i,j, false);
                    testplan2.insert_nonconcurrency(i,j, false);
                    testplan1.find_transitive_ordering();
//                    testplan1.print_nontransitive_orderings();
                    testplan2.find_transitive_ordering();
//                    testplan2.print_nontransitive_orderings();
                    return min(testplan1.find_minimum(), testplan2.find_minimum());
                }
            }
        }

//        print_nontransitive_orderings();
//        if(id == 6) {
//            cout << "ID: " << id << endl;
//        }
        int dppl = DPPL();

//        cout << "ID: " << id <<  " DPPL: " << dppl << endl;
        return  dppl;
    }

    float  calculate_flex() {
        vector<vector<int>> ordering(n+2 , vector<int>(n+2, 0));
        for (int i = 0 ; i <= n+1 ; i++) {
            for (int j = 0 ; j <= n+1 ; j++) {
                if (i == j) continue;
                if(is_ordered(i,j))
                    ordering[i][j] =1;

            }
        }
        int count_total = 0;
        int count_connected = 0;
        int nonconcurrent = 0;

        for (int i = 0; i < n ; i++) {
            for (int j = i+1; j <= n; j++) {
                count_total++;
                if (ordering[i][j] > 0 || ordering[j][i] > 0) {
                    count_connected++;
                } else if(has_nonconcurrency(i,j) || has_nonconcurrency(j,i))
                    nonconcurrent++;
            }
        }
        int unpaired = (count_total- count_connected);
        int concurrent = (unpaired - nonconcurrent);
//        cout << "total: " << count_total << "\tconnected: "<< count_connected << "\tunpaired: " << unpaired << endl;
//        cout << "nonconcurrent: "<< nonconcurrent << "\tconcurrent: " << concurrent << endl;
        float flex = (float)unpaired / (float)count_total;
        float cflex = (float)concurrent / (float)count_total;
        return cflex;
    }

};

class test{
    int n =10000;
public:
    void start(){
//        myID = 0;
        remove("cflex.csv");
        freopen ("log.txt","w",stdout);
        ofstream out;
        string result_file = "cflex.csv";
        out.open (result_file, ios::out | ios::app );
        out << "size, non-transitive-ordering, non-concurrency, duration, cflex\n";
        for(int i= 0; i<n ;i++){
            cout << i << endl;
//            cout << "________________________________________________________________________________________"<< endl;
            testplan plan;
            plan.initiate();
            plan.id = 0;
            plan.print_orderings();
            out << plan.n << "," << plan.nontransitive_orderings.size() << "," << plan.nonconcurrency_constraints.size()
            << "," << plan.find_minimum() << ","<< plan.calculate_flex() << endl;
//            cout << "minimum duration:" << plan.find_minimum() <<  " cflex:" << plan.calculate_flex() <<endl;

        }
        out.close();

    }
};



#endif //FAST_DOWNWARD_MYUTILS_H
