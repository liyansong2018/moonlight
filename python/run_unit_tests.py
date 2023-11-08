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
Generate and run the MoonLight unit tests.

Author: Liam Hayes
"""

from __future__ import print_function
from __future__ import division

import argparse
import glob
import json
import math
import os
import sys

from builtins import bytes

try:
    from tempfile import TemporaryDirectory
except ImportError:
    from tempdir import TemporaryDirectory

from util import check_results, run_moonlight


# Global contsants
UNIT_TESTS_DIR = os.path.join(os.path.dirname(__file__), 'unit_tests')


def binary_data(string):
    """
    Takes a python string '1111111100000000' and converts it to a byte string
    b'\xff\x00'
    """
    padded_len = int(math.ceil(len(string) / 8) * 8)
    string = string.ljust(padded_len, '0')
    data = []

    while string:
        data.append(int(string[:8], 2))
        string = string[8:]

    return bytes(data)


def write_corpus(corpus_dir, corpus_data):
    """
    Take an exemplar_name to exemplar_data dictionary and write them into the
    corpus directory (in the format MoonLight expects).
    """
    for exemplar, data in corpus_data.items():
        with open(os.path.join(corpus_dir, exemplar), 'wb') as corpus_file:
            corpus_file.write(data['value'])


def write_weights(corpus_dir, test_name, corpus_data):
    """
    Take an exemplar_name to exemplar_weight dictionary and write it to a file
    in corpus directory.
    """
    with open(os.path.join(corpus_dir, test_name), 'w') as test_file:
        for exemplar, data in corpus_data.items():
            test_file.write('{} {}\n'.format(exemplar, data['weight']))


def parse_args():
    """
    Parse command-line arguments.
    """
    parser = argparse.ArgumentParser(description='Run MoonLight unit tests.')
    parser.add_argument('-m', '--moonlight-path', required=True,
                        help='Path to the MoonLight executable')

    return parser.parse_args()


def main():
    """
    The main function.
    """
    # Parse command-line arguments
    args = parse_args()

    tests_passed = False
    for test_path in glob.glob(os.path.join(UNIT_TESTS_DIR, '*.json')):
        test_name = os.path.basename(test_path)
        print('Running unit test "{}"...'.format(test_name), end=' ')

        # Read the test data
        with open(test_path, 'r') as test_file:
            test_data = json.load(test_file)

        # Discard non-positive weighted rows
        test_data['corpus'] = {exemplar: data for exemplar, data in test_data['corpus'].items()
                               if data['weight'] >= 0}

        # Format the test data dict and bulk it up with the required values
        value_lens = [len(data['value']) for data in test_data['corpus'].values()]
        test_data['num_basic_blocks'] = int(math.ceil(max(value_lens) / 8) * 8)
        test_data['corpus_size'] = len(test_data['corpus'])

        for exemplar, data in test_data['corpus'].items():
            test_data['corpus'][exemplar]['value'] = binary_data(data['value'])

        # Run MoonLight in the context of a temporary directory, which ensures
        # that all MoonLight-produced files are automatically cleaned up at the
        # end of each test.
        with TemporaryDirectory() as corpus_dir:
            # Write the test files to the temporary directory
            write_corpus(corpus_dir, test_data['corpus'])
            write_weights(corpus_dir, test_name, test_data['corpus'])

            if test_data['weighted']:
                weights_file = os.path.join(corpus_dir, test_name)
            else:
                weights_file = None

            # Run MoonLight
            moonlight_cmd = [args.moonlight_path, '-d', corpus_dir,
                             '-r', 'exemplar_', '-i']
            if test_data['algorithm'] == 'greedy':
                moonlight_cmd.append('-g')
            if weights_file:
                moonlight_cmd.extend(['-w', weights_file])

            results = run_moonlight(moonlight_cmd, corpus_dir, silent=True)
            if 'error' in results:
                print('MOONLIGHT ERROR: {}'.format(results['error']))
                sys.exit(1)

            # Compare the solutions to the expected test data
            tests_passed = check_results(results, test_data)

    # Set the return code
    sys.exit(not tests_passed)


if __name__ == '__main__':
    main()
