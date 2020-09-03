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
    std::vector<int> new_state;
    std::vector<int> old_state;

    priority_queues::HeapQueue<int> queue;
    std::vector<int> queue_propositions;
    std::vector<int> queue_operators;
    int num_in_queue;

    

    bool did_write_overflow_warning;
    bool first_time = true;

    void setup_exploration_queue();
    void setup_exploration_queue_2();
    void adjust_variable(int q);
    void adjust_variable_2(int q, int type);
    OpID getMinOperator(Proposition * prop);
    int get_pre_condition_sum(OpID id);
    void solve_equations();
    void solve_equations_2();
    std::pair<int,int> get_min();
    bool prop_is_part_of_s(PropID prop);
    int make_inf(int a);
    int make_op(int q);
    OpID get_op(int q);
    std::vector<int> manage_state_comparison(std::vector<int> & bigger, std::vector<int> & smaller, int which);

    void write_overflow_warning();

    int compute_heuristic(const State &state);
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit TestHeuristic(const options::Options &opts);
};


}


#endif
