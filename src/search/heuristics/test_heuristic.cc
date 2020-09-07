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
        prop.priority = -1;
    }
    for(UnaryOperator &op : unary_operators) {
        op.cost = MAX_COST_VALUE;
        op.rhsq = MAX_COST_VALUE;
        op.priority = -1;
    }

    // for each o ∈ O with Prec(o) = ∅ do set rhso := xo := 1
    for(UnaryOperator &op : unary_operators) {
        if(get_preconditions_vector(get_op_id(op)).empty()) {
            op.cost = 1;
            op.rhsq = 1;
            op.priority = -1;
        }
    }

    
    /**for (Proposition &prop : propositions) {
        prop.cost = MAX_COST_VALUE; // == infinity
        prop.rhsq = MAX_COST_VALUE;
        prop.priority = -1;
        prop.del_bound = MAX_COST_VALUE;
    }
    for (UnaryOperator &op : unary_operators) {
        if(get_preconditions_vector(get_op_id(op)).empty()) {
            op.cost = 1;
            op.rhsq = 1;
            op.del_bound = 1;
        } else {
            op.cost = MAX_COST_VALUE;
            op.rhsq = MAX_COST_VALUE;
            op.del_bound = MAX_COST_VALUE;
        }
        op.priority = -1;
    }*/
    
}

/**void TestHeuristic::setup_exploration_queue() {
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
*/

void TestHeuristic::adjust_proposition(Proposition *prop) {
    if(prop->cost != prop->rhsq) {
        if(prop->priority == -1) {
            ++num_in_queue;
        }
        prop->priority = std::min(prop->cost, prop->rhsq);
    } else if(prop->priority != -1) {
        prop->priority = -1;
        --num_in_queue;
    }
}

void TestHeuristic::adjust_operator(UnaryOperator  *un_op) {
    if(un_op->cost != un_op->rhsq) {
        if(un_op->priority == -1) {
            ++num_in_queue;
        }
        un_op->priority = std::min(un_op->cost, un_op->rhsq);
    } else if(un_op->priority != -1) {
        un_op->priority = -1;
        --num_in_queue;
    }
}

/**void TestHeuristic::adjust_variable(int q) {
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
*/

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

OpID TestHeuristic::getMinOperator(Proposition *prop) {
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
    return std::min(a, MAX_COST_VALUE);
}

void TestHeuristic::solve_equations() {
    int num_props = static_cast<int>(propositions.size());
    while(num_in_queue > 0) {
        int index = get_min();
        if (index >= num_props) {
            index -= num_props;
            UnaryOperator *op = get_operator(index);
            if(op->rhsq < op->cost) {
                op->priority = -1;
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
                        OpID min_op = getMinOperator(prop);
                        prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
                        adjust_proposition(prop);
                    }
                }
            }
        } else {
            Proposition *prop = get_proposition(index);
            if (prop->rhsq < prop->cost) {
                prop->priority = -1;
                num_in_queue--;
                int old_cost = prop->cost;
                prop->cost = prop->rhsq;
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

            } else {
                prop->cost = MAX_COST_VALUE;
                if (!prop_is_part_of_s(index)) {
                    OpID min_op = getMinOperator(prop);
                    prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
                    adjust_proposition(prop);
                }
                for (OpID op_id : precondition_of_pool.get_slice(
                    prop->precondition_of, prop->num_precondition_occurences)) {
                    UnaryOperator* op = get_operator(op_id);
                    op->rhsq = MAX_COST_VALUE;
                    adjust_operator(op);
                }
            }
        }
    }
}

// Indices larger than or equal to num_props are used for operators
int TestHeuristic::get_min() {
    int result = -1;
    int min = MAX_COST_VALUE + 1;
    int index = 0;
    for (Proposition &prop : propositions) {
        int prio = prop.priority;
        if (prio >= 0 && prio < min) {
            result = index;
            min = prio;
        }
        ++index;
    }
    for (UnaryOperator &un_op : unary_operators) {
        int prio = un_op.priority;
        if (prio >= 0 && prio < min) {
            result = index;
            min = prio;
        }
        ++index;
    }
    assert(result >= 0);
    return result;
}

/**std::pair<int,int> TestHeuristic::get_min() {
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
*/

/**void TestHeuristic::solve_equations() {

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
*/

int TestHeuristic::compute_heuristic(const State &state) {
    if(first_time) {
        //empty the priority queue
        num_in_queue = 0;
        current_state = vector<bool>(propositions.size(), false);
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
        for(FactProxy fact : state) {
            PropID prop_id = get_prop_id(fact);
            current_state[prop_id] = true;
        }
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
            //current_state[prop_id] = true;
            Proposition *prop = get_proposition(prop_id);
            prop->rhsq = 0;
            adjust_proposition(prop);
        } else {
            //current_state[prop_id] = true;
        }
    }
}

// for each p ∈ s' \ s do rhsp := 1 + mino∈O|p∈Add(o) xo AdjustVariable(p)
void TestHeuristic::handle_last_state(const State &state) {
    for (size_t i = 0; i < task_proxy.get_variables().size(); ++i) {
        if (state[i].get_value() != last_state[i].get_value()) {
            PropID prop_id = get_prop_id(last_state[i]);
            Proposition *prop = get_proposition(prop_id);
            OpID min_op = getMinOperator(prop);
            prop->rhsq = make_inf(1 + get_operator(min_op)->cost);
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
