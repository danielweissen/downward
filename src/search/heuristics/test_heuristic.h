#ifndef HEURISTICS_TEST_HEURISTIC
#define HEURISTICS_TEST_HEURISTIC

#include "relaxation_heuristic.h"

#include "../algorithms/priority_queues.h"
#include "../utils/collections.h"

#include <cassert>

class State;

namespace test_heuristic {

struct xq;

using relaxation_heuristic::PropID;
using relaxation_heuristic::OpID;

using relaxation_heuristic::NO_OP;

using relaxation_heuristic::Proposition;
using relaxation_heuristic::UnaryOperator;



struct xq {
    int id; // id of prop or op
    int rhsq;
    unsigned int type : 1; // == 1 if Prop, == 0 if OP
};



class TestHeuristic : public relaxation_heuristic::RelaxationHeuristic {


    /* Costs larger than MAX_COST_VALUE are clamped to max_value. The
       precise value (100M) is a bit of a hack, since other parts of
       the code don't reliably check against overflow as of this
       writing. With a value of 100M, we want to ensure that even
       weighted A* with a weight of 10 will have f values comfortably
       below the signed 32-bit int upper bound.
     */
    static const int MAX_COST_VALUE = 100000000;
    std::vector<struct xq> p_and_o;

    priority_queues::AdaptiveQueue<xq> queue;
    bool did_write_overflow_warning;
    bool first_time = true;

    void setup_exploration_queue();
    void adjust_variable(struct xq &q,const State &state);
    bool xq_is_part_of_s(struct xq &q,const State &state);

    void write_overflow_warning();

    int compute_heuristic(const State &state);
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit TestHeuristic(const options::Options &opts);
};


}


#endif
