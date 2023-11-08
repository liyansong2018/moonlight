/*
 * Copyright 2017 The Australian National University
 *
 * This software is the result of a joint project between the Defence Science
 * and Technology Group and the Australian National University. It was enabled
 * as part of a Next Generation Technology Fund grant:
 * see https://www.dst.defence.gov.au/nextgentechfund
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

/**
 * \file
 *
 * \author Shane Magrath
 * \date March 12, 2015
 *
 * \author Liam Hayes + Jonathan Milford
 * \date Feb 2017
 */

#ifndef SOLUTION_H
#define SOLUTION_H

#include <vector>

#include <boost/filesystem/path.hpp>
#include <boost/serialization/vector.hpp>

#include "moonlight.h"

class Solution {
    ///////////////////////////////////////////////////////////////////////
    // Serialisation
    ///////////////////////////////////////////////////////////////////////

    // we do all this in the header to avoid some compilation issues.
    friend class boost::serialization::access;
    friend std::ostream &operator<<(std::ostream &os, const Solution &element);

    template <class Archive>
    void save(Archive &ar, unsigned int version) const {
        // we don't serialise 'path' objects but the string representation
        std::vector<std::string> sols;
        for (auto sol : solution) {
            std::string fname = sol.native();
            sols.push_back(fname);
        }

        ar &corpusname;
        ar &scorelabel;
        ar &scoresum;
        ar &num_columns;
        ar &num_rows;
        ar &weight;
        ar &sols;
        ar &rowdata;
        ar &initial_singularities;
        ar &num_non_optimal;
        ar &weight_non_optimal;
    }

    template <class Archive> void load(Archive &ar, unsigned int version) {
        // we don't serialise 'path' objects but the string representation
        std::vector<std::string> sols;

        ar &corpusname;
        ar &scorelabel;
        ar &scoresum;
        ar &num_columns;
        ar &num_rows;
        ar &weight;
        ar &sols;
        ar &rowdata;
        ar &initial_singularities;
        ar &num_non_optimal;
        ar &weight_non_optimal;

        for (auto sol : sols) {
            boost::filesystem::path f = boost::filesystem::path(sol);
            solution.push_back(f);
        }
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

public:
    Solution();
    Solution(const Solution &orig);

    /** Write the solution to a JSON file */
    void json_print(const boost::filesystem::path &fpath) const;

    void remove_from_soln(std::vector<int> rows);

    void add_to_soln(const boost::filesystem::path &f, ROW row, double weight,
                     bool optimal);

    std::string corpusname;
    std::string scorelabel;

    /** Sum of the scores */
    double scoresum;

    /** Number of columns in raw matrix */
    int num_columns;

    /** Number of rows in raw matrix */
    int num_rows;

    /** Total weight of the exemplars in the solution **/
    double weight;

    /** Each element is a file path that is in the solution set */
    std::vector<boost::filesystem::path> solution;

    /** Raw row data */
    std::vector<ROW> rowdata;

    /** Score of each exemplar in the solution. Currently unused */
    MEASURE scores;

    /** Record of the initial singularities in the solution */
    INDEX_LIST initial_singularities;

    /**
     * Number of heuristic reductions, number of rows in the solution that are
     * non optimal
     */
    int num_non_optimal;

    /** Total weight of all non optimal rows */
    double weight_non_optimal;
};

BOOST_CLASS_VERSION(Solution, 0)

#endif /* SOLUTION_H */
