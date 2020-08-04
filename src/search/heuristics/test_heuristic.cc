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
    //struct xq bla = {1,-1,1};
    //queue.push(4,bla);
    //bla = {2,-1,0};
    //queue.push(2,bla);
    //bla = {3,-1,1};
    //queue.push(3,bla);
    //utils::g_log << queue.pop().second.type << endl;
    //utils::g_log << queue.pop().second.type << endl;
    //utils::g_log << queue.pop().second.type << endl;
    //struct xq bla2 = {2,10,0};
    //utils::g_log << queue.remove(bla2) << endl;
    //utils::g_log << queue.pop().second.id << endl;
    //utils::g_log << queue.pop().second.id << endl;
    //utils::g_log << queue.empty() << endl;
}



void TestHeuristic::setup_exploration_queue() {
    queue.clear();
    p_and_o.clear();
    struct xq temp;
    for (Proposition &prop : propositions) {
        prop.cost = MAX_COST_VALUE; // == infinity
        temp = {(int)get_prop_id(prop),MAX_COST_VALUE,1};
        p_and_o.push_back(temp);
        queue.push(MAX_COST_VALUE,temp);
    }
    for (UnaryOperator &op : unary_operators) {
        op.cost = MAX_COST_VALUE; // == infinity
        temp = {(int)get_op_id(op),MAX_COST_VALUE,0};
        p_and_o.push_back(temp);
        queue.push(MAX_COST_VALUE,temp);
    }
}

void TestHeuristic::adjust_variable(struct xq &q,const State &state) {
    if(q.type == 1) {               // if q is part of P
        PropID p_id = q.id;
        OpID min_op;
        Proposition *prop = get_proposition(p_id);
        if(xq_is_part_of_s(q,state)) {
            q.rhsq = 0;
        } else {
            if(prop->add_effects.size() != 0) {
                min_op = getMinOperator(prop);
                q.rhsq = 1 + get_operator(min_op)->cost;
            }
        }
        queue.remove(q);
        if(prop->cost != q.rhsq) {
            queue.push(std::min(prop->cost,q.rhsq),q);
        }

    } else {                        // q is part of O
        OpID op_id = q.id;
        UnaryOperator* un_op = get_operator(op_id);
        q.rhsq = 1 + get_pre_condition_sum(q);
        queue.remove(q);
        if(un_op->cost != q.rhsq) {
            queue.push(std::min(un_op->cost,q.rhsq),q);
        }
    }

}

int TestHeuristic::get_pre_condition_sum(struct xq &q) {
    OpID o_id = q.id;
    int sum = 0;
    for(PropID prop : get_preconditions(o_id)) {
        sum += get_proposition(prop)->cost;
    }
    return sum;
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

bool TestHeuristic::prop_is_part_of_s(PropID prop, const State &state) {
    for (FactProxy fact : state) {
        PropID init_prop = get_prop_id(fact);
        if((int)prop == (int)init_prop) {
            return true;
        }
    }
    return false;
}

OpID TestHeuristic::getMinOperator(Proposition * prop) {
    OpID min = prop->add_effects.front();
    for(OpID a : prop->add_effects) {
        if((int) get_operator(a)->cost < (int) get_operator(min)->cost) {
            min = a;
        }
    }
    return min;
}

void TestHeuristic::solve_equations(const State &state) {
    while(!queue.empty()) {
        struct xq current = queue.pop().second;
        if(current.type == 1) {
            Proposition *prop = get_proposition((PropID)current.id);
            if(current.rhsq < prop->cost) {
                prop->cost = current.rhsq;
                for (OpID op_id : precondition_of_pool.get_slice(
                 prop->precondition_of, prop->num_precondition_occurences)) {
                     struct xq bla = {(int)op_id,MAX_COST_VALUE,0};
                     auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
                     adjust_variable(*cur,state);
                 }
            } else {
                //utils::g_log << current.id << "         " << current.rhsq << "          " <<current.type << endl;
                prop->cost = MAX_COST_VALUE;
                adjust_variable(current,state);
                for (OpID op_id : precondition_of_pool.get_slice(
                 prop->precondition_of, prop->num_precondition_occurences)) {
                     struct xq bla = {(int)op_id,MAX_COST_VALUE,0};
                     auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
                     adjust_variable(*cur,state);
                 }
            }
        } else {
            UnaryOperator *op = get_operator((OpID)current.id);
            if(current.rhsq < op->cost) {
                op->cost = current.rhsq;
                PropID add = op->effect;
                if(!prop_is_part_of_s(add,state)) {
                    struct xq bla = {(int)add,MAX_COST_VALUE,1};
                    auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
                    adjust_variable(*cur,state);
                }
            } else {
                op->cost = MAX_COST_VALUE;
                adjust_variable(current,state);
                PropID add = op->effect;
                if(!prop_is_part_of_s(add,state)) {
                    struct xq bla = {(int)add,MAX_COST_VALUE,1};
                    auto cur = find(p_and_o.begin(), p_and_o.end(), bla);
                    adjust_variable(*cur,state);
                }
            }
        }
    }
}


vector<int> TestHeuristic::manage_state_comparison(vector<int> & bigger, vector<int> & smaller) {

    vector<int> c = {};
        sort(bigger.begin(),bigger.end());
        sort(smaller.begin(),smaller.end());

        int counta = 0;
        int countb = 0;



        while(counta < (int)bigger.size() && countb < (int)smaller.size()) {
                if(bigger[counta]<smaller[countb]) {
                    c.push_back(bigger[counta]);
                    counta++;
                } else if(bigger[counta]>smaller[countb]) {
                    c.push_back(smaller[countb]);
                    countb++;
                } else {
                    counta++;
                    countb++;
                }
        }
        return c;
}

int TestHeuristic::compute_heuristic(const State &state) {
    if(first_time) {
        setup_exploration_queue();
        for(struct xq q : p_and_o) {
            adjust_variable(q, state);
        }

        vector<int> a = {1,2,3,4,5,6,8,9};
        vector<int> b = {1,2,3,7,8,10,11};
        vector<int> c;
        if(a.size() < b.size()) {
            c = manage_state_comparison(b,a);
        } else {
            c = manage_state_comparison(a,b);
        }

        for(int i : c) {
            utils::g_log << i << endl;
        }
        first_time = false;
    } else {
        /**PropID prop;
        new_state.clear();
        for (FactProxy fact : state) {
            prop = get_prop_id(fact);
            new_state.push_back((int)prop);
        }*/
    }

    

    solve_equations(state);



    int total_cost = 0;
    for (PropID goal_id : goal_propositions) {
        const Proposition *goal = get_proposition(goal_id);
        int goal_cost = goal->cost;
        if (goal_cost >= MAX_COST_VALUE)
            return DEAD_END;
        total_cost+=goal_cost;
    }

    for(FactProxy v : state) {
        old_state.push_back(get_prop_id(v));
    }

    return (total_cost/2);
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
