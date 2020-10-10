#ifndef HEURISTICS_PINCH_TRACKING_HEURISTIC
#define HEURISTICS_PINCH_TRACKING_HEURISTIC

#include "relaxation_heuristic.h"

#include "../algorithms/priority_queues.h"
#include "../utils/collections.h"

#include <cassert>
#include <iostream>
#include <tuple>

class State;

namespace pinch_tracking_heuristic {


using relaxation_heuristic::PropID;
using relaxation_heuristic::OpID;

using relaxation_heuristic::NO_OP;

using relaxation_heuristic::Proposition;
using relaxation_heuristic::UnaryOperator;





class PinchTrackingHeuristic : public relaxation_heuristic::RelaxationHeuristic {


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
    int num_of_different_state_variables = 0;
    int num_of_same_state_variables = 0;
    int num_of_true_state_variable;
    int num_of_under_consitent_q = 0;
    int num_of_over_consistent_q = 0;
    int num_out_of_queue = 0;
    int num_out_of_queue_and_processed = 0;
    std::vector<int> number_of_state_variables_not_in_common;
    std::vector<int> number_of_state_variables_in_common;
    std::vector<int> number_of_under_consistent_q;
    std::vector<int> number_of_over_consistent_q;
    std::vector<int> number_of_prop_cost_adjustments;
    std::vector<int> number_of_op_cost_adjustments;
    std::vector<int> number_out_of_queue;
    std::vector<int> number_out_of_queue_and_processed;
    std::vector<int> adjustment_0;
    std::vector<int> adjustment_1;
    std::vector<int> adjustment_2;
    std::vector<int> adjustment_total;

    double adjustment_0_mean;
    double adjustment_1_mean;
    double adjustment_2_mean;
    double adjustment_total_mean;
    double number_of_state_variables_not_in_common_mean;
    double number_of_state_variables_in_common_mean;
    double number_of_under_consistent_q_mean;
    double number_of_over_consistent_q_mean;
    double number_of_prop_cost_adjustments_mean;
    double number_of_op_cost_adjustments_mean;
    double number_of_state_variables_not_in_common_variance;
    double number_out_of_queue_mean;
    double number_out_of_queue_mean_processed;
    

    priority_queues::BucketQueue<int> queue;
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
    bool prop_is_part_of_s(PropID prop);
    int make_inf(int a);
    int make_op(OpID op);
    int compute_total_cost();
    int compute_heuristic(const State &state);
    std::tuple<int,int,int> get_num_adjustments();
    void update_adjustment_means();
    void calc_means();
protected:
    virtual int compute_heuristic(const GlobalState &global_state) override;
public:
    explicit PinchTrackingHeuristic(const options::Options &opts);
    double get_current_adjustment_mean(int which);
    double get_state_variables_not_in_common_mean();
    double get_state_variables_not_in_common_mean_in_relation_to_total_number_of_state_variables();
    double get_under_consisten_variables_mean();
    double get_over_consisten_variables_mean();
    double get_state_variables_not_in_common_variance();
    double get_number_out_of_queue_mean();
    double get_number_out_of_queue_processed_mean();
    int get_total_number_of_variables();
    int get_total_number_of_operators();
    int get_total_number_of_q();
    double get_average_number_of_preconditions_per_operator();
    double get_mean_number_of_true_state_variables_in_relation_to_total_number_of_state_variables();

};


}


#endif
