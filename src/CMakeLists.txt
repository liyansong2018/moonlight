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

add_executable(moonlight Corpus.cpp
                         ExemplarData.cpp
                         Matrix.cpp
                         moonlight.cpp
                         OSCPSolver.cpp
                         Solution.cpp)
target_link_libraries(moonlight ${Boost_LIBRARIES} pthread)

install(TARGETS moonlight RUNTIME DESTINATION bin)
