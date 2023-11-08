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
 * \date 2 March 2015
 *
 * \author Liam Hayes + Jonathan Milford
 * \date Feb 2017
 *
 * \brief After looking at various libraries for an appropriate sparse matrix
 * library it looks like that I may be better off implementing my own. Most
 * libraries are targeted for linear algebra operations and my requirements
 * are different enough to render those implementations as inappropriate.
 *
 * Reasons are:
 * * My data is a __very large logical matrix__. It only needs to store ones
 *   and zeros. I don't need to store arbitrary values.
 * * It needs to be memory efficient.
 * * I need to be able to _delete rows and columns_. I do _not_ need to be able
 *   to insert rows and columns.
 *
 * This implementation will be a _List of Lists_ implementation in row major
 * format.
 */

#ifndef MATRIX_H
#define MATRIX_H

#include <vector>

#include <boost/serialization/vector.hpp>

#include "moonlight.h"

#define DELETED -1

/**
 * \brief Each row in the matrix is represented as a row element.
 *
 * A row element is stored in the matrix in a doubly linked list. Overall
 * we are implementing a List of Lists data model of a matrix. We record useful
 * information about each row such as which exemplar it models, the file size
 * and most importantly the column data.
 */
class RowElem {
    ///////////////////////////////////////////////////////////////////////
    // Serialisation
    ///////////////////////////////////////////////////////////////////////

    friend class boost::serialization::access;
    friend std::ostream &operator<<(std::ostream &os, const RowElem &element);

    template <class Archive>
    void save(Archive &ar, const unsigned int version) const {
        // we don't serialise 'path' objects but the string representation
        std::string fname = file_path.native();
        ar &file_size;
        ar &row_sum;
        ar &fname; // string version of path
        ar &column;
        ar &weight;
    }

    template <class Archive>
    void load(Archive &ar, const unsigned int version) {
        // we don't serialise 'path' objects but the string representation
        std::string fname;
        ar &file_size;
        ar &row_sum;
        ar &fname; // string version of path
        ar &column;
        ar &weight;
        this->file_path = boost::filesystem::path(fname); // recreate
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

public:
    /** default constructor */
    RowElem();

    RowElem(boost::filesystem::path file, int filesize, int sum, double weight);

    /**
     * \brief Constructor to create a row element given a path to the exemplar
     * file and a column index transform. If all columns are to be read in then
     * the identity transform is used.
     *
     * \todo There is an open question to whether there is a genuine
     * performance benefit of having multiple worker threads concurrently
     * making calls to this method to process batches of exemplar files
     * together. The real question behind this is whether there is a
     * performance benefit in concurrent file IO and this is influenced
     * significantly by the underlying kernel support and disk and controller
     * hardware and technology. I let some young coding ninja work this out :-)
     *
     * \param exemplar path to the exemplar file
     * \param column index transform (new_index = transform[old_index]) so that
     * a set of columns can be ignored
     */
    RowElem(const boost::filesystem::path &exemplar,
            const std::vector<int> &init_col_transform);

    /** Size of trace file in bytes */
    int file_size;

    /** Number of ones in the row */
    int row_sum;

    /** Path to the exemplar file */
    boost::filesystem::path file_path;

    /** Sequence of non-zero column indices for this row */
    COL_DATA column;

    /** The file weighting (for weighted set cover problem) */
    double weight;

    /**
     * \brief Two RowElem objects are equivalent iff they have the same column
     * values irrespective of exemplar file...
     */
    bool operator==(const RowElem &other) const;
};

BOOST_CLASS_VERSION(RowElem, 0)

/**
 * \brief Matrix is a data model for a logical sparse matrix.
 *
 * The matrix data model is a __list of lists__ abstraction in row major format.
 * The row list is a doubly linked list of RowElement objects ordered by row
 * index. Each RowElement object contains some meta data about the row such
 * the path to the exemplar file, the file size, the row sum and most
 * importantly a list of column indices.
 *
 * The column data records only the indices of columns where there is a one
 * in the matrix. If a column index is not present in the list it is assumed
 * to be zero. This results in a large memory saving for sparse matrices.
 * Specifically, the memory requirements are
 *  * 32bits times number of ones in the matrix, plus
 *  * number of rows times sizeof(RowElement)
 *
 * In practice, the first factor dominates the memory requirements given real
 * data.
 *
 * This implementation supports row and column deletions but __not__ insertions
 * apart from the obvious initial matrix construction. Row operations are
 * generally efficient but column operations, and deletions in particular, are
 * more difficult - a direct consequence of our list of lists data model. One
 * interesting aspect of column deletions is my choice __not__ to actually
 * release storage of any deleted column indices in the lists. Instead I simply
 * mark these elements as "deleted" and decrement the greater indices.
 * Consequently the column lists never get smaller and memory requirements
 * remain constant. It is a complex balance of considerations whether
 * ultimately this might be beneficial to our application but one I choose to
 * completely ignore as a distraction to my goals. If you're interested in
 * having a look go fill your boots mate. Oh, and also look at the
 * multi-threaded file IO performance question as well while you're at it...
 */
class Matrix {
    ///////////////////////////////////////////////////////////////////////
    // serialisation
    ///////////////////////////////////////////////////////////////////////

    friend class boost::serialization::access;
    friend std::ostream &operator<<(std::ostream &os, const Matrix &matrix);

    template <class Archive>
    void save(Archive &ar, unsigned int version) const {
        // we don't serialise 'path' objects but the string representation
        std::string dname = directory.native();
        ar &num_rows;
        ar &num_cols;
        ar &num_cols_orig;
        ar &num_elems;
        ar &dname; // TODO verify
        ar &pattern;
        ar &rowlist;
    }

    template <class Archive> void load(Archive &ar, unsigned int version) {
        // we don't serialise 'path' objects but the string representation
        std::string dname;
        ar &num_rows;
        ar &num_cols;
        ar &num_cols_orig;
        ar &num_elems;
        ar &dname; // TODO verify
        ar &pattern;
        ar &rowlist;
        this->directory = boost::filesystem::path(dname); // recreate
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

public:
    /** Default constructor */
    Matrix();

    /** Useful constructor */
    Matrix(int rows, int columns);

    /** Useful constructor */
    Matrix(const boost::filesystem::path &directory, const std::string &pattern,
           const boost::filesystem::path &weight_file,
           INDEX_LIST cols_to_ignore);
    /**
     * \brief Useful constructor
     *
     * Restore Matrix from serialised data file
     */
    Matrix(const boost::filesystem::path &matrixfile);

    /** Copy constructor */
    Matrix(const Matrix &orig);

    /** Copy assignment */
    Matrix &operator=(const Matrix &orig);

    /** Move constructor */
    Matrix(const Matrix &&orig) noexcept;

    /** Move assignment */
    Matrix &operator=(const Matrix &&orig) noexcept;

    ///////////////////////////////////////////////////////////////////////
    // API
    ///////////////////////////////////////////////////////////////////////

    /**
     * \brief Number of rows in the matrix.
     *
     * \return Surprise - the number of rows in the matrix
     */
    int get_num_rows() const;

    /**
     * \brief Number of columns in the matrix.
     *
     * \return Surprise - the number of columns in the matrix
     */
    int get_num_cols() const;

    /**
     * \brief Number of columns in the original matrix, before any columns were
     * ignored or 'deleted'.
     *
     * num_cols_orig == 8 * file_size(largest_file)
     *
     * \return The number of columns in the original matrix
     */
    int get_num_cols_orig() const;

    /**
     * \brief Number of elements in the matrix.
     *
     * \return Surprise - the number of columns in the matrix
     */
    long long get_num_elements() const;

    /**
     * \return Iterator to start of rowlist. Allows direct access to the
     * RowElems that make up the matrix
     */
    std::vector<RowElem>::iterator rowlist_begin();

    /**
     * \return Iterator to end of rowlist. Allows direct access to the RowElems
     * that make up the matrix
     */
    std::vector<RowElem>::iterator rowlist_end();

    /**
     * \return Iterator to start of vector of column indices for given row
     */
    COL_DATA::iterator column_begin(int row);

    /**
     * \return Iterator to end of vector of column indices for given row
     */
    COL_DATA::iterator column_end(int row);

    /**
     * \brief Insert a row into the matrix.
     *
     * After the matrix is constructed it is empty of any data. This method
     * simply allows you to install a new row to the end of the matrix.
     * Assumption is that the number of rows inserted is equal to the number
     * of rows provided in the constructor. Behaviour is undefined if that is
     * not the case.
     */
    void insert_row(RowElem &row);

    /**
     * \brief Delete a row from the matrix.
     *
     * Remove row 'r' from the matrix.
     *
     * \param r row index.
     * \throws out_of_range exception if 'r' is negative or too big.
     */
    void remove_row(int r);

    /**
     * \brief Delete a set of rows from the matrix.
     *
     * All the row indices in the list are taken to refer to the row positions
     * __before any deletions__ have occurred. For example {1, 3, 5} means
     * delete rows 1, 3 and 5. Therefore this is __not the same__ as performing
     * remove_row(1), remove_row(3), remove_row(5) because the row indices
     * will change during each remove_row operation.
     *
     * \param del_list sequence of rows to be deleted
     * \throws out_of_range exception if any row index is negative or too big.
     */
    void remove_rows(INDEX_LIST &del_list);

    /**
     * \brief Delete a column from the matrix.
     *
     * Remove column 'c' from the matrix.
     *
     * \param c column index.
     * \throws out_of_range exception if 'c' is negative or too big.
     */
    void remove_col(int c);

    /**
     * \brief Delete a set of columns from the matrix.
     *
     * All the column indices in the list are taken to refer to the column
     * positions __before any deletions__ have occurred. For example {1, 3, 5}
     * means delete columns 1, 3 and 5. Therefore this is __not the same__ as
     * performing remove_col(1), remove_col(3), remove_col(5) because the
     * column indices will change during each remove_col operation.
     *
     * \param del_list sequence of columns to be deleted
     * \throws out_of_range exception if any column index is negative or too
     * big.
     */
    void remove_cols(INDEX_LIST &del_list);

    /**
     * \brief Retrieve a column vector from the matrix.
     *
     * \param c column index
     * \throws out_of_range exception if column index is negative or too big.
     */
    COLUMN get_col(int c) const;

    /**
     * \brief Retrieve a row vector from the matrix.
     *
     * \param r row index
     * \throws out_of_range exception if row index is negative or too big.
     */
    ROW get_row(int r) const;

    /**
     * \brief Is the value of the matrix at [r,c] one? If so return true.
     *
     * \param r row index
     * \param c column index
     * \return true if Matrix[r,c] == 1 else false
     * \throws out_of_range exception if any row or column index is negative or
     * too big.
     */
    bool is_row_column_set(int r, int c) const;

    /**
     * \brief Get / compute the row sum for a given row
     *
     * \return row sum
     */
    int get_row_sum(int r) const;

    /**
     * \brief Compute the row sum for each row in the matrix and return as a
     * vector.
     *
     * \return row sum vector
     */
    ROW_SUM get_row_sum() const;

    /**
     * \brief Compute the column sum for each column in the matrix and return
     * as a vector.
     *
     * \return column sum vector
     */
    COLUMN_SUM get_column_sum() const;

    /**
     * \brief Compute number of columns contained in the two given rows
     *
     * \return number of overlapping columns
     * \todo move function to OSCPSolver, not
     */
    int get_overlap(int r1, int r2) const;

    /**
     * \brief Get path to binary file from which the given row was constructed
     *
     * \return path to data file
     */
    boost::filesystem::path get_row_exemplar(int r) const;

    /**
     * \brief Get size of binary file from which the given row was constructed
     * vector.
     *
     * \return file size
     */
    int get_row_file_size(int r) const;

    /**
     * /brief Get the row's weight, used for the weighted set cover problem
     *
     * \param r The row index
     * \return a double representing the weight
     */
    double get_row_weight(int r) const;

    // debugging function, check matrix consistency
    void assert_row_sums() const;

    /**
     * \brief Two Matrix objects are equivalent iff they have the same number of
     * columns, rows, elements and each [row,col] value is identical.
     *
     * Note: if one matrix is a row-column permutation of the other we don't
     * consider them identical.
     */
    bool operator==(const Matrix &other) const;

private:
    /** Number of rows */
    int num_rows;

    /** Number of columns */
    int num_cols;

    /** Number of columns in original matrix*/
    int num_cols_orig;

    /** Number of ones in the matrix */
    long long num_elems;

    /** Directory path to the corpus */
    boost::filesystem::path directory;

    /** Regex pattern to select corpus exemplars in the directory */
    std::string pattern;

    /**
     * Doubly linked list of row elements. Each row element contains amongst
     * other things column data
     */
    std::vector<RowElem> rowlist;
};

BOOST_CLASS_VERSION(Matrix, 0)

#endif /* MATRIX_H */
