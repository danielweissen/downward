#ifndef HEURISTICS_TEST_HEURISTIC
#define HEURISTICS_TEST_HEURISTIC

#include "relaxation_heuristic.h"

#include "../algorithms/priority_queues.h"
#include "../utils/collections.h"

#include <cassert>
#include <iostream>
#include <tuple>

class State;

namespace test_heuristic {

struct xq;

using relaxation_heuristic::PropID;
using relaxation_heuristic::OpID;

using relaxation_heuristic::NO_OP;

using relaxation_heuristic::Proposition;
using relaxation_heuristic::UnaryOperator;





class TestHeuristic : public relaxation_heuristic::RelaxationHeuristic {


    /* Costs larger than MAX_COST_VALUE are clamped to max_value. The
       precise value (100M) is a bit of a hack, since other parts of
       the code don't reliably check against overflow as of this
       writing. With a value of 100M, we want to ensure that even
       weighted A* with a weight of 10 will have f values comfortably
       below the signed 32-bit int upper bound.
     */
    static const int MAX_COST_VALUE = 100000000;
    std::vector<bool> current_state;
    State last_state;

    priority_queues::HeapQueue<int> queue;
    int num_in_queue;

    bool first_time = true;

    void setup_exploration_queue();
    void adjust_proposition(Proposition *prop);
    void adjust_operator(UnaryOperator *un_op);
    int get_min_operator_cost(Proposition *prop);
    int get_pre_condition_sum(OpID id);
    void solve_equations();
    void handle_current_state(const State &state);
    void handle_last_state(const State &state);
    // std::pair<int,int> get_min();
    int get_min();
    bool prop_is_part_of_s(PropID prop);
    int make_inf(int a);
    int make_op(OpID op);
    int compute_total_cost();
    int compute_heuristic(const State &state);
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit TestHeuristic(const options::Options &opts);
};


}


#endif
