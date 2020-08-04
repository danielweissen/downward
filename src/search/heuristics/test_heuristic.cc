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
    queue.clear();
    p_and_o.clear();
    struct xq temp;
    for (Proposition &prop : propositions) {
        prop.cost = MAX_COST_VALUE; // == infinity
        temp = {(int)get_prop_id(prop),MAX_COST_VALUE,1};
        p_and_o.push_back(temp);
    }
    for (UnaryOperator &op : unary_operators) {
        if((int)get_preconditions_vector(get_op_id(op)).size() == 0) {
            temp = {(int)get_op_id(op),1,0};
            op.cost = 1;
        } else {
            temp = {(int)get_op_id(op),MAX_COST_VALUE,0};
            op.cost = MAX_COST_VALUE;
        }
        p_and_o.push_back(temp);
    }
}

void TestHeuristic::adjust_variable(struct xq &q) {
    if(q.type == 1) {
        PropID p_id = q.id;
        Proposition *prop = get_proposition(p_id);
        queue.remove(q); // if q is not in queue just returns false
        if(prop->cost != q.rhsq) {
            queue.push(std::min(prop->cost,q.rhsq),q);
        }
    } else {
        OpID op_id = q.id;
        UnaryOperator* un_op = get_operator(op_id);
        queue.remove(q); // if q is not in queue just returns false
        if(un_op->cost != q.rhsq) {
            queue.push(std::min(un_op->cost,q.rhsq),q);
        }
    }
}

int TestHeuristic::get_pre_condition_sum(struct xq &q) {
    OpID o_id = q.id;
    int sum = 0;
    for(PropID prop : get_preconditions(o_id)) {
        sum += get_proposition(prop)->cost;
    }
    return sum;
}

bool TestHeuristic::xq_is_part_of_s(struct xq &q,const State &state) {
    for (FactProxy fact : state) {
        PropID init_prop = get_prop_id(fact);
        if(q.id == (int)init_prop) {
            return true;
        }
    }
    return false;
}

bool TestHeuristic::prop_is_part_of_s(PropID prop, const State &state) {
    for (FactProxy fact : state) {
        PropID init_prop = get_prop_id(fact);
        if((int)prop == (int)init_prop) {
            return true;
        }
    }
    return false;
}

OpID TestHeuristic::getMinOperator(Proposition * prop) {
    OpID min = prop->add_effects.front();
    for(OpID a : prop->add_effects) {
        if((int) get_operator(a)->cost < (int) get_operator(min)->cost) {
            min = a;
        }
    }
    return min;
}

void TestHeuristic::solve_equations(const State &state) {

    while(!queue.empty()) {
        struct xq current = queue.top().second;
        if(current.type == 1) {
            Proposition *prop = get_proposition((PropID)current.id);
            if(current.rhsq < prop->cost) {
                queue.remove(current);
                int old_cost = prop->cost;
                prop->cost = current.rhsq;
                for (OpID op_id : precondition_of_pool.get_slice(
                 prop->precondition_of, prop->num_precondition_occurences)) {
                     struct xq bla = {(int)op_id,MAX_COST_VALUE,0};
                     auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
                     if(cur->rhsq >= MAX_COST_VALUE) {
                        cur->rhsq = 1 + get_pre_condition_sum(*cur);
                     } else {
                        cur->rhsq = cur->rhsq - old_cost + prop->cost; 
                     }
                     adjust_variable(*cur);
                 }

            } else {
                prop->cost = MAX_COST_VALUE;
                if(!prop_is_part_of_s(current.id,state)) {
                    OpID min_op = getMinOperator(prop);
                    current.rhsq = 1 + get_operator(min_op)->cost;
                    adjust_variable(current);
                }
                for (OpID op_id : precondition_of_pool.get_slice(
                 prop->precondition_of, prop->num_precondition_occurences)) {
                     struct xq bla = {(int)op_id,MAX_COST_VALUE,0};
                     auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
                     cur->rhsq = MAX_COST_VALUE;
                     adjust_variable(*cur);
                 }
            }
        } else {
            UnaryOperator *op = get_operator((OpID)current.id);
            if(current.rhsq < op->cost) {
                queue.remove(current);
                op->cost = current.rhsq;
                PropID add = op->effect;
                if(!prop_is_part_of_s(add,state)) {
                    struct xq bla = {(int)add,MAX_COST_VALUE,1};
                    auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
                    cur->rhsq = std::min(cur->rhsq,(1+op->cost));
                    adjust_variable(*cur);
                }
            } else {
                int x_old = op->cost;
                op->cost = MAX_COST_VALUE;
                current.rhsq = 1 + get_pre_condition_sum(current);
                adjust_variable(current);
                PropID add = op->effect;
                if(!prop_is_part_of_s(add,state)) {
                    struct xq bla = {(int)add,MAX_COST_VALUE,1};
                    auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
                    if(cur->rhsq == (1 + x_old)) {
                        OpID min_op = getMinOperator(get_proposition(add));
                        cur->rhsq = 1 + get_operator(min_op)->cost;
                        adjust_variable(*cur);
                    }
                }
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
        for(FactProxy fact : state) {
            PropID init_prop = get_prop_id(fact);
            struct xq bla = {(int)init_prop,MAX_COST_VALUE,1};
            auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
            cur->rhsq = 0;
            adjust_variable(*cur);
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
            struct xq q = {i,MAX_COST_VALUE,1};
            auto cur = find(p_and_o.begin(), p_and_o.end(), q);
            cur->rhsq = 0;
            adjust_variable(*cur);
        }

        for(int i : p_in_s_strich) {
            struct xq q = {i,MAX_COST_VALUE,1};
            auto cur = find(p_and_o.begin(), p_and_o.end(), q);
            OpID min_op = getMinOperator(get_proposition((PropID)cur->id));
            cur->rhsq = 1 + get_operator(min_op)->cost;
            adjust_variable(*cur);
        }
    }

    solve_equations(state);

    int total_cost = 0;
    for (PropID goal_id : goal_propositions) {
        const Proposition *goal = get_proposition(goal_id);
        int goal_cost = goal->cost;
        if (goal_cost >= MAX_COST_VALUE)
            return DEAD_END;
        total_cost+=goal_cost;
    }
    

    old_state.clear();
    for(FactProxy v : state) {
        old_state.push_back(get_prop_id(v));
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
