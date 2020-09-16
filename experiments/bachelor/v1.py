#! /usr/bin/env python3

import itertools
import os

from lab.environments import LocalEnvironment, BaselSlurmEnvironment
from lab.reports import Attribute, arithmetic_mean

from downward.reports.compare import ComparativeReport

import common_setup
from common_setup import IssueConfig, IssueExperiment

DIR = os.path.dirname(os.path.abspath(__file__))
SCRIPT_NAME = os.path.splitext(os.path.basename(__file__))[0]
BENCHMARKS_DIR = os.environ["DOWNWARD_BENCHMARKS"]
REVISIONS = ["HEAD"]
CONFIGS = [
    IssueConfig("inc-add", ["--search", "astar(pinch())"], driver_options=[]),
    IssueConfig("add", ["--search", "astar(add())"], driver_options=[]),
    IssueConfig("inc-add-tracking", ["--search", "astar(pinch_tracking())"], driver_options=[]),
]

SUITE = common_setup.DEFAULT_SATISFICING_SUITE_UC
#SUITE = common_setup.DEFAULT_OPTIMAL_SUITE
#SUITE = IssueExperiment.DEFAULT_TEST_SUITE

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
    "perc_1_adjust",
    "perc_2_adjust",
    "perc_overconsistent",
    "perc_underconsistent",
    "avg_num_diff_vars",
    "var_num_diff_vars",
]

exp.add_absolute_report_step(attributes=ATTRIBUTES)
#exp.add_comparison_table_step()
#exp.add_scatter_plot_step(relative=True, attributes=["search_time", "total_time"])

exp.run_steps()
