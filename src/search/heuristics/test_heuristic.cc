#include "test_heuristic.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include <typeinfo>

#include <cassert>
#include <vector>

using namespace std;

namespace test_heuristic { 



const int TestHeuristic::MAX_COST_VALUE;

// construction and destruction
TestHeuristic::TestHeuristic(const Options &opts)
    : RelaxationHeuristic(opts),
      did_write_overflow_warning(false) {
    utils::g_log << "Initializing test heuristic..." << endl;
}

void TestHeuristic::setup_exploration_queue_2() {
    queue_propositions.clear();
    queue_operators.clear();
    num_in_queue = 0;
    for (Proposition &prop : propositions) {
        prop.cost = MAX_COST_VALUE; // == infinity
        prop.rhsq = MAX_COST_VALUE;
        prop.del_bound = MAX_COST_VALUE;
        queue_propositions.push_back(-1);
    }
    for (UnaryOperator &op : unary_operators) {
        if((int)get_preconditions_vector(get_op_id(op)).size() == 0) {
            op.cost = 1;
            op.rhsq = 1;
            op.del_bound = 1;
        } else {
            op.cost = MAX_COST_VALUE;
            op.rhsq = MAX_COST_VALUE;
            op.del_bound = MAX_COST_VALUE;
        }
        queue_operators.push_back(-1);
    }
}

void TestHeuristic::setup_exploration_queue() {
    queue.clear();
    for (Proposition &prop : propositions) {
        prop.cost = MAX_COST_VALUE; // == infinity
        prop.rhsq = MAX_COST_VALUE;
        prop.del_bound = MAX_COST_VALUE;
    }
    for (UnaryOperator &op : unary_operators) {
        if((int)get_preconditions_vector(get_op_id(op)).size() == 0) {
            op.cost = 1;
            op.rhsq = 1;
            op.del_bound = 1;
        } else {
            op.cost = MAX_COST_VALUE;
            op.rhsq = MAX_COST_VALUE;
            op.del_bound = MAX_COST_VALUE;
        }
    }
}

void TestHeuristic::adjust_variable_2(int q, int type) {
    if(type == 1) {
        Proposition *prop = get_proposition(q);
        if(prop->cost != prop->rhsq) {
            int min = std::min(prop->cost,prop->rhsq);
            if(queue_propositions[q] == -1) {
                queue_propositions[q] = min;
                num_in_queue++;
            } else {
                queue_propositions[q] = min;
            }
        } else {
            if(queue_propositions[q] != -1) {
                queue_propositions[q] = -1;
                num_in_queue--;
            }
        }
    } else {
        UnaryOperator* un_op = get_operator(q);
        if(un_op->cost != un_op->rhsq) {
            int min = std::min(un_op->cost,un_op->rhsq);
            if(queue_operators[q] == -1) {
                queue_operators[q] = min;
                num_in_queue++;
            } else {
                queue_operators[q] = min;
            }
        } else {
            if(queue_operators[q] != -1) {
                queue_operators[q] = -1;
                num_in_queue--;
            }
        }
    }
}

void TestHeuristic::adjust_variable(int q) {
    if(q >= 0) { // i differentiate propositions and operators by inserting positive numbers into the queue for propositions and negative numbers for operators
        PropID p_id = q;
        Proposition *prop = get_proposition(p_id);
        if(prop->cost != prop->rhsq) {
            int min = std::min(prop->cost,prop->rhsq);
            prop->del_bound = min; // here we save the value that tells us which queue entries to ignore
            queue.push(min,q);
        } else {
            prop->del_bound = -1;
        }
    } else {
        OpID op_id = make_op(q);
        UnaryOperator* un_op = get_operator(op_id);
        if(un_op->cost != un_op->rhsq) {
            int min = std::min(un_op->cost,un_op->rhsq);
            un_op->del_bound = min; // here we save the value that tells us which queue entries to ignore
            queue.push(min,q);
        } else {
            un_op->del_bound = -1;
        }
    }
}

int TestHeuristic::make_op(int q) {
    q++;
    return (-q);
}

int TestHeuristic::get_pre_condition_sum(OpID id) {
    int sum = 0;
    for(PropID prop : get_preconditions(id)) {
        if(sum >= MAX_COST_VALUE) {
            return MAX_COST_VALUE;
        }
        sum += get_proposition(prop)->cost;
    }
    return sum;
}

bool TestHeuristic::prop_is_part_of_s(PropID prop) {
    for(PropID i : new_state) {
        if(prop == i) {
            return true;
        }
    }
    return false;
}

OpID TestHeuristic::getMinOperator(Proposition * prop) {
    if(prop->add_effects.empty()) {
        return 0;
    }
    OpID min = prop->add_effects.front();
    for(OpID a : prop->add_effects) {
        if(get_operator(a)->cost < get_operator(min)->cost) {
            min = a;
        }
    }
    return min;
}

int TestHeuristic::make_inf(int a) {
    if(a > MAX_COST_VALUE) {
        return MAX_COST_VALUE;
    }
    return a;
}

void TestHeuristic::solve_equations_2() {

    while(num_in_queue != 0) {
        std::pair<int,int> top = get_min();
        int index = top.first;
        int type = top.second;
        if(type == 1) {
            Proposition *prop = get_proposition(index);
                if(prop->rhsq < prop->cost) {
                    queue_propositions[index] = -1;
                    num_in_queue--;
                    int old_cost = prop->cost;
                    prop->cost = prop->rhsq;
                    for (OpID op_id : precondition_of_pool.get_slice(
                    prop->precondition_of, prop->num_precondition_occurences)) {
                        UnaryOperator *op = get_operator(op_id);
                        if(op->rhsq >= MAX_COST_VALUE) {
                            op->rhsq = make_inf(1 + get_pre_condition_sum(op_id));
                        } else {
                            op->rhsq = make_inf(op->rhsq - old_cost + prop->cost);
                        }
                        adjust_variable_2(op_id,0);
                    }

                } else {
                    prop->cost = MAX_COST_VALUE;
                    if(!prop_is_part_of_s(index)) {
                        OpID min_op = getMinOperator(prop);
                        prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
                        adjust_variable_2(index,type);
                    }
                    for (OpID op_id : precondition_of_pool.get_slice(
                    prop->precondition_of, prop->num_precondition_occurences)) {
                        UnaryOperator *op = get_operator(op_id);
                        op->rhsq = MAX_COST_VALUE;
                        adjust_variable_2(op_id,0);
                    }
                }
        } else {
            UnaryOperator *op = get_operator(index);
                if(op->rhsq < op->cost) {
                    queue_operators[index] = -1;
                    num_in_queue--;
                    op->cost = op->rhsq;
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        prop->rhsq = make_inf(std::min(prop->rhsq,(1+op->cost)));
                        adjust_variable_2(add,1);
                    }
                } else {
                    int x_old = op->cost;
                    op->cost = MAX_COST_VALUE;
                    op->rhsq = make_inf(1 + get_pre_condition_sum(index));
                    adjust_variable_2(index,type);
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        if(prop->rhsq == (1 + x_old)) {
                            OpID min_op = getMinOperator(get_proposition(add));
                            prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
                            adjust_variable_2(add,1);
                        }
                    }
                }
        }
    }
}

std::pair<int,int> TestHeuristic::get_min() {
    int index = -1;
    int min = MAX_COST_VALUE;
    int type = 2;

    for(std::vector<int>::size_type i = 0; i != queue_propositions.size(); i++) {
        if(queue_propositions[i] == -1) {
            continue;
        }
        if(queue_propositions[i]<=min) {
            index = i;
            min = queue_propositions[i];
            type = 1;
        }
    }

    for(std::vector<int>::size_type i = 0; i != queue_operators.size(); i++) {
        if(queue_operators[i] == -1) {
            continue;
        }
        if(queue_operators[i]<=min) {
            index = i;
            min = queue_operators[i];
            type = 0;
        }
    }
    return std::make_pair(index,type);
}

void TestHeuristic::solve_equations() {

    while(!queue.empty()) {
        int value = queue.top().first;
        int current = queue.top().second;
        if(current >= 0) {
            Proposition *prop = get_proposition((PropID)current);
            if(value == prop->del_bound) {
                prop->del_bound = -1;
                if(prop->rhsq < prop->cost) {
                    queue.pop();
                    int old_cost = prop->cost;
                    prop->cost = prop->rhsq;
                    for (OpID op_id : precondition_of_pool.get_slice(
                    prop->precondition_of, prop->num_precondition_occurences)) {
                        UnaryOperator *op = get_operator(op_id);
                        if(op->rhsq >= MAX_COST_VALUE) {
                            op->rhsq = make_inf(1 + get_pre_condition_sum(op_id));
                        } else {
                            op->rhsq = make_inf(op->rhsq - old_cost + prop->cost);
                        }
                        adjust_variable(make_op(op_id));
                    }

                } else {
                    prop->cost = MAX_COST_VALUE;
                    if(!prop_is_part_of_s(current)) {
                        OpID min_op = getMinOperator(prop);
                        prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
                        adjust_variable(current);
                    }
                    for (OpID op_id : precondition_of_pool.get_slice(
                    prop->precondition_of, prop->num_precondition_occurences)) {
                        UnaryOperator *op = get_operator(op_id);
                        op->rhsq = MAX_COST_VALUE;
                        adjust_variable(make_op(op_id));
                    }
                }
            } else {
                queue.pop();
                continue;
            }
        } else {
            UnaryOperator *op = get_operator(make_op(current));
            if(value == op->del_bound) {
                op->del_bound = -1;
                if(op->rhsq < op->cost) {
                    queue.pop();
                    op->cost = op->rhsq;
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        prop->rhsq = make_inf(std::min(prop->rhsq,(1+op->cost)));
                        adjust_variable(add);
                    }
                } else {
                    int x_old = op->cost;
                    op->cost = MAX_COST_VALUE;
                    op->rhsq = make_inf(1 + get_pre_condition_sum(make_op(current)));
                    adjust_variable(current);
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        if(prop->rhsq == (1 + x_old)) {
                            OpID min_op = getMinOperator(get_proposition(add));
                            prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
                            adjust_variable(add);
                        }
                    }
                }
            } else {
                queue.pop();
                continue;
            }
        }
    }
}

vector<int> TestHeuristic::manage_state_comparison(vector<int> & bigger, vector<int> & smaller, int which) {

    vector<int> c = {};
        sort(bigger.begin(),bigger.end());
        sort(smaller.begin(),smaller.end());

        int counta = 0;
        int countb = 0;

        bool done_a = false;
        bool done_b = false;


        while(!done_a || !done_b) {
                if(bigger[counta]<smaller[countb]) {
                    if(counta < (int)bigger.size()-1) {
                        if(which == 1)
                        c.push_back(bigger[counta]);
                        counta++;
                    } else if(counta < (int)bigger.size() && !done_a) {
                        if(which == 1)
                        c.push_back(bigger[counta]);
                        done_a = true;
                    } else if(countb < (int)smaller.size()-1){
                        if(which == 0)
                        c.push_back(smaller[countb]);
                        countb++;
                    } else if(countb < (int)smaller.size()){
                        if(which == 0)
                        c.push_back(smaller[countb]);
                        done_b = true;
                    }
                } else if(bigger[counta] > smaller[countb]) {
                    if(countb < (int)smaller.size()-1){
                        if(which == 0)
                        c.push_back(smaller[countb]);
                        countb++;
                    } else if(countb < (int)smaller.size() && !done_b){
                        if(which == 0)
                        c.push_back(smaller[countb]);
                        done_b = true;
                    } else if(counta < (int)bigger.size()-1) {
                        if(which == 1)
                        c.push_back(bigger[counta]);
                        counta++;
                    } else if(counta < (int)bigger.size()) {
                        if(which == 1)
                        c.push_back(bigger[counta]);
                        done_a = true;
                    }
                } else {
                    if(counta < (int)bigger.size()-1) {
                        counta++;
                    } else if(counta < (int)bigger.size() && !done_a) {
                        done_a = true;
                    }
                    if(countb < (int)smaller.size()-1) {
                        countb++;
                    } else if(countb < (int)smaller.size() && !done_b) {
                        done_b = true;
                    }
                }

        }
        return c;
}


int TestHeuristic::compute_heuristic(const State &state) {
    if(first_time) {
        setup_exploration_queue_2();
        new_state.clear();
        for(FactProxy fact : state) {
            PropID init_prop = get_prop_id(fact);
            Proposition *prop = get_proposition(init_prop);
            new_state.push_back(init_prop);
            prop->rhsq = 0;
            adjust_variable_2(init_prop,1);
        }
        first_time = false;
    } else {
        new_state.clear();
        for (FactProxy fact : state) {
            new_state.push_back(get_prop_id(fact));
        }

        vector<int> p_in_s;
        vector<int> p_in_s_strich;
        
        p_in_s = manage_state_comparison(new_state,old_state,1);
        p_in_s_strich = manage_state_comparison(new_state,old_state,0);

        num_in_queue = 0;

        for(int i : p_in_s) {
            Proposition *prop = get_proposition(i);
            prop->rhsq = 0;
            adjust_variable_2(i,1);
        }

        for(int i : p_in_s_strich) {
            Proposition *prop = get_proposition(i);
            OpID min_op = getMinOperator(prop);
            prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
            adjust_variable_2(i,1);
        }
    }

    solve_equations_2();

    int total_cost = 0;
    for (PropID goal_id : goal_propositions) {
        const Proposition *goal = get_proposition(goal_id);
        int goal_cost = goal->cost;
        if (goal_cost >= MAX_COST_VALUE)
            return DEAD_END;
        total_cost+=goal_cost;
    }

    old_state.clear();
    for(int v : new_state) {
        old_state.push_back(v);
    }
    return (total_cost/2);
}

/**int TestHeuristic::compute_heuristic(const State &state) {
    if(first_time) {
        setup_exploration_queue();
        new_state.clear();
        for(FactProxy fact : state) {
            PropID init_prop = get_prop_id(fact);
            Proposition *prop = get_proposition(init_prop);
            new_state.push_back(init_prop);
            prop->rhsq = 0;
            adjust_variable(init_prop);
        }
        first_time = false;
    } else {
        //queue.clear();

        new_state.clear();
        for (FactProxy fact : state) {
            new_state.push_back(get_prop_id(fact));
        }

        vector<int> p_in_s;
        vector<int> p_in_s_strich;
        
        p_in_s = manage_state_comparison(new_state,old_state,1);
        p_in_s_strich = manage_state_comparison(new_state,old_state,0);

        for(int i : p_in_s) {
            Proposition *prop = get_proposition(i);
            prop->rhsq = 0;
            adjust_variable(i);
        }

        for(int i : p_in_s_strich) {
            Proposition *prop = get_proposition(i);
            OpID min_op = getMinOperator(prop);
            prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
            adjust_variable(i);
        }
    }

    solve_equations();

    int total_cost = 0;
    for (PropID goal_id : goal_propositions) {
        const Proposition *goal = get_proposition(goal_id);
        int goal_cost = goal->cost;
        if (goal_cost >= MAX_COST_VALUE)
            return DEAD_END;
        total_cost+=goal_cost;
    }

    old_state.clear();
    for(int v : new_state) {
        old_state.push_back(v);
    }

    return (total_cost/2);
}
 */


int TestHeuristic::compute_heuristic(const GlobalState &global_state) {
    return compute_heuristic(convert_global_state(global_state));
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis("Additive heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support(
        "axioms",
        "supported (in the sense that the planner won't complain -- "
        "handling of axioms might be very stupid "
        "and even render the heuristic unsafe)");
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "yes for tasks without axioms");
    parser.document_property("preferred operators", "yes");

    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return nullptr;
    else
        return make_shared<TestHeuristic>(opts);
}

static Plugin<Evaluator> _plugin("test", _parse);


}
