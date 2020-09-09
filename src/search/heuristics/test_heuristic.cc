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
      last_state(task_proxy.get_initial_state()) {
    utils::g_log << "Initializing test heuristic..." << endl;
}

void TestHeuristic::setup_exploration_queue() {
    // for each q ∈ P ∪ O do set rhsq := xq := ∞
    for(Proposition &prop : propositions) {
        prop.cost = MAX_COST_VALUE;
        prop.rhsq = MAX_COST_VALUE;
        prop.val_in_queue = -1;
    }
    for(UnaryOperator &op : unary_operators) {
        op.cost = MAX_COST_VALUE;
        op.rhsq = MAX_COST_VALUE;
        op.val_in_queue = -1;
    }

    // for each o ∈ O with Prec(o) = ∅ do set rhso := xo := 1
    for(UnaryOperator &op : unary_operators) {
        if(get_preconditions_vector(get_op_id(op)).empty()) {
            op.cost = 1;
            op.rhsq = 1;
            op.val_in_queue = 1;
            //this isnt part of the pseudo code
            queue.push(1,make_op(get_op_id(op)));
            ++num_in_queue;
        }
    }    
}

void TestHeuristic::adjust_proposition(Proposition *prop) {
    if(prop->cost != prop->rhsq) {
        if(prop->val_in_queue == -1) {
            ++num_in_queue;
        }
        int min = std::min(prop->cost, prop->rhsq);
        prop->val_in_queue = min;
        queue.push(min,get_prop_id(*prop));
    } else if(prop->val_in_queue != -1) {
        prop->val_in_queue = -1;
        --num_in_queue;
    }
}

void TestHeuristic::adjust_operator(UnaryOperator  *un_op) {
    if(un_op->cost != un_op->rhsq) {
        if(un_op->val_in_queue == -1) {
            ++num_in_queue;
        }
        int min = std::min(un_op->cost, un_op->rhsq);
        un_op->val_in_queue = min;
        queue.push(min,make_op(get_op_id(*un_op)));
    } else if(un_op->val_in_queue != -1) {
        un_op->val_in_queue = -1;
        --num_in_queue;
    }
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
    return current_state[prop];
}
 
int TestHeuristic::make_op(OpID op) {
    op++;
    return -op;
}

int TestHeuristic::get_min_operator_cost(Proposition *prop) {
    if(prop->add_effects.empty()) {
        return MAX_COST_VALUE;
    }

    OpID min = prop->add_effects.front();
    for(OpID a : prop->add_effects) {
        if(get_operator(a)->cost < get_operator(min)->cost) {
            min = a;
        }
    }
    return get_operator(min)->cost;
}

int TestHeuristic::make_inf(int a) {
    return std::min(a, MAX_COST_VALUE);
}

void TestHeuristic::solve_equations() {
    // while the priority queue is not empty do
    while(num_in_queue > 0) {
        // assign the element with the smallest priority in the priority queue to q
        int queue_val = queue.top().first;
        int index = queue.top().second;
        if (index < 0) {
            index = make_op(index);
            UnaryOperator *op = get_operator(index);
            if(op->val_in_queue == queue_val) {
                if(op->rhsq < op->cost) {
                    queue.pop();
                    op->val_in_queue = -1;
                    num_in_queue--;
                    op->cost = op->rhsq;
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        prop->rhsq = make_inf(std::min(prop->rhsq,(1+op->cost)));
                        adjust_proposition(prop);
                    }
                } else {
                    int x_old = op->cost;
                    op->cost = MAX_COST_VALUE;
                    op->rhsq = make_inf(1 + get_pre_condition_sum(index));
                    adjust_operator(op);
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        if(prop->rhsq == (1 + x_old)) {
                            prop->rhsq = make_inf(1 + get_min_operator_cost(prop));
                            adjust_proposition(prop);
                        }
                    }
                }
            } else {
                queue.pop();
                continue;
            }
        } else {
            //if q ∈ P then 
            Proposition *prop = get_proposition(index);
            if(prop->val_in_queue == queue_val) {
                // if rhsq < xq then
                if (prop->rhsq < prop->cost) {
                    // delete q from the priority queue
                    queue.pop();
                    prop->val_in_queue = -1;
                    num_in_queue--;
                    // set xold := xq
                    int old_cost = prop->cost;
                    // set xq := rhsq
                    prop->cost = prop->rhsq;
                    // for each o ∈ O such that q ∈ Prec(o) do
                    for (OpID op_id : precondition_of_pool.get_slice(
                        prop->precondition_of, prop->num_precondition_occurences)) {
                        UnaryOperator* op = get_operator(op_id);
                        // if rhso = ∞ then
                        if (op->rhsq >= MAX_COST_VALUE) {
                            // set rhso := 1 + SUM p∈Prec(o) xp
                            op->rhsq = make_inf(1 + get_pre_condition_sum(op_id));
                        } else {
                            // else set rhso := rhso − xold + xq
                            op->rhsq = make_inf(op->rhsq - old_cost + prop->cost);
                        }
                        // AdjustVariable(o)
                        adjust_operator(op);
                    }

                } else {
                    // set xq := ∞
                    prop->cost = MAX_COST_VALUE;
                    // if q /∈ s then
                    if (!prop_is_part_of_s(index)) {
                        // set rhsq = 1 + mino∈O|q∈Add(o) xo
                        prop->rhsq = make_inf(1 + get_min_operator_cost(prop));
                        // AdjustVariable(q)
                        adjust_proposition(prop);
                    }
                    // for each o ∈ O such that q ∈ Prec(o) do
                    for (OpID op_id : precondition_of_pool.get_slice(
                        prop->precondition_of, prop->num_precondition_occurences)) {
                        // set rhso := ∞
                        UnaryOperator* op = get_operator(op_id);
                        op->rhsq = MAX_COST_VALUE;
                        // AdjustVariable(o)
                        adjust_operator(op);
                    }
                }
            } else {
                queue.pop();
                continue;
            }
        }
    }
}

int TestHeuristic::compute_heuristic(const State &state) {
    if(first_time) {
        //empty the priority queue
        num_in_queue = 0;
        current_state = vector<bool>(propositions.size(), false);
        queue.clear();
        //for each q ∈ P ∪ O do set rhsq := xq := ∞
        //for each o ∈ O with P rec(o) = ∅ do set rhso := xo := 1
        setup_exploration_queue();
        assert(state == task_proxy.get_initial_state());
        // set s to the state whose heuristic value needs to get computed
        //for each p ∈ s do rhsp := 0 AdjustVariable(p)
        for(FactProxy fact : state) { 
            PropID prop_id = get_prop_id(fact);
            current_state[prop_id] = true;
            Proposition *prop = get_proposition(prop_id);
            prop->rhsq = 0;
            adjust_proposition(prop);
        }
        first_time = false;
    } else {
        // set s to the state whose heuristic value needs to get computed next
        num_in_queue = 0;
        current_state = vector<bool>(propositions.size(), false);
        queue.clear();
        //for(FactProxy fact : state) {
        //    PropID prop_id = get_prop_id(fact);
           // current_state[prop_id] = true;
        //}
        // for each p ∈ s \ s' do rhsp := 0 AdjustVariable(p)
        handle_current_state(state);
        // for each p ∈ s 0 \ s do rhsp := 1 + mino∈O|p∈Add(o) xo AdjustVariable(p)
        handle_last_state(state);
    }
    // forever do
    solve_equations();
    // use hadd(s) = 1/2 sum goal cost
    int total_cost = compute_total_cost();
    // set s' := s
    last_state = State(state);
    return (total_cost);
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

int TestHeuristic::compute_total_cost() {
    int total_cost = 0;
    for (PropID goal_id : goal_propositions) {
        const Proposition *goal = get_proposition(goal_id);
        int goal_cost = goal->cost;
        if (goal_cost >= MAX_COST_VALUE)
            return DEAD_END;
        total_cost+=goal_cost;
    }
    return (total_cost/2);
 }

 // for each p ∈ s \ s' do rhsp := 0 AdjustVariable(p)
void TestHeuristic::handle_current_state(const State &state) {
    for (size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        PropID prop_id = get_prop_id(state[i]);
        if (state[i].get_value() != last_state[i].get_value()) {
            current_state[prop_id] = true;
            Proposition *prop = get_proposition(prop_id);
            prop->rhsq = 0;
            adjust_proposition(prop);
        } else {
            current_state[prop_id] = true;
        }
    }
}

// for each p ∈ s' \ s do rhsp := 1 + mino∈O|p∈Add(o) xo AdjustVariable(p)
void TestHeuristic::handle_last_state(const State &state) {
    for (size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        if (state[i].get_value() != last_state[i].get_value()) {
            PropID prop_id = get_prop_id(last_state[i]);
            Proposition *prop = get_proposition(prop_id);
            prop->rhsq = make_inf(1 + get_min_operator_cost(prop));
            adjust_proposition(prop);
        }
    }
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
