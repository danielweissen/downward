#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, arithmetic_mean

from downward.reports.compare import ComparativeReport
from downward.experiment import FastDownwardExperiment

import common_setup
from common_setup import IssueConfig, IssueExperiment
from downward.reports.scatter import ScatterPlotReport
#from relativescatter import RelativeScatterPlotReport

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["HEAD"]
CONFIGS = [
    IssueConfig("inc-add", ["--search", "astar(pinch())"], driver_options=[]),
    IssueConfig("add", ["--search", "astar(add())"], driver_options=[]),
   # IssueConfig("inc-add-tracking", ["--search", "astar(pinch_tracking())"], driver_options=[]),
   # IssueConfig("add-tracking", ["--search", "astar(add_tracking())"], driver_options=[]),
]

#SUITE = common_setup.DEFAULT_SATISFICING_SUITE_UC
#SUITE = common_setup.DEFAULT_SATISFICING_SUITE
#SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
SUITE = IssueExperiment.DEFAULT_TEST_SUITE

def domain_as_category(run1, run2):
     return run1['domain']

ENVIRONMENT = BaselSlurmEnvironment(
    partition="infai_2",
    email="daniel.weissen@stud.unibas.ch",
    export=["PATH", "DOWNWARD_BENCHMARKS"])

if common_setup.is_test_run():
    SUITE = IssueExperiment.DEFAULT_TEST_SUITE
    ENVIRONMENT = LocalEnvironment(processes=1)

exp = IssueExperiment(
    revisions=REVISIONS,
    configs=CONFIGS,
    environment=ENVIRONMENT,
)
exp.add_suite(BENCHMARKS_DIR, SUITE)

#exp.add_parse_again_step()

exp.add_parser(exp.EXITCODE_PARSER)
exp.add_parser(exp.SINGLE_SEARCH_PARSER)
exp.add_parser(exp.PLANNER_PARSER)
exp.add_parser('custom-parser.py')

exp.add_step('build', exp.build)
exp.add_step('start', exp.start_runs)
exp.add_fetcher(name='fetch')

ATTRIBUTES = exp.DEFAULT_TABLE_ATTRIBUTES + [
    Attribute("perc_0_adjust", function=arithmetic_mean),
    Attribute("perc_1_adjust", function=arithmetic_mean),
    Attribute("perc_2_adjust", function=arithmetic_mean),
    Attribute("perc_overconsistent", function=arithmetic_mean),
    Attribute("perc_underconsistent", function=arithmetic_mean),
    Attribute("avg_num_diff_vars", function=arithmetic_mean),
    Attribute("var_num_diff_vars", function=arithmetic_mean),
    Attribute("perc_0_adjust_number", function=arithmetic_mean),
    Attribute("perc_1_adjust_number", function=arithmetic_mean),
    Attribute("perc_2_adjust_number", function=arithmetic_mean),
    Attribute("total_num_var", function=arithmetic_mean),
    Attribute("total_num_op", function=arithmetic_mean),
    Attribute("total_num_q", function=arithmetic_mean),
    Attribute("total_num_q_popped", function=arithmetic_mean),
    Attribute("total_num_q_popped_processed", function=arithmetic_mean),
    Attribute("average_num_pre_per_o", function=arithmetic_mean),
]



exp.add_absolute_report_step(attributes=ATTRIBUTES)
#exp.add_comparison_table_step(attributes=ATTRIBUTES)
#exp.add_scatter_plot_step(relative=True, attributes=["search_time", "total_time"])

#exp.add_report(ScatterPlotReport(attributes=["expansions_until_last_jump"],filter_algorithm=["add", "inc-add"],get_category=domain_as_category,format="png",),name="scatterplot-expansions")

exp.add_report(ScatterPlotReport(attributes=["search_time"],filter_algorithm=["astar(add())", "astar(pinch())"]),outfile = "plot")

exp.run_steps()
