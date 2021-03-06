#include "additive_heuristic_tracking.h"

#include "../global_state.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"
#include "../utils/logging.h"

#include <cassert>
#include <vector>

using namespace std;

namespace additive_heuristic_tracking {
const int AdditiveHeuristicTracking::MAX_COST_VALUE;

// construction and destruction
AdditiveHeuristicTracking::AdditiveHeuristicTracking(const Options &opts)
    : RelaxationHeuristic(opts),
      did_write_overflow_warning(false) {
    utils::g_log << "Initializing additive heuristic..." << endl;
}

void AdditiveHeuristicTracking::write_overflow_warning() {
    if (!did_write_overflow_warning) {
        // TODO: Should have a planner-wide warning mechanism to handle
        // things like this.
        utils::g_log << "WARNING: overflow on h^add! Costs clamped to "
                     << MAX_COST_VALUE << endl;
        cerr << "WARNING: overflow on h^add! Costs clamped to "
             << MAX_COST_VALUE << endl;
        did_write_overflow_warning = true;
    }
}

// heuristic computation
void AdditiveHeuristicTracking::setup_exploration_queue() {
    queue.clear();
    num_of_cost_adjustments = 0;
    for (Proposition &prop : propositions) {
        prop.cost = -1;
        prop.marked = false;
    }

    // Deal with operators and axioms without preconditions.
    for (UnaryOperator &op : unary_operators) {
        op.unsatisfied_preconditions = op.num_preconditions;
        op.cost = op.base_cost; // will be increased by precondition costs
        if (op.unsatisfied_preconditions == 0) {
            enqueue_if_necessary(op.effect, op.base_cost, get_op_id(op));
        }
    }
}

void AdditiveHeuristicTracking::setup_exploration_queue_state(const State &state) {
    for (FactProxy fact : state) {
        PropID init_prop = get_prop_id(fact);
        enqueue_if_necessary(init_prop, 0, NO_OP);
    }
}

void AdditiveHeuristicTracking::relaxed_exploration() {
    num_out_of_queue = 0;
    num_out_of_queue_and_processed = 0; 
    int unsolved_goals = goal_propositions.size();
    while (!queue.empty()) {
        pair<int, PropID> top_pair = queue.pop();
        ++num_out_of_queue;
        int distance = top_pair.first;
        PropID prop_id = top_pair.second;
        Proposition *prop = get_proposition(prop_id);
        int prop_cost = prop->cost;
        num_of_cost_adjustments++;
        assert(prop_cost >= 0);
        assert(prop_cost <= distance);
        if (prop_cost < distance)
            continue;
        ++num_out_of_queue_and_processed;
        if (prop->is_goal && --unsolved_goals == 0)
            return;
        for (OpID op_id : precondition_of_pool.get_slice(
                 prop->precondition_of, prop->num_precondition_occurences)) {
            UnaryOperator *unary_op = get_operator(op_id);
            increase_cost(unary_op->cost, prop_cost);
            --unary_op->unsatisfied_preconditions;
            assert(unary_op->unsatisfied_preconditions >= 0);
            if (unary_op->unsatisfied_preconditions == 0) {
                num_of_cost_adjustments++;
                enqueue_if_necessary(unary_op->effect,
                                     unary_op->cost, op_id);
            }
        }
    }
}

void AdditiveHeuristicTracking::mark_preferred_operators(
    const State &state, PropID goal_id) {
    Proposition *goal = get_proposition(goal_id);
    if (!goal->marked) { // Only consider each subgoal once.
        goal->marked = true;
        OpID op_id = goal->reached_by;
        if (op_id != NO_OP) { // We have not yet chained back to a start node.
            UnaryOperator *unary_op = get_operator(op_id);
            bool is_preferred = true;
            for (PropID precond : get_preconditions(op_id)) {
                mark_preferred_operators(state, precond);
                if (get_proposition(precond)->reached_by != NO_OP) {
                    is_preferred = false;
                }
            }
            int operator_no = unary_op->operator_no;
            if (is_preferred && operator_no != -1) {
                // This is not an axiom.
                OperatorProxy op = task_proxy.get_operators()[operator_no];
                assert(task_properties::is_applicable(op, state));
                set_preferred(op);
            }
        }
    }
}

int AdditiveHeuristicTracking::compute_add_and_ff(const State &state) {
    setup_exploration_queue();
    setup_exploration_queue_state(state);
    relaxed_exploration();

    int total_cost = 0;
    for (PropID goal_id : goal_propositions) {
        const Proposition *goal = get_proposition(goal_id);
        int goal_cost = goal->cost;
        if (goal_cost == -1)
            return DEAD_END;
        increase_cost(total_cost, goal_cost);
    }

    number_out_of_queue.push_back(num_out_of_queue);
    number_out_of_queue_and_processed.push_back(num_out_of_queue_and_processed);
    number_of_cost_adjustments.push_back(num_of_cost_adjustments);
    //for (Proposition &prop : propositions) {
    //    utils::g_log << "prop id:" << get_prop_id(prop)<< endl;
    //    utils::g_log << "prop cost:" << prop.cost << endl;
    //}
    //for (UnaryOperator &op : unary_operators) {
    //    utils::g_log << "op id:" << get_op_id(op)<< endl;
    //    utils::g_log << "op cost:" << op.cost << endl;
    //}
    return total_cost;
}

int AdditiveHeuristicTracking::compute_heuristic(const State &state) {
    int h = compute_add_and_ff(state);
    if (h != DEAD_END) {
        for (PropID goal_id : goal_propositions)
            mark_preferred_operators(state, goal_id);
    }
    return h;
}

int AdditiveHeuristicTracking::get_total_number_of_variables() {
    return propositions.size();
}
int AdditiveHeuristicTracking::get_total_number_of_operators() {
    return unary_operators.size();
}
int AdditiveHeuristicTracking::get_total_number_of_q() {
    return (propositions.size() + unary_operators.size());
}

int AdditiveHeuristicTracking::compute_heuristic(const GlobalState &global_state) {
    return compute_heuristic(convert_global_state(global_state));
}

double AdditiveHeuristicTracking::get_number_out_of_queue_mean() {
    if(number_out_of_queue.empty()) {
        return 0;
    }
    number_out_of_queue_mean = ((std::accumulate(std::begin(number_out_of_queue), std::end(number_out_of_queue), 0.0) / number_out_of_queue.size()));
    return number_out_of_queue_mean;
}

double AdditiveHeuristicTracking::get_number_of_cost_adjustments_mean() {
    if(number_of_cost_adjustments.empty()) {
        return 0;
    }
    number_of_cost_adjustments_mean = ((std::accumulate(std::begin(number_of_cost_adjustments), std::end(number_of_cost_adjustments), 0.0) / number_of_cost_adjustments.size()));
    return number_of_cost_adjustments_mean;
}

double AdditiveHeuristicTracking::get_number_out_of_queue_processed_mean() {
    if(number_out_of_queue_and_processed.empty()) {
        return 0;
    }
    number_out_of_queue_mean_processed = ((std::accumulate(std::begin(number_out_of_queue_and_processed), std::end(number_out_of_queue_and_processed), 0.0) / number_out_of_queue_and_processed.size()));
    return number_out_of_queue_mean_processed;
}

double AdditiveHeuristicTracking::get_average_number_of_preconditions_per_operator() {
    std::vector<int> pre_number_vec = vector<int>(unary_operators.size(), 0);
    for(UnaryOperator &o : unary_operators) {
        pre_number_vec.push_back(o.num_preconditions);
    }
    if(pre_number_vec.empty()) {
        return 0;
    }
    return ((std::accumulate(std::begin(pre_number_vec), std::end(pre_number_vec), 0.0) / pre_number_vec.size()));
}

void AdditiveHeuristicTracking::compute_heuristic_for_cegar(const State &state) {
    compute_heuristic(state);
}

static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis("Additive heuristic tracking", "");
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
        return make_shared<AdditiveHeuristicTracking>(opts);
}

static Plugin<Evaluator> _plugin("add_tracking", _parse);
}
