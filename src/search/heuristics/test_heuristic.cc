#include "test_heuristic.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

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

void TestHeuristic::setup_exploration_queue() {
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

void TestHeuristic::adjust_variable(int q) {
    if(q >= 0) {
        PropID p_id = q;
        Proposition *prop = get_proposition(p_id);
        if(prop->cost != prop->rhsq) {
            prop->del_bound = std::min(prop->cost,prop->rhsq); // here we save the value that tells us which queue entries to ignore
            queue.push(std::min(prop->cost,prop->rhsq),q);
        } else {
            prop->del_bound = -1;
        }
    } else {
        OpID op_id = make_op(q);
        UnaryOperator* un_op = get_operator(op_id);
        if(un_op->cost != un_op->rhsq) {
            un_op->del_bound = std::min(un_op->cost,un_op->rhsq); // here we save the value that tells us which queue entries to ignore
            queue.push(std::min(un_op->cost,un_op->rhsq),q); // starts at -1 - minus infinity
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
        sum += get_proposition(prop)->cost;
    }
    return sum;
}

bool TestHeuristic::prop_is_part_of_s(PropID prop) {
    for(PropID i : new_state) {
        if((int)prop == (int)i) {
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
        if((int) get_operator(a)->cost < (int) get_operator(min)->cost) {
            min = a;
        }
    }
    return min;
}

void TestHeuristic::solve_equations() {

    while(!queue.empty()) {
        int value = queue.top().first;
        int current = queue.top().second;
        if(value >= MAX_COST_VALUE) {
           /** utils::g_log <<"id                                  "<< current << endl;
            utils::g_log << "queue value                    "<< value << endl;
            if(get_operator(make_op(current))->num_preconditions != 0) {
                for(PropID prop : get_preconditions(make_op(current))) {
                    utils::g_log << "prec                    "<< prop << endl;
                    utils::g_log << "prec cost                  "<< get_proposition(prop)->cost << endl;
                }
            } */
        }
        if(current >= 0) {
            Proposition *prop = get_proposition((PropID)current);
            if(value == prop->del_bound) {
                if(prop->rhsq < prop->cost) {
                    queue.pop();
                    int old_cost = prop->cost;
                    prop->cost = prop->rhsq;
                    for (OpID op_id : precondition_of_pool.get_slice(
                    prop->precondition_of, prop->num_precondition_occurences)) {
                        UnaryOperator *op = get_operator(op_id);
                        if(op->rhsq >= MAX_COST_VALUE) {
                            op->rhsq = 1 + get_pre_condition_sum(op_id);
                        } else {
                            op->rhsq = op->rhsq - old_cost + prop->cost; 
                        }
                        adjust_variable(make_op(op_id));
                    }

                } else {
                    if(prop->rhsq == prop->cost) {
                        queue.pop();
                    }
                    prop->cost = MAX_COST_VALUE;
                    if(!prop_is_part_of_s(current)) {
                        OpID min_op = getMinOperator(prop);
                        prop->rhsq = 1 + get_operator(min_op)->cost;
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
            //utils::g_log << op->del_bound << endl;
            //utils::g_log << "adsfasd"<<op->rhsq << endl;
            //utils::g_log << op->cost << endl;
            if(value == op->del_bound) {
                if(op->rhsq < op->cost) {
                    queue.pop();
                    op->cost = op->rhsq;
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        prop->rhsq = std::min(prop->rhsq,(1+op->cost));
                        adjust_variable(add);
                    }
                } else {
                    if(op->rhsq == op->cost) {
                        queue.pop();
                    }
                    int x_old = op->cost;
                    op->cost = MAX_COST_VALUE;
                    op->rhsq = 1 + get_pre_condition_sum(make_op(current));
                    adjust_variable(current);
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        if(prop->rhsq == (1 + x_old)) {
                            OpID min_op = getMinOperator(get_proposition(add));
                            prop->rhsq = 1 + get_operator(min_op)->cost;
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
        setup_exploration_queue();
        new_state.clear();
        for(FactProxy fact : state) {
            PropID init_prop = get_prop_id(fact);
            Proposition *prop = get_proposition(init_prop);
            new_state.push_back((int)init_prop);
            prop->rhsq = 0;
            adjust_variable(init_prop);
        }
        first_time = false;
    } else {
        PropID prop;
        new_state.clear();
        for (FactProxy fact : state) {
            prop = get_prop_id(fact);
            new_state.push_back((int)prop);
        }

        vector<int> p_in_s;
        vector<int> p_in_s_strich;

        if(new_state.size() == 0 && old_state.size() == 0) {
            p_in_s = {};
            p_in_s_strich = {};
        } else if(new_state.size() == 0 && old_state.size() > 0) {
            p_in_s = {};
            p_in_s_strich = old_state;
        } else if(new_state.size() > 0 && old_state.size() == 0) {
            p_in_s = new_state;
            p_in_s_strich = {};
        } else if(new_state.size() < old_state.size()) {
            p_in_s = manage_state_comparison(old_state,new_state,1);
            p_in_s_strich = manage_state_comparison(old_state,new_state,0);
        } else {
            p_in_s = manage_state_comparison(new_state,old_state,1);
            p_in_s_strich = manage_state_comparison(new_state,old_state,0);
        }

        for(int i : p_in_s) {
            Proposition *prop = get_proposition((PropID)i);
            prop->rhsq = 0;
            adjust_variable(i);
        }

        for(int i : p_in_s_strich) {
            Proposition *prop = get_proposition((PropID)i);
            OpID min_op = getMinOperator(prop);
            prop->rhsq = 1 + get_operator(min_op)->cost;
            adjust_variable(i);
        }
    }
    solve_equations();

    int total_cost = 0;
    for (PropID goal_id : goal_propositions) {
        const Proposition *goal = get_proposition(goal_id);
        int goal_cost = goal->cost;
        //utils::g_log << goal_cost << endl;
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
