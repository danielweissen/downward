#include "pinch_tracking_heuristic.h"

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

namespace pinch_tracking_heuristic { 



const int PinchTrackingHeuristic::MAX_COST_VALUE;

// construction and destruction
PinchTrackingHeuristic::PinchTrackingHeuristic(const Options &opts)
    : RelaxationHeuristic(opts),
      last_state(task_proxy.get_initial_state()) {
    utils::g_log << "Initializing pinch tracking heuristic..." << endl;
}

void PinchTrackingHeuristic::setup_exploration_queue() {
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

void PinchTrackingHeuristic::adjust_proposition(Proposition *prop) {
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

void PinchTrackingHeuristic::adjust_operator(UnaryOperator  *un_op) {
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

int PinchTrackingHeuristic::get_pre_condition_sum(OpID id) {
    int sum = 0;
    for(PropID prop : get_preconditions(id)) {
        if(sum >= MAX_COST_VALUE) {
            return MAX_COST_VALUE;
        }
        sum += get_proposition(prop)->cost;
    }
    return sum;
}

bool PinchTrackingHeuristic::prop_is_part_of_s(PropID prop) {
    return current_state[prop];
}
 
int PinchTrackingHeuristic::make_op(OpID op) {
    op++;
    return -op;
}

int PinchTrackingHeuristic::get_min_operator_cost(Proposition *prop) {
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

int PinchTrackingHeuristic::make_inf(int a) {
    return std::min(a, MAX_COST_VALUE);
}

void PinchTrackingHeuristic::solve_equations() {
    while(num_in_queue > 0) {
        std::pair<int,int> top = queue.top();
        int queue_val = top.first;
        int index = top.second;
        num_out_of_queue++;
        if (index < 0) {
            index = make_op(index);
            UnaryOperator *op = get_operator(index);
            if(op->val_in_queue == queue_val) {
                num_out_of_queue_and_processed++;
                if(op->rhsq < op->cost) {
                    num_of_over_consistent_q++;
                    queue.pop();
                    op->val_in_queue = -1;
                    num_in_queue--;
                    op->cost = op->rhsq;
                    number_of_op_cost_adjustments[index] = number_of_op_cost_adjustments[index]+1;
                    PropID add = op->effect;
                    if(!prop_is_part_of_s(add)) {
                        Proposition *prop = get_proposition(add);
                        prop->rhsq = make_inf(std::min(prop->rhsq,(1+op->cost)));
                        adjust_proposition(prop);
                    }
                    continue;
                } else {
                    num_of_under_consitent_q++;
                    int x_old = op->cost;
                    op->cost = MAX_COST_VALUE;
                    number_of_op_cost_adjustments[index] = number_of_op_cost_adjustments[index]+1;
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
                    continue;
                }
            } else {
                queue.pop();
                continue;
            }
        } else { 
            Proposition *prop = get_proposition(index);
            if(prop->val_in_queue == queue_val) {
                num_out_of_queue_and_processed++;
                if (prop->rhsq < prop->cost) {
                    num_of_over_consistent_q++;
                    queue.pop();
                    prop->val_in_queue = -1;
                    num_in_queue--;
                    int old_cost = prop->cost;
                    prop->cost = prop->rhsq;
                    number_of_prop_cost_adjustments[index] = number_of_prop_cost_adjustments[index]+1;
                    for (OpID op_id : precondition_of_pool.get_slice(
                        prop->precondition_of, prop->num_precondition_occurences)) {
                        UnaryOperator* op = get_operator(op_id);
                        if (op->rhsq >= MAX_COST_VALUE) {
                            op->rhsq = make_inf(1 + get_pre_condition_sum(op_id));
                        } else {
                            op->rhsq = make_inf(op->rhsq - old_cost + prop->cost);
                        }
                        adjust_operator(op);
                    }
                    continue;
                } else {
                    num_of_under_consitent_q++;
                    prop->cost = MAX_COST_VALUE;
                    number_of_prop_cost_adjustments[index] = number_of_prop_cost_adjustments[index]+1;
                    if (!prop_is_part_of_s(index)) {
                        prop->rhsq = make_inf(1 + get_min_operator_cost(prop));
                        adjust_proposition(prop);
                    }
                    for (OpID op_id : precondition_of_pool.get_slice(
                        prop->precondition_of, prop->num_precondition_occurences)) {
                        UnaryOperator* op = get_operator(op_id);
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

int PinchTrackingHeuristic::compute_heuristic(const State &state) {

    num_of_different_state_variables = 0;
    num_of_under_consitent_q = 0;
    num_of_over_consistent_q = 0;
    num_out_of_queue_and_processed = 0;
    num_out_of_queue = 0;
    num_in_queue = 0;
    current_state = vector<bool>(propositions.size(), false);
    number_of_prop_cost_adjustments = vector<int>(propositions.size(), 0);
    number_of_op_cost_adjustments = vector<int>(unary_operators.size(), 0);

    if(first_time) {
        num_of_true_state_variable = task_proxy.get_variables().size();
        num_in_queue = 0;
        current_state = vector<bool>(propositions.size(), false);
        queue.clear();
        setup_exploration_queue();
        assert(state == task_proxy.get_initial_state());
        for(FactProxy fact : state) { 
            num_of_different_state_variables++;
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

    calc_means();

    return (total_cost);
}

void PinchTrackingHeuristic::calc_means() {
    number_of_under_consistent_q.push_back(num_of_under_consitent_q);
    number_of_over_consistent_q.push_back(num_of_over_consistent_q);
    number_of_state_variables_not_in_common.push_back(num_of_different_state_variables);
    number_out_of_queue.push_back(num_out_of_queue);
    number_out_of_queue_and_processed.push_back(num_out_of_queue_and_processed);
    update_adjustment_means();
}

std::tuple<int,int,int> PinchTrackingHeuristic::get_num_adjustments() {
    int counter0 = 0;
    int counter1 = 0;
    int counter2 = 0;
    for(int i : number_of_prop_cost_adjustments) {
        if(i==0) {
            counter0++;
        } else if(i==1) {
            counter1++;
        } else if(i==2) {
            counter2++;
        }
    }
    for(int i : number_of_op_cost_adjustments) {
        if(i==0) {
            counter0++;
        } else if(i==1) {
            counter1++;
        } else if(i==2) {
            counter2++;
        }
    }
    return std::make_tuple(counter0,counter1,counter2);
}

void PinchTrackingHeuristic::update_adjustment_means() {
    std::tuple<int,int,int> val = get_num_adjustments();
    adjustment_0.push_back(std::get<0>(val));
    adjustment_1.push_back(std::get<1>(val));
    adjustment_2.push_back(std::get<2>(val));
}

double PinchTrackingHeuristic::get_state_variables_not_in_common_variance() {
    double var = 0;
    for(int n = 0; n < (int)number_of_state_variables_not_in_common.size(); n++ )    {
        var += (number_of_state_variables_not_in_common[n] - number_of_state_variables_not_in_common_mean) * (number_of_state_variables_not_in_common[n] - number_of_state_variables_not_in_common_mean);
    }
    var /= number_of_state_variables_not_in_common.size();
    number_of_state_variables_not_in_common_variance = var;
    return var;
    //sd = sqrt(var);
}

double PinchTrackingHeuristic::get_current_adjustment_mean(int which) {
    if(which == 0) {
        adjustment_0_mean = ((std::accumulate(std::begin(adjustment_0), std::end(adjustment_0), 0.0) / adjustment_0.size()) / (propositions.size() + unary_operators.size()));
        return adjustment_0_mean;
    } else if(which == 1) {
        adjustment_1_mean = ((std::accumulate(std::begin(adjustment_1), std::end(adjustment_1), 0.0) / adjustment_1.size()) / (propositions.size() + unary_operators.size()));
        return adjustment_1_mean;
    } else if(which == 2) {
        adjustment_2_mean = ((std::accumulate(std::begin(adjustment_2), std::end(adjustment_2), 0.0) / adjustment_2.size()) / (propositions.size() + unary_operators.size()));
        return adjustment_2_mean;
    } else if(which == 3) {
        return ((std::accumulate(std::begin(adjustment_0), std::end(adjustment_0), 0.0) / adjustment_0.size()));
    } else if(which == 4) {
        return ((std::accumulate(std::begin(adjustment_1), std::end(adjustment_1), 0.0) / adjustment_1.size()));
    } else {
        return ((std::accumulate(std::begin(adjustment_2), std::end(adjustment_2), 0.0) / adjustment_2.size()));
    }
}

int PinchTrackingHeuristic::get_total_number_of_variables() {
    return propositions.size();
}
int PinchTrackingHeuristic::get_total_number_of_operators() {
    return unary_operators.size();
}
int PinchTrackingHeuristic::get_total_number_of_q() {
    return (propositions.size() + unary_operators.size());
}

// number of state variables that change from one state to another in relation to total number of state variables
double PinchTrackingHeuristic::get_state_variables_not_in_common_mean() {
    number_of_state_variables_not_in_common_mean = ((std::accumulate(std::begin(number_of_state_variables_not_in_common), std::end(number_of_state_variables_not_in_common), 0.0) / number_of_state_variables_not_in_common.size()) / num_of_true_state_variable);
    return number_of_state_variables_not_in_common_mean;
}

double PinchTrackingHeuristic::get_number_out_of_queue_mean() {
    number_out_of_queue_mean = ((std::accumulate(std::begin(number_out_of_queue), std::end(number_out_of_queue), 0.0) / number_out_of_queue.size()));
    return number_out_of_queue_mean;
}

double PinchTrackingHeuristic::get_number_out_of_queue_processed_mean() {
    number_out_of_queue_mean_processed = ((std::accumulate(std::begin(number_out_of_queue_and_processed), std::end(number_out_of_queue_and_processed), 0.0) / number_out_of_queue_and_processed.size()));
    return number_out_of_queue_mean_processed;
}

// number of q that are under consisten relation to total amount of q
double PinchTrackingHeuristic::get_under_consisten_variables_mean() {
    number_of_under_consistent_q_mean = ((std::accumulate(std::begin(number_of_under_consistent_q), std::end(number_of_under_consistent_q), 0.0) / number_of_under_consistent_q.size()) / (propositions.size() + unary_operators.size()));
    return number_of_under_consistent_q_mean;
}

double PinchTrackingHeuristic::get_over_consisten_variables_mean() {
    number_of_over_consistent_q_mean = ((std::accumulate(std::begin(number_of_over_consistent_q), std::end(number_of_over_consistent_q), 0.0) / number_of_over_consistent_q.size()) / (propositions.size() + unary_operators.size()));
    return number_of_over_consistent_q_mean;
}

int PinchTrackingHeuristic::compute_heuristic(const GlobalState &global_state) {
    return compute_heuristic(convert_global_state(global_state));
}

int PinchTrackingHeuristic::compute_total_cost() {
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
void PinchTrackingHeuristic::handle_current_state(const State &state) {
    for (size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        PropID prop_id = get_prop_id(state[i]);
        if (state[i].get_value() != last_state[i].get_value()) {
            num_of_different_state_variables++;
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
void PinchTrackingHeuristic::handle_last_state(const State &state) {
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
    parser.document_synopsis("PINCH tracking heuristic", "");
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
        return make_shared<PinchTrackingHeuristic>(opts);
}

static Plugin<Evaluator> _plugin("pinch_tracking", _parse);


}
