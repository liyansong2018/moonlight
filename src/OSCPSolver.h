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
 * \date February 13, 2015
 *
 * \author Liam Hayes + Jonathan Milford
 * \date Feb 2017
 */

#ifndef OSCP_SOLVER_H
#define OSCP_SOLVER_H

#include <vector>

#include "ExemplarData.h"
#include "Solution.h"
#include "moonlight.h"

class Matrix;

using EDGE = std::pair<int, int>;
using WEIGHT = double;
using SCORE_ROW = std::pair<double, int>;

#define NULL_INDEX -1

///////////////////////////////////////////////////////////////////////
// Reduction Functions
///////////////////////////////////////////////////////////////////////

/**
 * \brief Remove rows which are unitarian - ie uniquely cover a column
 *
 * \param data Corpus data to remove rows from
 * \param solution contains solution so far, and has a row added to it
 * \param corpus_data Contains statistics etc
 * \return whether or not the matrix was modified
 */
bool eliminate_row_unitarians(Matrix &data, Solution &solution,
                              CORPUS_DATA &corpus_data);

/**
 * \brief Given a data matrix and a set of unitarian columns find the
 * associated row indices that gave us those unities.
 *
 * We also provide a CORPUS_DATA object that contains and records our corpus
 * analytics. This object is updated as a side-effect to the main calculation.
 * Not good separation of concerns but the performance issues here a first
 * order concern.
 *
 * \param data Corpus data provider that contains the data matrix
 * \param columns set of column indices that are unitarian
 * \param corpus_data Container object of corpus data analytics
 * \return set of unitarian rows. Guaranteed not to have duplicates.
 * \throws out_of_range exception if the index is invalid.
 * \todo do we assume that given column is unitarian?
 */
INDEX_LIST find_unitarian_rows(Matrix &data, INDEX_LIST &columns,
                               CORPUS_DATA &c_data);

/**
 * \brief Find rows that have equal or higher weight to a superset row
 *
 * \param data Matrix data to serach for subset rows
 * \return a list or row indices to eliminate
 */
INDEX_LIST find_subset_rows(Matrix &data);

/**
 * \brief Remove rows which are subset of other rows
 *
 * \param data Corpus data to remove rows from
 * \return whether or not the matrix was modified
 */
bool eliminate_subset_rows(Matrix &data);

/**
 * \brief Remove columns which are supersets of other column
 *
 * \param data Corpus data to remove columns from
 * \return whether or not the matrix was modified
 */
bool eliminate_superset_cols(Matrix &data);

/**
 * \brief Find columns which are supersets of other column
 *
 * \param data Corpus data to search for super set columns
 * \return a list of columns to eliminate
 */
INDEX_LIST find_superset_cols(Matrix &data);

/**
 * \brief Choose a row with maximum row sum and add it to the solution
 *
 * \param data contains matrix data
 * \param solution contains solution so far, and has a row added to it
 * \param corpus_data Contains statistics etc
 * \return whether or not the matrix was modified
 */
bool eliminate_max_score(Matrix &data, Solution &solution,
                         CORPUS_DATA corpus_data);

///////////////////////////////////////////////////////////////////////
// Heuristic Selection Functions
///////////////////////////////////////////////////////////////////////

/**
 * \brief Provides a deterministic Heuristic selection from the matrix
 *
 * \param data Corpus data provider
 * \param scores list of row scores used to make the heuristic choice
 * \return index of selected row in scores vector
 */
int deterministic_select(Matrix &data, const std::vector<SCORE_ROW> &scores);

/**
 * \brief Breaks ties between two rows alphabetically
 *
 * \param data Corpus data provider
 * \param row1 first row to check
 * \param row2 second row to check
 * \return compare value
 */
int deterministic_compare(Matrix &data, int row1, int row2);

/**
 * \brief Data is column singular if there exists at least one column whose
 * sum is zero.
 *
 * \param freq the joint frequency distribution to test
 * \return true iff there exists an element in the vector that is zero
 */
bool is_column_singular(const COLUMN_SUM &freq);

/**
 * \brief Data is row singular if there exists at least one row whose sum is
 * zero.
 *
 * \param rowsum sum of each row
 * \return true iff there exists an element in the vector that is zero
 */
bool is_row_singular(const ROW_SUM &rowsum);

/**
 * \brief Record the column indices of any singularities.
 *
 * \param freq the joint frequency distribution of data
 * \return list of column indices identifying singular columns
 */
INDEX_LIST get_singular_columns(const COLUMN_SUM &freq);

/**
 * \brief Record the row indices of any row singularities.
 *
 * \param rowsum sum of each row
 * \return list of row indices identifying singular rows
 */
INDEX_LIST get_singular_rows(const ROW_SUM &rowsum);

///////////////////////////////////////////////////////////////////////
// Generally Useful Functions
///////////////////////////////////////////////////////////////////////

/**
 * \brief Calculate the row sum score for all the rows in the data matrix.
 *
 * We also provide a CORPUS_DATA object that contains and records our corpus
 * analytics. This object is updated as a side-effect to the main calculation.
 * Not good separation of concerns but the performance issues here a first
 * order concern.
 *
 * \param data corpus data provider that contains the data matrix
 * \param corpus_data Container object of corpus data analytics
 * \returns row sum measure for every row in the data matrix
 */
MEASURE score_rows(Matrix &data);

/**
 * \brief add a row to the solution
 *
 * This involves reading the original row data from file Matrix.orig_num_rows
 * and Row_Elem.fname are useful for this
 */
void add_to_solution(Matrix &data, Solution &S, int row, bool optimal);

///////////////////////////////////////////////////////////////////////
// Singularity Clean Up
///////////////////////////////////////////////////////////////////////

/**
 * \brief Method to remove any column singularities in the data matrix.
 *
 * It does so by deleting those columns whose sum is zero as signaled by
 * the joint frequency distribution. Any column indices in use are undefined
 * after this operation. Moreover any associated column data is invalidated
 * such as the joint frequency distribution.
 *
 * \param data corpus data provider whose singularities will be eliminated
 * \param solution contains solution so far
 */
bool eliminate_column_singularities(Matrix &data, Solution &solution);

/**
 * \brief Method to remove any row singularities in the data matrix.
 *
 * It does so by deleting those rows whose sum is zero.
 * Any row indices in use are undefined after this operation.
 *
 * \param data Corpus data provider whose singularities will be eliminated
 * \param rowsum the joint frequency distribution of data
 */
void eliminate_row_singularities(Matrix &data, const ROW_SUM &rowsum);

///////////////////////////////////////////////////////////////////////
// Manipulate Matrix
///////////////////////////////////////////////////////////////////////

/**
 * \brief Compute the reduction of a data matrix given a set of row indices.
 *
 * A reduction is the process of carving out a sub-matrix in the data matrix
 * by deleting all the rows in the row set and all the columns in the
 * corresponding column projection.
 *
 * \param data corpus data provider
 * \param rowset list of row indices for the reduction
 */
void reduce(Matrix &data, INDEX_LIST &rowset);

/**
 * \brief deduplicate
 *
 * \param indices, the indexlist to dedup
 * \return
 */
INDEX_LIST dedup(const INDEX_LIST &indices);

/**
 * \brief Compute the column projection of the data matrix given a list of
 * rows indices.
 *
 * A column projection for a row set is a list of __ALL the column indices__
 * formed where [row, column] is one for all the rows in the row set.
 *
 * \param data corpus data provider
 * \param rowset list of row indices that we use to induce a column projection
 * \return a list of column indices that forms the projection
 */
INDEX_LIST project_columns(Matrix &data, const INDEX_LIST &rowset);

///////////////////////////////////////////////////////////////////////
// Post Solution Checks
///////////////////////////////////////////////////////////////////////

/**
 * \brief take all the rows selected for solution. calculate the sum of each
 * column. It is used to verify the solution, check that all columns of the
 * original matrix (except initial_singularities) are covered.
 *
 * \param data, the matrix (reference)
 * \param S, the solution (reference)
 * \return a column sum
 */
COLUMN_SUM calc_soln_col_sum(Matrix &data, Solution &S);

/**
 * \brief Verifies that a solution is a cover
 *
 * \param data corpus data provider
 * \param S given solution
 * \return boolean representing if a solution is a cover
 */
bool verify_solution(Matrix &data, Solution &S,
                     const boost::filesystem::path &weight_file);

/**
 * \brief Find unnecessary rows of a solution. The rows are checked in the
 * order they were added to the solution. Fairly effective at enhancing a
 * Greedy solution.
 *
 * \param data corpus data provider
 * \param S given solution
 * \return a list of row indices that are unnecessarry
 */
std::vector<int> primality_check(Matrix &data, Solution &S);

///////////////////////////////////////////////////////////////////////
// Print Functions
///////////////////////////////////////////////////////////////////////

/**
 * \brief current state of matrix to log
 *
 * Useful for debugging
 * \param data
 */
void print_matrix_to_log(Matrix &data);

/**
 * \brief print the highest scoring rows and their scores to log
 *
 * \param data the matrix
 * \param sorted_scores sorted vector of pair<double score, int row_index>
 */
void print_row_scores(Matrix &data,
                      const std::vector<SCORE_ROW> &sorted_scores);

/**
 * \brief prints solution file names to the log.
 *
 * \param solution
 */
void print_solution(Solution &solution);

///////////////////////////////////////////////////////////////////////
// Other
///////////////////////////////////////////////////////////////////////

/**
 * \brief Exhaustively searches for any solution better than best given
 *
 * It is a VERY EXPENSIVE function - it takes O(m*2^n) time, where n is the
 * number of rows in the Matrix, and m is the number of columns.
 *
 * \param data corpus data provider
 * \param best_solution is the size of the solution it is trying to beat
 * \return a list of rows which make up the best solution. If it doesn't find
 *         anything, this is empty.
 * \todo ensure that it starts searching for solutions at the lower bound
 * of possible optimal solutions
 */
INDEX_LIST brute_force(Matrix &data, int best_solution);

/**
 * Returns number of occurances of each value in values vector
 */
std::vector<int> occurances(const std::vector<int> &values);

///////////////////////////////////////////////////////////////////////
// Solver Class
///////////////////////////////////////////////////////////////////////

/**
 * This class is both a container of meta-data about the corpus and an
 * interface to finding a distillation of the corpus.
 */
class OSCPSolver {
public:
    /**
     * \brief Useful constructor.
     *
     * \param corpus_data Corpus data source.
     */
    OSCPSolver();

    ///////////////////////////////////////////////////////////////////////
    // API
    ///////////////////////////////////////////////////////////////////////

    /**
     * \brief Finds columns singularities and columns covered by row unitarians
     * without loading entire matrix into memory.
     *
     * Typically (when using code coverage data) this eliminates large
     * proportion of all the non-empty rows to decrease data size.
     *
     * \param directory the folder to look for exemplars in
     * \param pattern find exemplar names matching this pattern
     * \param weight_file path to weight file
     * \return list of column indices to ignore when you read in again
     */
    INDEX_LIST calc_cols_to_ignore(const boost::filesystem::path &directory,
                                   const std::string &pattern,
                                   const boost::filesystem::path &weight_file);

    /**
     * \brief Start the solver given the corpus data provided.
     *
     * The returned object contains a vector of file paths. Each file in this
     * vector is part of the solution. This object owns the solution so we
     * return a reference to it.
     *
     * \param data is the sparse matrix data structure
     * \param corpus_data extra meta data about the corpus
     * \param name user defined run name
     * \param greedy whether to use the Greedy Algorithm or the Reduction
     * Algorithm
     * \return Solution to the OSCP problem
     */
    Solution solve_oscp(Matrix &data, CORPUS_DATA &corpus_data,
                        const std::string &name, bool greedy,
                        const boost::filesystem::path &weight_file);

protected:
    ///////////////////////////////////////////////////////////////////////
    // Member Variables
    ///////////////////////////////////////////////////////////////////////

    /**
     * This solution class is required as when we are dealing with corpuses
     * that result in a matrix larger than physcial memory (--largedata).
     * We do the first reduction step (select row unitarians) before
     * constructing the matrix in memory (see calc_cols_to_ignore()). Hence
     * need somewhere to store the solution-so-far before full read in.
     */
    Solution solution;
};

#endif /* OSCP_SOLVER_H */
