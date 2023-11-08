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
 * \author Shane Magrath, Liam Hayes, Jonathan Milford
 * \date Feb 2017
 *
 * \author Liam Hayes + Jonathan Milford
 * \date Feb 2017
 */

#include <set>

#include "Corpus.h"
#include "Matrix.h"
#include "OSCPSolver.h"

using namespace std;
using namespace boost::filesystem;
namespace src = boost::log::sources;

///////////////////////////////////////////////////////////////////////
// Constructors, Destructors, etc.
///////////////////////////////////////////////////////////////////////

OSCPSolver::OSCPSolver() : solution(Solution()) {
}

///////////////////////////////////////////////////////////////////////
// OSCP Methods
///////////////////////////////////////////////////////////////////////

INDEX_LIST OSCPSolver::calc_cols_to_ignore(const path &directory,
                                           const string &pattern,
                                           const path &weight_file) {
    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "CALC_COLS_TO_IGNORE()...";

    vector<CorpusFile> corpus = get_file_list(directory, pattern);
    sort(corpus.begin(), corpus.end(), greater<CorpusFile>());

    // don't need to sort by filesize, just need to know max filesize
    int num_files = corpus.size();
    assert(num_files > 0);
    int num_cols = 8 * corpus[0].file_size;

    // create a map from exemplar file names to weights
    bool weighted = false;
    map<string, double> weight_map;

    // empty path implies unweighted version
    if (!weight_file.empty()) {
        weighted = true;
        weight_map = get_weight_data(weight_file);
    }

    int num_unitarian = 0;
    BOOST_LOG(mylog) << "large_data = true";
    BOOST_LOG(mylog)
        << "Going to eliminate row unitarians before reading in all data";
    BOOST_LOG(mylog) << "";
    BOOST_LOG(mylog) << "Step 1: Counting column frequencies";
    vector<int> col_freq(num_cols, 0);

    for (int r = 0; r < num_files; r++) {
        path f = corpus[r].file_path;
        if ((r % 500) == 0) {
            BOOST_LOG(mylog) << "File: " << r << ", " << f.filename();
        }

        // ignore exemplars with invalid (non positive) weights
        if (weighted && weight_map[f.filename().string()] <= 0) {
            continue;
        }
        ROW row_data = get_exemplar_data(f);
        assert(num_cols >= (int) row_data.size());
        int rowsum = 0;
        for (unsigned int i = 0; i < row_data.size(); i++) {
            col_freq[i] += row_data[i];
            rowsum += row_data[i];
        }
    }

#if 0
    // print out rowsum and colsum distributions to help visualise the data
    vector<int> colsum_occurances = occurances(col_freq);
    for (int i = 0; i < colsum_occurances.size(); i++){
        if (colsum_occurances[i])
            BOOST_LOG(mylog) << "Colsum " << i << " Occurances " <<
    colsum_occurances[i];
    }
    vector<int> rowsum_occurances = occurances(rowsums);
    for (int i = 0; i < rowsum_occurances.size(); i++){
        if (rowsum_occurances[i])
            BOOST_LOG(mylog) << "Rowsum " << i << " Occurances " <<
    rowsum_occurances[i];
    }
#endif

    BOOST_LOG(mylog) << "";
    BOOST_LOG(mylog) << "Step 2: Recording all column singularities in the "
                     << "solution class";
    set<INDEX> cols_to_ignore;
    for (int i = 0; i < num_cols; i++) {
        if (col_freq[i] == 0) {
            cols_to_ignore.insert(i);
        }
    }

    int num_singularities = cols_to_ignore.size();
    this->solution.initial_singularities =
        INDEX_LIST(cols_to_ignore.begin(), cols_to_ignore.end());

    // note that ignoring column singularities at read in does not use less
    // memory since they are not present in a sparse representation anyway.
    // They have to be recorded in the solution class so the solution can be
    // verified later.
    // Since they are already found, may as well ignore them too

    BOOST_LOG(mylog) << "";
    BOOST_LOG(mylog) << "Step 3: Finding all row unitarians and ignoring "
                     << "their columns";
    BOOST_LOG(mylog) << "        and adding the rows and their weights to the "
                     << "solution.";
    for (int r = 0; r < num_files; r++) {
        path f = corpus[r].file_path;
        if ((r % 500) == 0) {
            BOOST_LOG(mylog) << "File: " << r << ", " << f.filename();
        }
        if (weighted && weight_map[f.filename().string()] <= 0) {
            continue;
        }
        ROW row_data = get_exemplar_data(f);
        assert(num_cols >= (int) row_data.size());
        bool unitarian = false;
        for (unsigned int i = 0; i < row_data.size(); i++) {
            if (row_data[i] == 1 && col_freq[i] == 1) {
                unitarian = true;
                num_unitarian++;
                string f_name = f.filename().string();
                this->solution.add_to_soln(f.filename(), row_data, 0.0, true);
                if (weighted) {
                    this->solution.weight += weight_map[f_name];
                } else {
                    this->solution.weight += 1.0;
                }
                break;
            }
        }
        if (unitarian) {
            for (unsigned int i = 0; i < row_data.size(); i++) {
                if (row_data[i]) {
                    cols_to_ignore.insert(i);
                }
            }
        }
    }
    BOOST_LOG(mylog) << "Row unitarians: " << num_unitarian;
    BOOST_LOG(mylog) << "Num cols (total): " << num_cols;
    BOOST_LOG(mylog) << "Num cols (singularities): " << num_singularities;
    BOOST_LOG(mylog) << "Num cols (covered by row unitarians): "
                     << cols_to_ignore.size() - num_singularities;
    BOOST_LOG(mylog) << "Num cols (remaining): "
                     << num_cols - cols_to_ignore.size();
    BOOST_LOG(mylog) << "List of columns to ignore now complete";
    BOOST_LOG(mylog) << "Ready to do full read in";
    BOOST_LOG(mylog) << "";

    return INDEX_LIST(cols_to_ignore.begin(), cols_to_ignore.end());
}

Solution OSCPSolver::solve_oscp(Matrix &data, CORPUS_DATA &corpus_data,
                                const string &name, bool greedy,
                                const path &weight_file) {
    src::severity_logger<> &mylog = my_logger::get();

    int r = data.get_num_rows();
    int c = data.get_num_cols();

    BOOST_LOG(mylog) << "SOLVE_OSCP()...";
    BOOST_LOG(mylog) << "STATS:  "
                     << "Data[" << r << ", " << c << "]";
    BOOST_LOG(mylog) << "";

    // container of file paths and meta data that constitute our solution
    Solution solution = this->solution;

    // initialise the solution with some meta-data
    if (greedy) {
        solution.scorelabel = "Greedy heuristic";
    } else {
        solution.scorelabel = "Milford-Hayes reduction";
    }

    solution.corpusname = name;
    solution.num_rows = data.get_num_rows();
    solution.num_columns = data.get_num_cols_orig();

    COLUMN_SUM init_freq = data.get_column_sum();
    INDEX_LIST singularities = get_singular_columns(init_freq);

    // remove column singularities - row singularities are harmless at this
    // stage
    eliminate_column_singularities(data, solution);
    r = data.get_num_rows();
    c = data.get_num_cols();

    // reduction options in order of priority:
    //  [0] row unitarians
    //  [1] row subsets
    //  [2] col supersets
    vector<bool> reduction_options = {true, true, true};
    int non_optimal = 0;
    while (r && c) {
        BOOST_LOG(mylog) << "STATS:  "
                         << "Matrix[" << r << ", " << c << "]";
        BOOST_LOG(mylog) << "STATS:  "
                         << "Soln size=" << solution.solution.size()
                         << " weight=" << solution.weight;
        BOOST_LOG(mylog) << "";

        if (!greedy && reduction_options[0]) {
            reduction_options[0] = false;
            if (eliminate_row_unitarians(data, solution, corpus_data)) {
                reduction_options[1] = true;
            }
        } else if (!greedy && reduction_options[1]) {
            reduction_options[1] = false;
            if (eliminate_subset_rows(data)) {
                reduction_options[0] = true;
                reduction_options[2] = true;
            }
        } else if (!greedy && reduction_options[2]) {
            reduction_options[2] = false;
            if (eliminate_superset_cols(data)) {
                reduction_options[1] = true;
            }
        } else {
            if (eliminate_max_score(data, solution, corpus_data)) {
                reduction_options[1] = true;
                non_optimal++;
            }
        }

        r = data.get_num_rows();
        c = data.get_num_cols();
    }

    BOOST_LOG(mylog) << "CHECKS: "
                     << "Finished reducing. Matrix[" << r << ", " << c << "]";
    BOOST_LOG(mylog) << "";

    print_solution(solution);

    BOOST_LOG(mylog) << "STATS:  "
                     << "Solution size: " << solution.solution.size();
    BOOST_LOG(mylog) << "STATS:  "
                     << "Solution weight: " << solution.weight;
    BOOST_LOG(mylog) << "STATS:  "
                     << "Non-optimal choices: " << non_optimal;

    bool verified = verify_solution(data, solution, weight_file);
    BOOST_LOG(mylog) << "CHECKS: "
                     << "Solution verified: " << verified;

    return solution;
}

///////////////////////////////////////////////////////////////////////
// Reduction Functions
///////////////////////////////////////////////////////////////////////

bool eliminate_row_unitarians(Matrix &data, Solution &solution,
                              CORPUS_DATA &corpus_data) {
    bool changed = false;
    if (data.get_num_rows() == 0 || data.get_num_cols() == 0) {
        return changed;
    }

    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "METHOD: "
                     << "row_unitarians";

    // there shouldn't be any column singularities (0 sum columns) if they were
    // removed at the start
    // can easily be checked here

    // find col unitarians
    COLUMN_SUM Freq = data.get_column_sum(); // frequency distribution
    int col = 0;
    INDEX_LIST unity_cols;

    for (int value : Freq) {
        if (value == 1) {
            unity_cols.push_back(col);
        }
        col++;
    }

    if (unity_cols.size() > 0) {
        changed = true;
        INDEX_LIST unity_rows =
            find_unitarian_rows(data, unity_cols, corpus_data);
        BOOST_LOG(mylog) << "INFO:   "
                         << "Data IS unitarian";
        BOOST_LOG(mylog) << "STATS:  "
                         << "Unitarian columns: " << unity_cols.size();
        BOOST_LOG(mylog) << "STATS:  "
                         << "Unitarian rows:    " << unity_rows.size();

        // add the unitarian rows to the solution we are building
        for (auto row : unity_rows) {
            add_to_solution(data, solution, row, true);
        }

        // remove rows and recalc Freq
        reduce(data, unity_rows);
    } else {
        BOOST_LOG(mylog) << "INFO:   "
                         << "Data is NOT unitarian";
    }

    BOOST_LOG(mylog) << "";

    return changed;
}

INDEX_LIST find_unitarian_rows(Matrix &data, INDEX_LIST &columns,
                               CORPUS_DATA &c_data) {
    src::severity_logger<> &mylog = my_logger::get();

    // use a set because the balance tree will be efficient for membership
    // testing
    BOOST_LOG(mylog) << "INFO:   "
                     << "Finding unitarian rows associated with "
                     << columns.size() << " columns";
    set<int> unitycols(columns.begin(), columns.end());

    // assert( columns are set of unitarian columns)
    INDEX_LIST rows;
    for (int r = 0; r < data.get_num_rows(); r++) {
        auto citer = data.column_begin(r);
        auto cend = data.column_end(r);
        for (int c = 0; citer != cend; c++, ++citer) {
            int value = (*citer);
            if (value != DELETED) {
                // check if the value is in the unitarian set
                if (unitycols.find(value) != unitycols.end()) {
                    rows.push_back(r);
                    c_data[r].score_unitarian +=
                        1.0; // update some corpus analytics
                    break;   // row only needs to have one unitarian column
                }
            }
        }

        if ((r % 100) == 99) { // progress report
            BOOST_LOG(mylog) << "LOOP:   "
                             << "Processed row " << r << " out of "
                             << data.get_num_rows();
        }
    }
    return rows;
}

bool eliminate_subset_rows(Matrix &data) {
    bool changed = false;
    src::severity_logger<> &mylog = my_logger::get();

    BOOST_LOG(mylog) << "METHOD: "
                     << "row_subsets";

    INDEX_LIST subset_rows = find_subset_rows(data);
    if (subset_rows.size() > 0) {
        changed = true;
        BOOST_LOG(mylog) << "INFO:   "
                         << "Eliminating " << subset_rows.size()
                         << " redundant rows";
        data.remove_rows(subset_rows);
    }

    BOOST_LOG(mylog) << "";

    return changed;
}

INDEX_LIST find_subset_rows(Matrix &data) {
    src::severity_logger<> &mylog = my_logger::get();

    int num_rows = data.get_num_rows();
    if (num_rows == 0) {
        return INDEX_LIST();
    }

    // indices of rows to remove (higher or equally weighted subset of other
    // rows)
    set<INDEX> del_set;

    // set of unique rows (for deduplicating the rows), maps the binary ROW to
    // row index in the matrix
    map<ROW, int> rows;
    int count_strict = 0; // stats

    // relevant meta data for the rows
    typedef struct row_meta {
        int index;
        int rowsum;
        double weight;
    } row_meta;

    // row comparison function
    //
    // true iff a should be before b in the sorted vector
    // sort first be rowsum (decreasing), then by weight (increasing) then
    // deterministically tiebreak (using filename)
    // rows are sorted such that if A preceeds B, then A can not be a
    // higher-weighted subset, so won't be removed
    auto cmp = [&data](row_meta a, row_meta b) {
        if (a.rowsum > b.rowsum) {
            return true;
        }
        if (a.rowsum < b.rowsum) {
            return false;
        }
        if (a.weight < b.weight) {
            return true;
        }
        if (a.weight > b.weight) {
            return false;
        }

        return (deterministic_compare(data, a.index, b.index) > 0);
    };

    // sort the rows
    ROW_SUM rowsums = data.get_row_sum();
    vector<row_meta> sorted_rows(rowsums.size());
    for (unsigned int i = 0; i < rowsums.size(); i++) {
        sorted_rows[i] = {(int) i, rowsums[i], data.get_row_weight(i)};
    }
    sort(sorted_rows.begin(), sorted_rows.end(), cmp);

#if 0
    // testing the sorting section
    for (int i = 0; i < sorted_rows.size() - 1; i++) {
        int sum1 = sorted_rows[i].rowsum;
        int sum2 = sorted_rows[i+1].rowsum;
        int w1 = sorted_rows[i].weight;
        int w2 = sorted_rows[i+1].weight;
        int index1 = sorted_rows[i].index;
        int index2 = sorted_rows[i+1].index;
        assert(index1 < num_rows);
        assert(index2 < num_rows);
        assert(sum1 == data.get_row_sum(index1));
        assert(sum2 == data.get_row_sum(index2));
        assert(w1 == data.get_row_weight(index1));
        assert(w2 == data.get_row_weight(index2));
        assert(sum1 >= sum2);
        if (sum1 == sum2)
            assert(w1 <= w2);
    }

    // print matrix visual to log
    if (data.get_num_rows() <= 100 && data.get_num_cols() <= 100) {
        for (auto it = sorted_rows.begin(); it != sorted_rows.end(); it++) {
            ROW row = data.get_row(it->index);
            string row_str = "";
            for (auto c = row.begin(); c != row.end(); c++) {
                if (*c) {
                    row_str += "@ ";
                } else {
                    row_str += ". ";
                }
            }
            row_str += "| index:" + to_string(it->index) + " sum:" +
    to_string(it->rowsum) + " w:" + to_string(it->weight);
            BOOST_LOG(mylog) << row_str;
        }
        BOOST_LOG(mylog) << "";
    }
#endif

    // first go through and remove deduplicate rows.
    // For corpus reduction problems there are many duplicate rows, hence this
    // is worthwhile
    int cur_rowsum = sorted_rows[0].rowsum;
    for (auto it = sorted_rows.begin(); it != sorted_rows.end(); it++) {
        if (cur_rowsum != it->rowsum) {
            // new (lower) rowsum value, so no row from now on can be a
            // duplicate of a previous one
            // so we can clear the memory intensive rows data structure
            cur_rowsum = it->rowsum;
            rows.clear();
        }

        ROW row = data.get_row(it->index);
        auto match = rows.find(row);
        if (match == rows.end()) {
            // not a duplicate, add it to the set of unique rows
            rows[row] = it->index;
        } else {
            // row is duplicate of a previous one since sorted, this row must
            // have higher or equal weight if higher weight, remove this new
            // one if equal weight, keep the earlier row when sorted
            // alphabetically by filename this keeps it nice and deterministic
            // and hence easy to test
            int match_index = match->second;
            if (it->weight == data.get_row_weight(match_index) &&
                deterministic_compare(data, match_index, it->index) > 0) {
                rows[row] = it->index;
                del_set.insert(match_index);
            } else {
                del_set.insert(it->index);
            }
        }
    }
    rows.clear();

    // now find pairs of rows such that:
    // 1. A has equal or higher rowsum than B, AND
    // 2. A has equal or lower weight than B
    // due to sorting, A is before B in sorted_rows
    for (auto it1 = sorted_rows.begin(); it1 != sorted_rows.end(); it1++) {
        if (del_set.find(it1->index) != del_set.end()) {
            continue;
        }

        for (auto it2 = it1 + 1; it2 != sorted_rows.end(); it2++) {
            if (del_set.find(it2->index) != del_set.end()) {
                continue;
            }

            if (it1->weight > it2->weight) {
                continue;
            }

            if (it2->rowsum == data.get_overlap(it1->index, it2->index)) {
                del_set.insert(it2->index);
                count_strict++;
            }
        }
    }

    BOOST_LOG(mylog) << "STATS:  " << num_rows << " <-- num of rows";
    BOOST_LOG(mylog) << "STATS:  " << count_strict
                     << " <-- num of strict subsets";
    BOOST_LOG(mylog) << "STATS:  " << del_set.size() - count_strict
                     << " <-- num of duplicate rows that are not subset of "
                     << "any other";
    BOOST_LOG(mylog) << "STATS:  " << num_rows - del_set.size()
                     << " <-- num of row remaining";
    INDEX_LIST result(del_set.begin(), del_set.end());

    return result;
}

bool eliminate_superset_cols(Matrix &data) {
    bool changed = false;
    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "METHOD: "
                     << "column_supersets";
    INDEX_LIST superset_cols = find_superset_cols(data);

    if (superset_cols.size() > 0) {
        changed = true;
        BOOST_LOG(mylog) << "INFO:   "
                         << "Eliminating " << superset_cols.size()
                         << " redundant cols";
        data.remove_cols(superset_cols);
    }

    BOOST_LOG(mylog) << "";

    return changed;
}

INDEX_LIST find_superset_cols(Matrix &data) {
    src::severity_logger<> &mylog = my_logger::get();
    int num_cols = data.get_num_cols();
    int num_rows = data.get_num_rows();

    BOOST_LOG(mylog) << "INFO:   "
                     << "Making local column-major sparse matrix";
    vector<vector<INDEX>> columns(num_cols, vector<int>());

    for (int r = 0; r < num_rows; r++) {
        auto c_iter = data.column_begin(r);
        auto c_end = data.column_end(r);
        while (c_iter != c_end) {
            int c = (*c_iter);
            if (c != DELETED) {
                columns[c].push_back(r);
            }
            advance(c_iter, 1);
        }
    }
    // we now have the matrix in column-major sparse form as a local variable
    // 'columns'
    BOOST_LOG(mylog) << "INFO:   "
                     << "Done. Now test for supersets";

    set<INDEX> supersets;
    int count_strict = 0;

    for (int c1 = 0; c1 < num_cols; c1++) {
        if (supersets.find(c1) != supersets.end()) {
            continue;
        }
        if (columns[c1].size() == 0) {
            continue;
        }

        // progress report
        if (c1 % 100 == 0) {
            BOOST_LOG(mylog) << "LOOP:   "
                             << "Checking col #" << c1;
        }

        for (int c2 = c1 + 1; c2 < num_cols; c2++) {
            // test if col1 is superset of col2 and visa versa
            if (supersets.find(c2) != supersets.end()) {
                continue;
            }

            if (columns[c2].size() == 0) {
                continue;
            }

            // these superset booleans will both remain true iff the two
            // columns are equal
            bool superset1 = true;
            bool superset2 = true;
            auto iter1 = columns[c1].begin();
            auto iter2 = columns[c2].begin();
            auto end1 = columns[c1].end();
            auto end2 = columns[c2].end();

            while (superset1 || superset2) {
                if (iter1 == end1) {
                    if (iter2 != end2) {
                        // col2 has something col1 doesn't (col2 hasn't
                        // finished iterating)
                        superset1 = false;
                    }
                    break;
                }
                if (iter2 == end2) {
                    // col1 has something col2 doesn't (col1 hasn't finished
                    // interating)
                    superset2 = false;
                    break;
                }

                if (*iter1 < *iter2) {
                    // col1 has something col2 doesn't
                    superset2 = false;
                    advance(iter1, 1);
                } else if (*iter1 > *iter2) {
                    // col2 has something col1 doesn't
                    superset1 = false;
                    advance(iter2, 1);
                } else {
                    advance(iter1, 1);
                    advance(iter2, 1);
                }
            }

            // superset1 has priority
            // if two cols are equal, it gets rid of lower indexed
            // the soln set is still deterministic, since soln set is of rows
            if (superset1) {
                supersets.insert(c1);
                if (!superset2) {
                    count_strict++;
                }
                break;
            } else if (superset2) {
                supersets.insert(c2);
                count_strict++;
            }
        }
    }

    BOOST_LOG(mylog) << "STATS:  " << num_cols << " <-- num of cols";
    BOOST_LOG(mylog) << "STATS:  " << count_strict
                     << " <-- num of strict supersets";
    BOOST_LOG(mylog) << "STATS:  " << supersets.size() - count_strict
                     << " <-- num of cols equal to another row and not "
                        "superset of any other";
    BOOST_LOG(mylog) << "STATS:  " << num_cols - supersets.size()
                     << " <-- num of cols remaining";

    INDEX_LIST result(supersets.begin(), supersets.end());

    return result;
}

bool eliminate_max_score(Matrix &data, Solution &solution,
                         CORPUS_DATA corpus_data) {
    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "METHOD: "
                     << "heuristic (single greedy select)";

    MEASURE row_scores = score_rows(data);
    vector<pair<double, int>> sorted_scores(row_scores.size());

    for (unsigned int i = 0; i < row_scores.size(); i++) {
        sorted_scores[i] = {row_scores[i], i};
    }

    sort(sorted_scores.begin(), sorted_scores.end(),
         greater<pair<double, int>>());
    print_row_scores(data, sorted_scores);

    int rank = deterministic_select(data, sorted_scores);
    int row = sorted_scores[rank].second;

    INDEX_LIST rowstodelete;
    if (row == NULL_INDEX) {
        BOOST_LOG(mylog) << "INFO:   "
                         << "No max rowsum found";
        BOOST_LOG(mylog) << "";
        return false;
    }

    double maxscore = sorted_scores[rank].first;
    BOOST_LOG(mylog) << "INFO:   "
                     << "Choosing score, row: " << maxscore << ", " << row;
    add_to_solution(data, solution, row, false);
    rowstodelete.push_back(row);
    reduce(data, rowstodelete);
    BOOST_LOG(mylog) << "";

    return true;
}

///////////////////////////////////////////////////////////////////////
// Heuristic Functions
///////////////////////////////////////////////////////////////////////

int deterministic_select(Matrix &data, const vector<SCORE_ROW> &scores) {
    if (scores.size() == 0) {
        return NULL_INDEX;
    }

    int bestindex = 0;
    unsigned int i = 1;

    // tie break by taking first exemplar when ordered alphabetically by
    // filename
    while (i < scores.size() && scores[i].first == scores[0].first) {
        if (deterministic_compare(data, scores[bestindex].second,
                                  scores[i].second) > 0) {
            bestindex = i;
        }
        i++;
    }

    return bestindex;
}

int deterministic_compare(Matrix &data, int row1, int row2) {
    // take two row IDs, return a comparison based on their file names (which
    // doesn't change)
    // +ve for exemplar1 > exemplar2 in alphabet
    string path1 = data.get_row_exemplar(row1).string();
    string path2 = data.get_row_exemplar(row2).string();

    return path1.compare(path2);
}

MEASURE score_rows(Matrix &data) {
    int rows = data.get_num_rows();
    MEASURE scores(rows);                 // vector of row scores
    ROW_SUM rowsums = data.get_row_sum(); // vector of row sums

    // calculate scores
    for (int r = 0; r < rows; r++) {
        scores[r] = rowsums[r] / data.get_row_weight(r);
    }

    return scores;
}

///////////////////////////////////////////////////////////////////////
// Generally Useful Functions
///////////////////////////////////////////////////////////////////////

void add_to_solution(Matrix &data, Solution &S, int row, bool optimal) {
    path fullpath = data.get_row_exemplar(row);
    path exemplar = fullpath.filename();
    ROW rowdata = get_exemplar_data(fullpath);
    double weight = data.get_row_weight(row);
    S.add_to_soln(exemplar, rowdata, weight, optimal);

    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "INFO:   "
                     << "Row #" << row << " added to soln. " << exemplar;
}

///////////////////////////////////////////////////////////////////////
// Singularity Clean Up
///////////////////////////////////////////////////////////////////////

bool eliminate_column_singularities(Matrix &data, Solution &solution) {
    // column singularities can exist in the initial data for a range of
    // reasons. We simply just need to remove them so they don't poison our
    // computation
    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "METHOD: "
                     << "column singularities";
    bool changed = false;
    int num_singularities = 0;
    COLUMN_SUM freq = data.get_column_sum(); // frequency distribution

    if (is_column_singular(freq)) {
        changed = true;
        INDEX_LIST singularities = get_singular_columns(freq);
        // dangerous assumption here...
        // assumes that if Solution.initial_singularities is non empty then:
        // a) largedata flag was used
        // b) Solution.initial_singularities contains indices of ALL initial col
        // singularities
        if (solution.initial_singularities.size() == 0) {
            solution.initial_singularities = singularities;
        } else {
            BOOST_LOG(mylog) << "INFO:   "
                             << "Indicies of column "
                             << "singularities already recorded";
        }

        num_singularities = singularities.size();
        BOOST_LOG(mylog) << "STATS:  "
                         << "Data has " << num_singularities
                         << " column singularities.";
        data.remove_cols(singularities);
        BOOST_LOG(mylog) << "INFO:   "
                         << "Singularities removed...";
        // need to recompute this now that the number of columns is different
    }

    BOOST_LOG(mylog) << "";

    return changed;
}

void eliminate_row_singularities(Matrix &data, const ROW_SUM &rowsum) {
    // make sure the data is singular
    if (is_row_singular(rowsum)) {
        INDEX_LIST singularities = get_singular_rows(rowsum);
        data.remove_rows(singularities);
    }
}

bool is_column_singular(const COLUMN_SUM &freq) {
    int size = freq.size();

    for (int col = 0; col < size; col++) {
        if (freq[col] == 0) {
            return true;
        }
    }

    return false;
}

bool is_row_singular(const ROW_SUM &rowsum) {
    int size = rowsum.size();

    for (int row = 0; row < size; row++) {
        if (rowsum[row] == 0) {
            return true;
        }
    }

    return false;
}

INDEX_LIST get_singular_columns(const COLUMN_SUM &freq) {
    INDEX_LIST singularities;

    for (unsigned int c = 0; c < freq.size(); c++) {
        if (freq[c] == 0) {
            // found: add the subscript to the vector
            singularities.push_back(c);
        }
    }

    return singularities;
}

INDEX_LIST get_singular_rows(const ROW_SUM &rowsum) {
    INDEX_LIST singularities;

    for (unsigned int r = 0; r < rowsum.size(); r++) {
        if (rowsum[r] == 0) {
            // found: add the subscript to the vector
            singularities.push_back(r);
        }
    }

    return singularities;
}

///////////////////////////////////////////////////////////////////////
// Reduce Matrix, Delete Rowset and All Their Columns
///////////////////////////////////////////////////////////////////////

void reduce(Matrix &data, INDEX_LIST &rowset) {
    src::severity_logger<> &mylog = my_logger::get();
    // get the list of columns we need to delete
    INDEX_LIST cols = project_columns(data, rowset);
    // deleting columns does not impact on row indices.
    int delta = cols.size();
    int cols_before = data.get_num_cols();
    int cols_after = cols_before - delta;
    double reduction = 100.0 * ((1.0 * delta) / (1.0 * cols_before));

    BOOST_LOG(mylog) << "STATS:  "
                     << "Removing " << delta << " columns (" << reduction
                     << "% of remaining)";
    BOOST_LOG(mylog) << "STATS:  "
                     << "Number of columns remaining : " << cols_after;

    data.remove_cols(cols);
    data.remove_rows(rowset);

    // we may now have row singularities which we need to remove...
    ROW_SUM rowsum = data.get_row_sum();
    if (is_row_singular(rowsum)) {
        BOOST_LOG(mylog) << "INFO:   "
                         << "We now have row singularities. ";
        eliminate_row_singularities(data, rowsum);
    }
}

INDEX_LIST project_columns(Matrix &data, const INDEX_LIST &rowset) {
    int cols = data.get_num_cols();
    INDEX_LIST C;

    for (auto r : rowset) {
        ROW rowdata;
        rowdata = data.get_row(r);
        for (int c = 0; c < cols; c++) {
            if (rowdata[c] == 1) {
                C.push_back(c);
            }
        }
    }

    // de-duplicate the columns
    C = dedup(C);

    return C;
}

///////////////////////////////////////////////////////////////////////
// Post Solution Checks
///////////////////////////////////////////////////////////////////////

COLUMN_SUM calc_soln_col_sum(Matrix &data, Solution &S) {
    // calculates the column sum for \all the columns from the subset of rows
    // given. could generalise Matrix::get_col_sum() to also do this
    int cols = data.get_num_cols_orig();
    COLUMN_SUM colsum(cols, 0);

    for (unsigned int i = 0; i < S.rowdata.size(); i++) {
        ROW row = S.rowdata[i];
        for (unsigned int c = 0; c < row.size(); c++) {
            if (row[c] == 1) {
                colsum[c] += 1;
            }
        }
    }

    return colsum;
}

bool verify_solution(Matrix &data, Solution &S, const path &weight_file) {
    // this function assumes that any column singularities have been a priori
    // removed from the data...

    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "INFO:   "
                     << "Init singularities: "
                     << S.initial_singularities.size();
    BOOST_LOG(mylog) << "INFO:   "
                     << "Rows to verify: " << S.solution.size();

    COLUMN_SUM colsum = calc_soln_col_sum(data, S);

    // look for any columns that have zero frequency and weren't in the
    // original record of column singularities
    sort(S.initial_singularities.begin(), S.initial_singularities.end());

    for (unsigned int c = 0; c < colsum.size(); c++) {
        if ((colsum[c] == 0) &&
            (!binary_search(S.initial_singularities.begin(),
                            S.initial_singularities.end(), c))) {
            BOOST_LOG(mylog) << "INFO:   "
                             << "Column " << c << " not covered!";
            return false;
        }
    }

    // verify the total weight if weighted version (weight file is not a empty
    // path)
    if (!weight_file.empty()) {
        double weight = 0;
        map<string, double> weight_map = get_weight_data(weight_file);
        for (unsigned int i = 0; i < S.solution.size(); i++) {
            weight += weight_map[S.solution[i].filename().string()];
        }

        if (weight != S.weight) {
            BOOST_LOG(mylog) << "Solution has inconsistent weight!";
            return false;
        }
    } else if (S.weight != S.solution.size()) {
        // unweighted version: every row has weight 1.0, so ensure soln weight
        // == soln size
        BOOST_LOG(mylog) << "Solution size doesn't equal solution weight!";

        return false;
    }

    BOOST_LOG(mylog) << "";

    return true;
}

vector<int> primality_check(Matrix &data, Solution &S) {
    // this function assumes that any column singularities have been a priori
    // removed from the data, and the solution has been verified could also
    // verify solution at the same time
    src::severity_logger<> &mylog = my_logger::get();

    int rows = S.solution.size();
    int cols = data.get_num_cols_orig();
    COLUMN_SUM colsum = calc_soln_col_sum(data, S);

    vector<int> result;

    // r is index in solution vector
    for (int r = 0; r < rows; r++) {
        ROW row = S.rowdata[r];
        bool unnecessary = true;

        for (int c = 0; c < cols; c++) {
            if (row[c] == 1 && colsum[c] == 1) {
                unnecessary = false;
            }
        }

        if (unnecessary) {
            for (int c = 0; c < cols; c++) {
                if (row[c] == 1)
                    colsum[c] -= 1;
            }

            BOOST_LOG(mylog) << S.solution[r] << " unnecessary";
            result.push_back(r);
        }
    }
    BOOST_LOG(mylog) << "Primality: " << result.size() << " unnecessary row(s)";

    return result;
}

///////////////////////////////////////////////////////////////////////
// Print Functions
///////////////////////////////////////////////////////////////////////

void print_matrix_to_log(Matrix &data) {
    src::severity_logger<> &mylog = my_logger::get();

    for (int i = 0; i < data.get_num_rows(); i++) {
        ROW row = data.get_row(i);
        string row_str = "";

        for (unsigned int j = 0; j < row.size(); j++) {
            if (row[j]) {
                row_str += "@ ";
            } else {
                row_str += ". ";
            }
        }

        row_str +=
            "| #" + to_string(i) + " w=" + to_string(data.get_row_weight(i));
        BOOST_LOG(mylog) << row_str;
    }

    BOOST_LOG(mylog) << "";
}

void print_row_scores(Matrix &data, const vector<SCORE_ROW> &sorted_scores) {
    // print first few rows and their scores and file names
    src::severity_logger<> &mylog = my_logger::get();

    for (unsigned int i = 0; i < 5 && i < sorted_scores.size(); i++) {
        if (i > 0 && sorted_scores[0].first - sorted_scores[i].first > 0.5) {
            break;
        }

        int r = sorted_scores[i].second;
        path fullpath = data.get_row_exemplar(r);
        path exemplar = fullpath.filename();
        BOOST_LOG(mylog) << "INFO:   "
                         << "Score, row: " << sorted_scores[i].first << ", "
                         << sorted_scores[i].second << ", " << exemplar;
    }
}

void print_solution(Solution &solution) {
    src::severity_logger<> &mylog = my_logger::get();
    sort(solution.solution.begin(), solution.solution.end());

    for (unsigned int i = 0; i < solution.solution.size(); i++) {
        BOOST_LOG(mylog) << "SOLN:   " << solution.solution[i];
    }

    BOOST_LOG(mylog) << "";
}

///////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////

vector<int> occurances(const vector<int> &values) {
    int max = *max_element(values.begin(), values.end());
    vector<int> result(max, 0);

    for (unsigned int i = 0; i < values.size(); i++) {
        if (values[i] >= 0)
            result[values[i]]++;
    }

    return result;
}

INDEX_LIST dedup(const INDEX_LIST &indices) {
    set<int> dedup(indices.begin(), indices.end());
    INDEX_LIST result = INDEX_LIST(dedup.begin(), dedup.end());

    return result;
}

///////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////

INDEX_LIST brute_force(Matrix &data, int best_solution) {
    INDEX_LIST result;
    int n = data.get_num_rows();
    int m = data.get_num_cols();
    vector<bool> seen;
    bool achieved = false;
    vector<bool> v(n);

    // we try solution sizes starting at 1
    for (int r = 1; r < best_solution; r++) {
        fill(v.end() - r, v.end(), true);

        // We permute the bit vector v(n) to generate combinations of rows with
        // r bits set.
        do {
            seen.assign(m, false);

            for (int i = 0; i < n; ++i) {
                if (v[i]) {
                    ROW row = data.get_row(i);

                    for (int j = 0; j < (int) row.size(); j++) {
                        int x = row[j];
                        if (x == 1) {
                            seen[j] = true;
                        }
                    }
                }
            }

            // Check to see if this combination provides a cover.
            bool done = true;
            for (bool s : seen) {
                if (!s) {
                    done = false;
                }
            }

            if (done) {
                achieved = true;
                break;
            }
        } while (next_permutation(v.begin(), v.end()));
    }

    if (!achieved) {
        return result;
    } else {
        for (int i = 0; i < n; ++i) {
            if (v[i]) {
                result.push_back(i);
            }
        }

        return result;
    }
}
