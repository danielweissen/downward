#! /usr/bin/env python

from lab.parser import Parser
import re

def main():
    print("Running custom parser")
    parser = Parser()
    parser.add_pattern(
        "perc_0_adjust", "mean number of q cost that have been adjusted 0 times in relation to total number of q: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "perc_1_adjust", "mean number of q cost that have been adjusted 1 times in relation to total number of q: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "perc_2_adjust", "mean number of q cost that have been adjusted 2 times in relation to total number of q: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "perc_overconsistent", "mean number of q that are at some point overconsitent in relation to total number of q: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "perc_underconsistent", "mean number of q that are at first underconsisten in relation to total number of q: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "avg_num_diff_vars", "mean number of state variables that are not in common between two following states in relation to total number of state variables: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "var_num_diff_vars", "variance number of state variables that are not in common between two following states in relation to total number of state variables: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "perc_0_adjust_number", "mean number of q cost that have been adjusted 0 times: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "perc_1_adjust_number", "mean number of q cost that have been adjusted 1 times: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "perc_2_adjust_number", "mean number of q cost that have been adjusted 2 times: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "total_num_var", "total number of variables: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "total_num_op", "total number of operators: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "total_num_q", "total number of qs: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "total_num_q_popped", "total number of q poped out of queue mean: (.+)$", type=float, flags='M')
    parser.add_pattern(
        "total_num_q_popped_processed", "total number of q poped out of queue and processed mean: (.+)$", type=float, flags='M')
    parser.parse()


main()
