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
  #  IssueConfig("PINCH", ["--search","eager_wastar([pinch()], w=2)"], driver_options=[]),
  #  IssueConfig("GD", ["--search","eager_wastar([add()], w=2)"], driver_options=[]),
  #  IssueConfig("inc-add", ["--search","lazy_greedy([pinch()])"], driver_options=[]),
  #  IssueConfig("add", ["--search","lazy_greedy([add()])"], driver_options=[]),
    IssueConfig("PINCH", ["--search", "eager_wastar([pinch_tracking()], w=2)"], driver_options=[]),
    IssueConfig("GD", ["--search", "eager_wastar([add_tracking()], w=2)"], driver_options=[]),
]

#SUITE = common_setup.DEFAULT_SATISFICING_SUITE_UC
SUITE = common_setup.DEFAULT_SATISFICING_SUITE
#SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
#SUITE = IssueExperiment.DEFAULT_TEST_SUITE

def domain_as_category(run1, run2):
     return run1['domain']

def improvement(run1, run2):
     time1 = run1.get('search_time', 1800)
     time2 = run2.get('search_time', 1800)
     if time1 > time2:
         return 'GD favored'
     if time1 == time2:
         return 'Equal'
     return 'PINCH favored'

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
    Attribute("perc_0_adjust", function=arithmetic_mean,digits=6),
    Attribute("perc_1_adjust", function=arithmetic_mean,digits=6),
    Attribute("perc_2_adjust", function=arithmetic_mean,digits=6),
    Attribute("perc_overconsistent", function=arithmetic_mean,digits=6),
    Attribute("perc_underconsistent", function=arithmetic_mean,digits=6),
    Attribute("avg_num_diff_vars", function=arithmetic_mean,digits=6),
    Attribute("var_num_diff_vars", function=arithmetic_mean,digits=6),
    Attribute("perc_0_adjust_number", function=arithmetic_mean,digits=6),
    Attribute("perc_1_adjust_number", function=arithmetic_mean,digits=6),
    Attribute("perc_2_adjust_number", function=arithmetic_mean,digits=6),
    Attribute("total_num_var", function=arithmetic_mean,digits=6),
    Attribute("total_num_op", function=arithmetic_mean,digits=6),
    Attribute("total_num_q", function=arithmetic_mean,digits=6),
    Attribute("total_num_q_popped", function=arithmetic_mean,digits=6),
    Attribute("total_num_q_popped_processed", function=arithmetic_mean,digits=6),
    Attribute("average_num_pre_per_o", function=arithmetic_mean,digits=6),
    Attribute("perc_total_adjust_number", function=arithmetic_mean,digits=6),
    Attribute("state_true_var_relate_prop", function=arithmetic_mean,digits=6),
    Attribute("state_var_relate_prop", function=arithmetic_mean,digits=6),  
]



exp.add_absolute_report_step(attributes=ATTRIBUTES)
#exp.add_comparison_table_step(attributes=ATTRIBUTES)
#exp.add_scatter_plot_step(relative=True, attributes=["search_time", "total_time"])

#exp.add_report(ScatterPlotReport(attributes=["expansions_until_last_jump"],filter_algorithm=["add", "inc-add"],get_category=domain_as_category,format="png",),name="scatterplot-expansions")

exp.add_report(ScatterPlotReport(attributes=["search_time"],filter_domain=common_setup.DEFAULT_SATISFICING_SUITE_NOW,filter_algorithm=["HEAD-PINCH", "HEAD-GD"],get_category=improvement),outfile = "plot.png")

#exp.add_report(ScatterPlotReport(attributes=["search_time"],filter_algorithm=["HEAD-inc-add", "HEAD-add"],get_category=domain_as_category),outfile = "plotSearch.png")
#exp.add_report("ScatterPlotReport(attributes=["total_num_q_popped_processed"],filter_algorithm=["HEAD-PINCH", "HEAD-GD"],get_category=improvement),outfile = "plotSearchimpr.png")
#exp.add_report(ScatterPlotReport(scale='symlog',attributes=["perc_total_adjust_number"],filter_algorithm=["HEAD-PINCH", "HEAD-GD"],get_category=domain_as_category),outfile = "plotSearch.png")
#exp.add_report(ScatterPlotReport(attributes=["total_num_q_popped"],filter_algorithm=["HEAD-inc-add-tracking", "HEAD-add-tracking"],get_category=domain_as_category),outfile = "plotQPopped.png")

exp.run_steps()
 
