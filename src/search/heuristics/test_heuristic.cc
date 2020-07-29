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
    struct xq bla = {1,-1,1};
    struct xq bla2 = {1,-1,0};
    queue.push(1,bla);
    queue.push(2,bla2);
    utils::g_log << queue.pop().second.type << endl;
    utils::g_log << queue.pop().second.type << endl;
}



void TestHeuristic::setup_exploration_queue() {
    queue.clear();
    struct xq temp;
    for (Proposition &prop : propositions) {
        prop.cost = -1;
        temp = {(int)get_prop_id(prop),-1,1};
        p_and_o.push_back(temp);
    }
    for (UnaryOperator &op : unary_operators) {
        op.cost = -1;
        temp = {(int)get_op_id(op),-1,0};
        p_and_o.push_back(temp);
    }
}

void TestHeuristic::adjust_variable(struct xq &q,const State &state) {
    if(q.type == 1) {               // if q is part of P
        if(xq_is_part_of_s(q,state)) {
            q.rhsq = 0;
        } else {
            PropID p_id = q.id;
            Proposition *prop = get_proposition(p_id);
            OpID op_id = prop->reached_by;
        }

    } else {                        // q is part of O

    }

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


int TestHeuristic::compute_heuristic(const State &state) {
    if(first_time) {
        setup_exploration_queue();
        for(struct xq q : p_and_o) {
            adjust_variable(q, state);
        }
        first_time = false;
    } else {

    }
    int h = 10;
    return h;
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
