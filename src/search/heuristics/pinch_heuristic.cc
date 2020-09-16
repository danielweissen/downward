#include "pinch_heuristic.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"
#include <typeinfo>

#include <cassert>
#include <vector>
#include <tuple>

using namespace std;

namespace pinch_heuristic { 



const int PinchHeuristic::MAX_COST_VALUE;

// construction and destruction
PinchHeuristic::PinchHeuristic(const Options &opts)
    : RelaxationHeuristic(opts),
      last_state(task_proxy.get_initial_state()) {
    utils::g_log << "Initializing pinch heuristic..." << endl;
}

void PinchHeuristic::setup_exploration_queue() {
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
            op.cost = op.base_cost;
            op.rhsq = op.base_cost;
            op.val_in_queue = op.base_cost;
            //this isnt part of the pseudo code
            queue.push(op.base_cost,make_op(get_op_id(op)));
            ++num_in_queue;
        }
    }    
}

void PinchHeuristic::adjust_proposition(Proposition *prop) {
    if(prop->cost != prop->rhsq) {
        if(prop->val_in_queue == -1) {
            ++num_in_queue;
        }
        int min = std::min(prop->cost, prop->rhsq);
        prop->val_in_queue = min;
        queue.push(min, get_prop_id(*prop));
    } else if(prop->val_in_queue != -1) {
        prop->val_in_queue = -1;
        --num_in_queue;
    }
}

void PinchHeuristic::adjust_operator(UnaryOperator  *un_op) {
    if(un_op->cost != un_op->rhsq) {
        if(un_op->val_in_queue == -1) {
            ++num_in_queue;
        }
        int min = std::min(un_op->cost, un_op->rhsq);
        un_op->val_in_queue = min;
        queue.push(min, make_op(get_op_id(*un_op)));
    } else if(un_op->val_in_queue != -1) {
        un_op->val_in_queue = -1;
        --num_in_queue;
    }
}

int PinchHeuristic::get_pre_condition_sum(OpID id) {
    int sum = 0;
    for(PropID prop : get_preconditions(id)) {
        if(sum >= MAX_COST_VALUE) {
            return MAX_COST_VALUE;
        }
        sum += get_proposition(prop)->cost;
    }
    return sum;
}

bool PinchHeuristic::prop_is_part_of_s(PropID prop) {
    return current_state[prop];
}
 
int PinchHeuristic::make_op(OpID op) {
    ++op;
    return -op;
}

int PinchHeuristic::get_min_operator_cost(Proposition *prop) {
    if(prop->add_effects.empty()) {
        return MAX_COST_VALUE;
    }

    OpID min = prop->add_effects.front();
    UnaryOperator * min_op = get_operator(min);
    UnaryOperator * current;
    for(OpID a : prop->add_effects) {
        current = get_operator(a);
        if((current->cost) < (min_op->cost)) {
            min_op = current;
        }
    }
    return  (min_op->base_cost + min_op->cost);
}

int PinchHeuristic::make_inf(int a) {
    return std::min(a, MAX_COST_VALUE);
}

void PinchHeuristic::solve_equations() {
    int queue_val;
    int index;
    int x_old;
    Proposition * prop;
    UnaryOperator * op;
    std::pair<int,int> top;
    PropID add;
    int old_cost;
    while(num_in_queue > 0) {
        top = queue.top();
        queue_val = top.first;
        index = top.second;
        if (index < 0) {
            index = make_op(index);
            op = get_operator(index);
            if(op->val_in_queue == queue_val) {
                if(op->rhsq < op->cost) {
                    queue.pop();
                    op->val_in_queue = -1;
                    --num_in_queue;
                    op->cost = op->rhsq;
                    add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        prop = get_proposition(add);
                        prop->rhsq = make_inf(std::min(prop->rhsq,(op->base_cost+op->cost)));
                        adjust_proposition(prop);
                    }
                    continue;
                } else {
                    x_old = op->cost;
                    op->cost = MAX_COST_VALUE;
                    op->rhsq = make_inf(op->base_cost + get_pre_condition_sum(index));
                    adjust_operator(op);
                    add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        prop = get_proposition(add);
                        if(prop->rhsq == (op->base_cost + x_old)) {
                            prop->rhsq = make_inf(get_min_operator_cost(prop));
                            adjust_proposition(prop);
                        }
                    }
                    continue;
                }
            } else {
                queue.pop();
                continue;
            }
        } else { 
            prop = get_proposition(index);
            if(prop->val_in_queue == queue_val) {
                if (prop->rhsq < prop->cost) {
                    queue.pop();
                    prop->val_in_queue = -1;
                    --num_in_queue;
                    old_cost = prop->cost;
                    prop->cost = prop->rhsq;
                    for (OpID op_id : precondition_of_pool.get_slice(
                        prop->precondition_of, prop->num_precondition_occurences)) {
                        op = get_operator(op_id);
                        if (op->rhsq >= MAX_COST_VALUE) {
                            op->rhsq = make_inf(op->base_cost + get_pre_condition_sum(op_id));
                        } else {
                            op->rhsq = make_inf(op->rhsq - old_cost + prop->cost);
                        }
                        adjust_operator(op);
                    }
                    continue;
                } else {
                    prop->cost = MAX_COST_VALUE;
                    if (!prop_is_part_of_s(index)) {
                        prop->rhsq = make_inf(get_min_operator_cost(prop));
                        adjust_proposition(prop);
                    }
                    for (OpID op_id : precondition_of_pool.get_slice(
                        prop->precondition_of, prop->num_precondition_occurences)) {
                        op = get_operator(op_id);
                        op->rhsq = MAX_COST_VALUE;
                        adjust_operator(op);
                    }
                    continue;
                }
            } else {
                queue.pop();
                continue;
            }
        }
    }
}

int PinchHeuristic::compute_heuristic(const State &state) {
    num_in_queue = 0;
    current_state = vector<bool>(propositions.size(), false);

    if(first_time) {
        num_in_queue = 0;
        current_state = vector<bool>(propositions.size(), false);
        queue.clear();
        setup_exploration_queue();
        assert(state == task_proxy.get_initial_state());
        for(FactProxy fact : state) { 
            PropID prop_id = get_prop_id(fact);
            current_state[prop_id] = true;
            Proposition *prop = get_proposition(prop_id);
            prop->rhsq = 0;
            adjust_proposition(prop);
        }
        first_time = false;
    } else {
        queue.clear();
        handle_current_state(state);
        handle_last_state(state);
    }

    solve_equations();

    int total_cost = compute_total_cost();

    last_state = State(state);

    return (total_cost);
}


int PinchHeuristic::compute_heuristic(const GlobalState &global_state) {
    return compute_heuristic(convert_global_state(global_state));
}

int PinchHeuristic::compute_total_cost() {
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
void PinchHeuristic::handle_current_state(const State &state) {
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
void PinchHeuristic::handle_last_state(const State &state) {
    for (size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        if (state[i].get_value() != last_state[i].get_value()) {
            PropID prop_id = get_prop_id(last_state[i]);
            Proposition *prop = get_proposition(prop_id);
            prop->rhsq = make_inf(get_min_operator_cost(prop));
            adjust_proposition(prop);
        }
    }
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis("PINCH additive heuristic", "");
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
        return make_shared<PinchHeuristic>(opts);
}

static Plugin<Evaluator> _plugin("pinch", _parse);


}
