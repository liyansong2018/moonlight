#!/usr/bin/env python

# Copyright 2017 The Australian National University
#
# This software is the result of a joint project between the Defence Science
# and Technology Group and the Australian National University. It was enabled
# as part of a Next Generation Technology Fund grant:
# see https://www.dst.defence.gov.au/nextgentechfund
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.

"""
Run MoonLight on the demo benchmarks.

Author: Adrian Herrera
"""

from __future__ import print_function

import argparse
import json
import os
import subprocess
import sys

try:
    import lzma
    import tarfile

    HAVE_LZMA = True
except ImportError:
    # Use subprocess instead
    HAVE_LZMA = False

try:
    from tempfile import TemporaryDirectory
except ImportError:
    from tempdir import TemporaryDirectory

from util import check_results, run_moonlight


# Global constants
CURRENT_DIR = os.path.dirname(__file__)
DATA_DIR = os.path.join(CURRENT_DIR, '..', 'data')
BENCHMARK_RESULTS_DIR = os.path.join(CURRENT_DIR, 'benchmark_results')

BENCHMARKS = {'adobe-pdf', 'microsoft-word', 'png', 'true-type-font', 'web-html'}


def decompress_data(archive_path, output_dir):
    """
    Decompress an archived benchmarks.

    All benchmarks are stored in tar.xz format in the data/ directory. How we
    perform the decompression depends on the version of Python available.
    """
    print('Extracting {}...'.format(archive_path), end=' ')
    sys.stdout.flush()

    if HAVE_LZMA:
        with tarfile.open(archive_path, mode='r:xz') as xz_file:
            xz_file.extractall(path=output_dir)
    else:
        tar_xz_cmd = ['tar', 'xJf', archive_path, '-C', output_dir]

        tar_xz = subprocess.Popen(tar_xz_cmd, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE)
        _, stderr = tar_xz.communicate()

        # Exit on an error
        if stderr:
            print('TAR ERROR: {}'.format(stderr))
            sys.exit(1)

    print('Done')


def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser(description='Run MoonLight benchmarks.')
    parser.add_argument('-m', '--moonlight-path', required=True,
                        help='Path to the MoonLight executable')
    parser.add_argument('-b', '--benchmarks', required=False, default=None,
                        nargs='+',
                        help='Only run particular benchmark(s). Valid '
                             'benchmarks are: {}'.format(', '.join(BENCHMARKS)))
    parser.add_argument('-d', '--data-dir', required=False, default=DATA_DIR,
                        help='Path to the benchmark data directory. Defaults '
                             'to {}'.format(DATA_DIR))

    return parser.parse_args()


def main():
    """
    The main function.
    """
    # Parse command-line arguments
    args = parse_args()

    if args.benchmarks:
        # Run only the benchmarks specified by the user. We have to validate
        # them first
        benchmarks_to_run = []

        for benchmark in args.benchmarks:
            if benchmark in BENCHMARKS:
                benchmarks_to_run.append(benchmark)
            else:
                print('ERROR: {} is not a valid benchmark. '
                      'Skipping...'.format(benchmark))
    else:
        # No benchmarks specified - run all of them
        benchmarks_to_run = BENCHMARKS

    print('Running benchmarks: {}\n'.format(', '.join(benchmarks_to_run)))

    tests_passed = False
    for benchmark in benchmarks_to_run:
        compressed_corpus_path = os.path.join(args.data_dir,
                                              '{}.tar.xz'.format(benchmark))
        benchmark_result_path = os.path.join(BENCHMARK_RESULTS_DIR,
                                             '{}.json'.format(benchmark))

        # Check if the corpus archive exists
        if not os.path.isfile(compressed_corpus_path):
            print('ERROR: {} is not a valid corpus archive. '
                  'Skipping...'.format(compressed_corpus_path))
            continue

        # Check if the benchmark results exists
        if not os.path.isfile(benchmark_result_path):
            print('ERROR: No expected results exist for {}. '
                  'Skipping...'.format(benchmark))
            continue

        # Run MoonLight in the context of a temporary directory, which ensures
        # that all MoonLight-produced files are automatically cleaned up at the
        # end of each test
        with TemporaryDirectory() as temp_dir:
            # Decompress the benchmark data
            decompress_data(os.path.realpath(compressed_corpus_path), temp_dir)

            # Run MoonLight
            corpus_dir = os.path.join(temp_dir, benchmark)
            moonlight_cmd = [args.moonlight_path, '-d', corpus_dir,
                             '-r', 'exemplar_', '-i']

            results = run_moonlight(moonlight_cmd, corpus_dir)
            if 'error' in results:
                print('MOONLIGHT ERROR: {}'.format(results['error']))
                sys.exit(1)

            # Load the expected results
            with open(benchmark_result_path, 'r') as benchmark_file:
                expected_results = json.load(benchmark_file)

            # Compare the results against the expected values
            tests_passed = check_results(results, expected_results)

    # Set the return code
    sys.exit(not tests_passed)


if __name__ == '__main__':
    main()
