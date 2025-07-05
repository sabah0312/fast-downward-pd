#include <sstream>
#include "block.h"


Block :: Block(int id, OperatorID op)
    :id (id),
    op_index(op),
    is_temporary(false),
    parent(-1),
    type(REGULAR){
}

Block :: Block(int id, OperatorID op, vector<int> internals, block_type type, bool temp, int parent)
        :id (id),
         op_index(op),
         internal_blocks(internals),
         type(type),
         is_temporary(temp),
         parent(parent){
}


Block:: Block(int id, vector<int> blocks, bool temp)
    : id (id),
      internal_blocks(blocks),
      op_index(OperatorID(-1)),
      parent(-1),
      is_temporary(temp),
      type(REGULAR){
}

bool Block :: is_immediate_predecessor(int b_x){
    auto it = find(predecessors.begin(), predecessors.end(), b_x);
    if (it == predecessors.end())
        return false;
    return true;
}

bool Block :: is_immediate_successor(int b_x){
    auto it = find(successors.begin(), successors.end(), b_x);
    if (it == successors.end())
        return false;
    return true;
}

void Block :: delete_predecessor(int b_x){
    predecessors.erase(std::remove(predecessors.begin(), predecessors.end(), b_x), predecessors.end());
}

void Block :: delete_successor(int b_x){
    successors.erase(std::remove(successors.begin(), successors.end(), b_x), successors.end());
}

void Block :: add_predecessor(int b_x){
    auto it = find(predecessors.begin(), predecessors.end(), b_x);
    if (it == predecessors.end())
        predecessors.insert(predecessors.begin(), b_x);
}

void Block :: add_successor(int b_x){
    auto it = find(successors.begin(), successors.end(), b_x);
    if (it == successors.end())
        successors.insert(successors.begin(), b_x);
    //cout << "\t(S+" << b_x << ")" << id << " ";
}

void Block :: add_internal(int b_x){
    auto it = find(internal_blocks.begin(), internal_blocks.end(), b_x);
    if (it == internal_blocks.end())
        internal_blocks.insert(it, b_x);
}

void Block :: delete_internal(int b_x){
    internal_blocks.erase(std::remove(internal_blocks.begin(), internal_blocks.end(), b_x), internal_blocks.end());
}


bool Block :: is_primary (){
    return internal_blocks.empty() && op_index.get_index() > -1;
}

//void Block:: remove_precondition(FactProxy factProxy){
//    for(int i = 0; i < preconditions.size(); i ++){
//        if (factProxy.get_pair() == preconditions[i].get_pair()){
//            preconditions.erase(preconditions.begin() + i);
//            break;
//        }
//    }
//}

string Block :: name(){
    ostringstream s;
    s << "b";
    if (is_primary()) s << "<" << id << ">";
    else {
        s <<"<";
        for (int i : internal_blocks){
            s << i;
            if(i != internal_blocks.size() - 1) s << ":";
        }
        s << ">";
    }
    return s.str();
}

string Block:: getTypeName (){
    if (is_internal()) return "Internal";
    else if (is_outer()) return  "Outer";
    else if(type == TEMP_REMOVED) return "Temporary removed";
    else if (type == TEMP_INTERNAL) return  "Temporary Internal";
}


/**
 * @brief return the preconditions of a block as a vector of FactProxy
 */
vector<FactProxy> Block :: get_preconditions (){
    return preconditions;
}

bool Block :: is_internal(){
    return parent >= 0;
}

bool Block :: is_outer(){
    return parent == -1;
}

void Block:: set_outer (){
    parent = -1;
}
void Block:: set_type_normal (){
    type = REGULAR;
}

bool Block :: is_temp_removed (){
    return type == TEMP_REMOVED;
}

bool Block :: is_temp_internal (){
    return type == TEMP_INTERNAL;
}


//void Block :: set_temp_removed (){
//     type = TEMP_REMOVED;
//};

string Block :: print(){
    string s = "ID: " + to_string(id) + ", type: "  + getTypeName();
    if (is_primary()){
        s+= ", primary, op_index=" + to_string(op_index.get_index());
    }
    if(is_internal() || type == TEMP_INTERNAL){
        s+=   ", parent: " + to_string( parent);
    }
    s+=" ," + to_string(internal_blocks.size()) + " internals :";
    for(int id: internal_blocks)
        s += to_string(id) + " ";
    return s;
}

string Block ::  print_predecessors_and_successors(){
    string s;
    s = "block: " + to_string(id) + ", predecessors: ";
    for(int id: predecessors)
        s+= to_string(id) + " ";
    s+= ", successors: ";
    for(int id: successors)
        s+= to_string(id) + " ";
    s += "\n";
    return s;
}

string Block :: print_preconditions_and_effects(){
    string s = "";
    s += "\tPreconditions : ";

    for (FactProxy factProxy1:  preconditions){
        s+= to_string(factProxy1.get_variable().get_id()) + ":" + to_string(factProxy1.get_value()) + " " + factProxy1.get_name() +" ";
    }

    s+= "\n";
    s+= "\tEffects : \n";
    for (EffectProxy effectProxy: effects ){
        s+= "\t\t" + to_string(effectProxy.get_fact().get_variable().get_id()) + ":"
             + to_string(effectProxy.get_fact().get_value()) + " " + effectProxy.get_fact().get_name() + " ";
        if(!effectProxy.get_conditions().empty()){
            s+= "conditions: ";
            for(FactProxy factProxy: effectProxy.get_conditions()){
                s += to_string(factProxy.get_variable().get_id()) + ":" + to_string(factProxy.get_value())
                + " " + factProxy.get_name() + ", ";
            }
        }
        s += "\n";
    }
    s += "\n";
    return s;
}


/**
 * @brief return the effects of a block as a vector of EffectProxy
 */
vector<EffectProxy> Block :: get_effects (){
    return effects;
}

/**
 * @brief return the effects those has factproxy
 */
vector<EffectProxy> Block :: get_effects (FactProxy factProxy){
    vector<EffectProxy> related_effects;
    for(EffectProxy effectProxy: effects){
        if(effectProxy.get_fact().get_pair() == factProxy.get_pair())
            related_effects.push_back(effectProxy);
    }
    return related_effects;
}


bool Block :: has_precondition(FactProxy factProxy){
    for (FactProxy f : preconditions){
        if (f.get_pair() == factProxy.get_pair()) return true;
    }
    return false;
}



bool Block :: has_precondition_of_different_value(FactProxy factProxy){
    for (FactProxy f : preconditions){
        if (f.get_variable() == factProxy.get_variable() && f.get_value() != factProxy.get_value())
            return true;
    }
    return false;
}

bool Block:: is_deleter( FactProxy factProxy) {
    for (FactProxy f : preconditions){
        if (f.get_variable() == factProxy.get_variable() && f.get_value() != factProxy.get_value())
            return false;
    }

    for (EffectProxy e: effects){
        FactProxy f = e.get_fact();
        if (f.get_variable() == factProxy.get_variable()){
            if(f.get_value() != factProxy.get_value()) return true;
        }
    }
    return false;
}


/**
 * @brief checks whether the block of index b_x is a producer of a fact f
 * @param b_x index of the block
 * @param factProxy the fact
 * @return true if the block is  a producer, false otherwise
 */
bool Block :: is_producer(FactProxy factProxy){
    if(is_consumer(factProxy)) return false;
    vector<FactProxy> facts;
    for (EffectProxy e : effects){
        if(e.get_fact().get_variable() == factProxy.get_variable()){
            facts.push_back(e.get_fact());
        }
//        cout << f.get_fact().get_variable().get_id() << " " << f.get_fact().get_value() << endl;
//        if (f.get_fact().get_pair() == factProxy.get_pair()) return true;
    }
    bool found = false;
    for(FactProxy f: facts){
        if(f.get_pair() == factProxy.get_pair())
            found = true;
        else
            return false;
    }
    if(found) return true;
    return false;
}


/**
 * @brief checks whether the block of index b_x is a consumer of a fact f
 * @param b_x index of the block
 * @param factProxy the fact
 * @return true if the block is  a consumer, false otherwise
 */
bool Block :: is_consumer(FactProxy factProxy){
    for (FactProxy f : preconditions){
        if (f.get_pair() == factProxy.get_pair()) return true;
    }
    return false;
}

