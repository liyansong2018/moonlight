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

#include <sstream>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include "Solution.h"

using namespace std;
using namespace boost::filesystem;
namespace pt = boost::property_tree;

Solution::Solution()
    : corpusname(""), scorelabel(""), scoresum(0.0), num_columns(0),
      num_rows(0), weight(0.0), solution(vector<path>()),
      rowdata(vector<ROW>()), scores(vector<double>()),
      initial_singularities(INDEX_LIST()), num_non_optimal(0),
      weight_non_optimal(0.0) {
}

Solution::Solution(const Solution &orig) {
    this->corpusname = orig.corpusname;
    this->scorelabel = orig.scorelabel;
    this->scoresum = orig.scoresum;
    this->num_columns = orig.num_columns;
    this->num_rows = orig.num_rows;
    this->rowdata = orig.rowdata;
    this->scores = orig.scores;
    this->initial_singularities = orig.initial_singularities;
    this->solution = orig.solution;
    this->weight = orig.weight;
    this->num_non_optimal = orig.num_non_optimal;
    this->weight_non_optimal = orig.weight_non_optimal;
}

void Solution::json_print(const path &fpath) const {
    pt::ptree tree;
    pt::ptree solution_exemplars;

    tree.put("corpus", corpusname);
    tree.put("corpus_size", num_rows);
    tree.put("solution_size", solution.size());
    tree.put("solution_weight", weight);
    tree.put("num_basic_blocks", num_columns);
    tree.put("initial_singularities", initial_singularities.size());
    tree.put("num_non_optimal", num_non_optimal);
    tree.put("weight_non_optimal", weight_non_optimal);
    tree.put("score_label", scorelabel);

    for (auto exemplar : solution) {
        pt::ptree exemplar_elem;

        exemplar_elem.put("", exemplar.native());
        solution_exemplars.push_back(make_pair("", exemplar_elem));
    }

    tree.add_child("solution", solution_exemplars);

    pt::write_json(fpath.native(), tree);
}

void Solution::remove_from_soln(vector<int> rows) {
    // sort in descending order. This ordering will preserve the integrity of
    // the row indices in the index list - that is they will be correct after
    // we delete rows with higher index values
    sort(rows.begin(), rows.end(), std::greater<int>());

    for (int row : rows) {
        this->solution.erase(this->solution.begin() + row);
        this->rowdata.erase(this->rowdata.begin() + row);
        this->scores.erase(this->scores.begin() + row);
    }
}

void Solution::add_to_soln(const path &f, ROW row, double weight,
                           bool optimal) {
    this->weight += weight;
    this->solution.push_back(f);
    this->rowdata.push_back(row);
    this->scores.push_back(0);

    if (!optimal) {
        num_non_optimal++;
        weight_non_optimal += weight;
    }
}
