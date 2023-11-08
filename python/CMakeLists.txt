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

find_program(PYTHON NAMES python python2.7 python3.5)
if(NOT PYTHON)
    message(SEND_ERROR "Python not found, unable to run unit tests")
endif()

file(COPY . DESTINATION .
     FILES_MATCHING PATTERN "*.py"
                    PATTERN "unit_tests/*.json"
                    PATTERN "benchmark_results/*.json")

add_custom_target(unit_tests
                  COMMAND ${PYTHON} run_unit_tests.py -m ${CMAKE_BINARY_DIR}/src/moonlight
                  DEPENDS run_unit_tests.py unit_tests
                  COMMENT "Running unit tests")

add_custom_target(benchmarks
                  COMMAND ${PYTHON} run_benchmarks.py -m ${CMAKE_BINARY_DIR}/src/moonlight
                  DEPENDS run_benchmarks.py benchmark_results
                  COMMENT "Running benchmarks")
