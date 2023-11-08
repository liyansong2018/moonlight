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
 */

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>

#include "Corpus.h"
#include "Matrix.h"

using namespace boost::filesystem;
using namespace boost::serialization;
using namespace std;
namespace src = boost::log::sources;

// unitity function prototypes
static vector<int> transform_index(int num_elements, INDEX_LIST &delset);
void pprint_map(map<string, int> mymap);

RowElem::RowElem()
    : file_size(0), row_sum(0), file_path(boost::filesystem::path("")),
      column(COL_DATA()), weight(1.0) {
}

RowElem::RowElem(boost::filesystem::path file, int filesize, int sum,
                 double weight)
    : file_size(filesize), row_sum(sum), file_path(file),
      column(COL_DATA(sum, 0)), weight(weight) {
}

RowElem::RowElem(const path &exemplar, const vector<int> &init_col_transform) {
    this->file_path = exemplar;
    this->file_size = boost::filesystem::file_size(exemplar);

    // read in the raw data from the file
    ROW rawdata = get_exemplar_data(exemplar);

    // make a column list
    COL_DATA temp;
    int rowsum = 0;
    for (unsigned int idx = 0; idx < rawdata.size(); idx++) {
        if (rawdata[idx]) {
            if (init_col_transform[idx] != DELETED) {
                temp.push_back(
                    init_col_transform[idx]); // record the column index
                rowsum++;
            }
        }
    }

    this->row_sum = rowsum;
    this->column = temp;
    this->weight = 1.0;
}

///////////////////////////////////////////////////////////////////////
// Binary Operators
///////////////////////////////////////////////////////////////////////

bool RowElem::operator==(const RowElem &other) const {
    if (this->row_sum != other.row_sum) {
        return false;
    }

    if (this->column != other.column) {
        return false;
    }

    return true;
}

bool Matrix::operator==(const Matrix &other) const {
    auto lrows = this->get_num_rows();
    auto lcols = this->get_num_cols();
    auto lelems = this->get_num_elements();
    auto rrows = other.get_num_rows();
    auto rcols = other.get_num_cols();
    auto relems = other.get_num_elements();

    // look for quick fails
    if (lrows != rrows) {
        return false;
    }

    if (lcols != rcols) {
        return false;
    }

    if (lelems != relems) {
        return false;
    }

    // this will be slow if the matrix is large
    for (int r = 0; r < lrows; r++) {
        ROW left = this->get_row(r);
        ROW right = other.get_row(r);

        if (left != right) {
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////
// Matrix Implementation
///////////////////////////////////////////////////////////////////////

Matrix::Matrix()
    : num_rows(0), num_cols(0), num_cols_orig(0), num_elems(0LL),
      directory(path("")), pattern(""), rowlist(vector<RowElem>()) {
}

Matrix::Matrix(int rows, int columns)
    : num_rows(rows), num_cols(columns), num_cols_orig(columns), num_elems(0LL),
      directory(path("")), pattern(""), rowlist(vector<RowElem>()) {
    if ((rows < 0) || (columns < 0)) {
        throw out_of_range("Row or column size can't be negative");
    }
}

Matrix::Matrix(const path &directory, const string &pattern,
               const path &weight_file, INDEX_LIST cols_to_ignore)
    : num_rows(0), num_cols(0), num_cols_orig(0), num_elems(0LL),
      directory(directory), pattern(pattern), rowlist(vector<RowElem>()) {
    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "Finding files at path: " << directory
                     << " with pattern: " << pattern;

    vector<CorpusFile> corpus = get_file_list(directory, pattern);
    if (corpus.empty()) {
        BOOST_LOG(mylog) << "No files found, exiting";
        exit(0);
    }

    // sort the corpus - largest to smallest
    sort(corpus.begin(), corpus.end(), greater<CorpusFile>());
    BOOST_LOG(mylog) << "Corpus size: " << corpus.size();

    // this is the TRACE file size, not the exemplar file size!
    int max_file_size = 0;
    if (corpus.size() > 0) {
        max_file_size = corpus[0].file_size;
    }
    num_cols_orig = (8 * max_file_size);

    // create a column index transform to ignore the given column indices
    vector<int> init_col_transform =
        transform_index(num_cols_orig, cols_to_ignore);

    // create a map from exemplar file names to weights
    bool weighted = false;
    map<string, double> weight_map;
    if (!weight_file.empty()) {
        BOOST_LOG(mylog) << "Weighted version";
        weighted = true;
        weight_map = get_weight_data(weight_file);
    } else {
        BOOST_LOG(mylog) << "Unweighted version";
    }

    // now parse the corpus files and insert into the matrix
    // adjust the weights if needed, or leave them as 1 (default)
    BOOST_LOG(mylog) << "Parsing corpus files and inserting into the matrix...";
    num_rows = 0;

    for (unsigned int r = 0; r < corpus.size(); r++) {
        path f = corpus[r].file_path;
        RowElem row(f, init_col_transform);
        if (weighted) {
            string name = row.file_path.filename().string();
            if (weight_map.find(name) != weight_map.end()) {
                row.weight = weight_map[name];
                weight_map.erase(name);

                if (row.weight > 0) {
                    // discard any exemplars with non +ve weights
                    this->insert_row(row);
                }
            } else {
                BOOST_LOG(mylog) << "Ignoring exemplar with no know weight: '"
                                 << name << "'";
            }
        } else {
            // unweighted version
            this->insert_row(row);
        }

        if ((r % 100) == 0) {
            BOOST_LOG(mylog) << "File: " << r << ", " << f.filename();
        }
    }
    num_cols = num_cols_orig - cols_to_ignore.size();

    // ensure all the exemplar weights are used, if not there is likely an
    // error
    assert(weight_map.size() == 0);

    double density = (100.0 * num_elems) / (1.0 * num_cols * num_rows);
    BOOST_LOG(mylog) << "Finished creating the matrix";
    BOOST_LOG(mylog) << "Number of Rows: " << num_rows;
    BOOST_LOG(mylog) << "Number of Columns: " << num_cols;
    BOOST_LOG(mylog) << "Number of Elements: " << num_elems;
    BOOST_LOG(mylog) << "Sparsity (density): " << density << " %";
    BOOST_LOG(mylog) << "";
}

Matrix::Matrix(const path &matrixfile) {
    src::severity_logger<> &mylog = my_logger::get();
    Matrix dmat;

    // check if file exists and is usable
    if (exists(matrixfile) && is_regular_file(matrixfile)) {
        // attempt to restore it
        std::ifstream matrix_file_in(matrixfile.native());
        BOOST_LOG(mylog) << "Reading matrix data in from file: " << matrixfile;
        // boost::archive::text_iarchive sin(matrix_file_in);
        boost::archive::binary_iarchive sin(matrix_file_in);

        // read class state from archive
        sin >> dmat;
        BOOST_LOG(mylog) << "Finished reading in Matrix data...";

        // archive and stream closed when destructors are called - hence inner
        // block
    } else {
        BOOST_LOG_SEV(mylog, error)
            << "Matrix data file does not exist or is not a regular file!";
        throw runtime_error(
            "Matrix data file does not exist or is not a regular file");
    }

    // move assign to *this
    *this = dmat;
}

// copy constructor
Matrix::Matrix(const Matrix &orig) {
    this->num_rows = orig.num_rows;
    this->num_cols = orig.num_cols;
    this->num_cols_orig = orig.num_cols_orig;
    this->num_elems = orig.num_elems;
    this->directory = orig.directory;
    this->pattern = orig.pattern;
    this->rowlist = orig.rowlist; // copy assignment
}

// copy assignment
Matrix &Matrix::operator=(const Matrix &rhs) {
    // protect against self assignment eg "mymatrix = mymatrix";
    if (this != &rhs) {
        this->num_rows = rhs.num_rows;
        this->num_cols = rhs.num_cols;
        this->num_cols_orig = rhs.num_cols_orig;
        this->num_elems = rhs.num_elems;
        this->directory = rhs.directory;
        this->pattern = rhs.pattern;
        this->rowlist = rhs.rowlist; // copy assignment
    }

    return *this;
}

// move constructor
Matrix::Matrix(const Matrix &&orig) noexcept {
    this->num_rows = orig.num_rows;
    this->num_cols = orig.num_cols;
    this->num_cols_orig = orig.num_cols_orig;
    this->num_elems = orig.num_elems;
    this->directory = orig.directory;
    this->pattern = orig.pattern;
    this->rowlist = orig.rowlist;
}

// move assignment
Matrix &Matrix::operator=(const Matrix &&rhs) noexcept {
    // protect against self assignment eg "mymatrix = mymatrix";
    if (this != &rhs) {
        this->num_rows = rhs.num_rows;
        this->num_cols = rhs.num_cols;
        this->num_cols_orig = rhs.num_cols_orig;
        this->num_elems = rhs.num_elems;
        this->directory = rhs.directory;
        this->pattern = rhs.pattern;
        this->rowlist = rhs.rowlist;
    }

    return *this;
}

///////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////

vector<int> transform_index(int num_elements, INDEX_LIST &delset) {
    vector<int> transform(num_elements, 0);

    sort(delset.begin(), delset.end());

    int del_count = 0;
    int num_del = delset.size();
    for (int i = 0; i < num_elements; i++) {
        if (del_count < num_del && i == delset[del_count]) {
            del_count++;
            transform[i] = DELETED;
        } else {
            transform[i] = i - del_count;
        }
    }

    assert(del_count == num_del); // fails if there are duplicates in delset

    return transform;
}

void pprint_map(const map<string, int> &mymap) {
    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "Weight map entries....";

    for (auto it = mymap.begin(); it != mymap.end(); it++) {
        BOOST_LOG(mylog) << "'" << it->first << "' " << it->second;
    }
}

void Matrix::assert_row_sums() const {
    assert(num_rows == (int) rowlist.size());
    auto riter = rowlist.begin();

    while (riter != rowlist.end()) {
        int count = 0;
        auto citer = riter->column.begin();

        while (citer != riter->column.end()) {
            if (*citer != DELETED) {
                count++;
            }

            advance(citer, 1);
        }

        assert(count == riter->row_sum);
        advance(riter, 1);
    }
}

///////////////////////////////////////////////////////////////////////
// API
///////////////////////////////////////////////////////////////////////

int Matrix::get_num_rows() const {
    return this->num_rows;
}

int Matrix::get_num_cols() const {
    return this->num_cols;
}

int Matrix::get_num_cols_orig() const {
    return this->num_cols_orig;
}

long long Matrix::get_num_elements() const {
    return num_elems;
}

void Matrix::insert_row(RowElem &row) {
    this->rowlist.push_back(row);
    num_rows++;
    num_elems += row.row_sum;
}

void Matrix::remove_row(int r) {
    INDEX_LIST del_list(r);
    remove_rows(del_list);
}

void Matrix::remove_rows(INDEX_LIST &del_list) {
    for (auto r : del_list) {
        if ((r < 0) || (r > num_rows)) {
            throw out_of_range("remove_rows: row index out of range");
        }
    }

    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "MATRIX: "
                     << "removing " << del_list.size() << " rows";

    // sort in descending order. This ordering will preserve the integrity of
    // the row indices in the index list - that is they will be correct after
    // we delete rows with higher index values.
    sort(del_list.begin(), del_list.end(), std::greater<int>());
    for (int r : del_list) {
        auto iter = rowlist.begin();
        advance(iter, r);
        num_elems -= iter->row_sum;
        rowlist.erase(iter, iter + 1); // delete this RowElem, shuffle the rest
                                       // down (since rowlist is vector)
        num_rows--;
        assert(num_rows == (int) rowlist.size());
    }
}

void Matrix::remove_col(int c) {
    INDEX_LIST del_list(c);
    remove_cols(del_list);
}

void Matrix::remove_cols(INDEX_LIST &del_list) {
    assert_row_sums();

    for (auto c : del_list) {
        if ((c < 0) || (c > num_cols)) {
            throw out_of_range("remove_cols: column index out of range");
        }
    }

    src::severity_logger<> &mylog = my_logger::get();
    BOOST_LOG(mylog) << "MATRIX: "
                     << "removing " << del_list.size() << " cols";
    vector<int> Transform = transform_index(num_cols, del_list);

    auto iter = rowlist.begin();
    for (int r = 0; r < num_rows; r++) {
        RowElem &element = *iter;        // ref! don't copy!
        COL_DATA &data = element.column; // ref! don't copy!
        int size = data.size();
        int current_rowsum = element.row_sum;
        int new_rowsum = 0;

        // transform the column elements
        for (int i = 0; i < size; i++) {
            if (data[i] != DELETED) {
                data[i] = Transform[data[i]];
                if (data[i] != DELETED) {
                    new_rowsum++;
                }
            }
        }

        int delta = current_rowsum - new_rowsum;
        element.row_sum = new_rowsum;
        num_elems -= delta;
        advance(iter, 1);
    }

    num_cols -= del_list.size();
    assert_row_sums();
}

COLUMN Matrix::get_col(int c) const {
    if (c < 0 || c >= num_cols) {
        throw out_of_range("get_col: column index out of range");
    }

    COLUMN result(num_rows, 0);
    auto riter = rowlist.begin();

    for (int r = 0; r < num_rows; r++) {
        const RowElem &element = *riter;
        const COL_DATA &data = element.column;
        int size = data.size();
        int i = 0;

        // scan through the vector to see if we have that column
        while ((i < size) && ((data[i] < c) || (data[i] == DELETED))) {
            i++; // scan passed...
        }

        if (i < size && data[i] == c) {
            result[r] = 1;
        }

        advance(riter, 1);
    }

    return result;
}

ROW Matrix::get_row(int r) const {
    if (r < 0 || r >= num_rows) {
        throw out_of_range("get_row: row index out of range");
    }

    ROW result(num_cols, 0);
    auto iter = rowlist.begin();
    advance(iter, r);                          // find the row
    const COL_DATA &columndata = iter->column; // ref! don't copy!

    for (INDEX value : columndata) {
        if (value != DELETED) {
            result[value] = 1;
        }
    }

    return result;
}

bool Matrix::is_row_column_set(int r, int c) const {
    if ((r < 0) || (c < 0) || (r >= num_rows) || (c >= num_cols)) {
        throw out_of_range("is_row_column_set: index out of range");
    }

    auto riter = rowlist.begin();
    advance(riter, r);
    const COL_DATA &data = riter->column; // ref! don't copy!
    int size = data.size();

    // scan through the vector to see if we have that column
    int idx = 0;
    while ((idx < size) && ((data[idx] < c) || (data[idx] == DELETED))) {
        idx++; // scan passed...
    }

    // we are either at the position where the column should be or gone passed
    // it
    if ((idx < size) && (data[idx] == c)) {
        // this row has the column
        return true;
    }

    // we don't have the column
    return false;
}

ROW_SUM Matrix::get_row_sum() const {
    ROW_SUM result(num_rows, 0);
    auto iter = rowlist.begin();

    for (int r = 0; r < num_rows; r++) {
        result[r] = iter->row_sum;
        advance(iter, 1);
    }

    return result;
}

COLUMN_SUM Matrix::get_column_sum() const {
    COLUMN_SUM result(num_cols, 0);
    auto iter = rowlist.begin();

    for (int r = 0; r < num_rows; r++) {
        const COL_DATA &columndata = rowlist[r].column; // TODO: bad for lists
        for (INDEX value : columndata) {
            if (value != DELETED) {
                result[value]++;
            }
        }
        advance(iter, 1);
    }

    return result;
}

int Matrix::get_overlap(int r1, int r2) const {
    if (r1 < 0 || r2 < 0 || r1 > num_rows || r2 > num_rows) {
        throw out_of_range("get_overlap: row index not in range");
    }

    int result = 0;
    auto iter1 = rowlist.begin();
    auto iter2 = rowlist.begin();
    advance(iter1, r1);
    advance(iter2, r2);
    const COL_DATA &columndata1 = iter1->column;
    const COL_DATA &columndata2 = iter2->column;
    unsigned int j = 0;

    for (unsigned int i = 0; i < columndata1.size(); i++) {
        if (columndata1[i] == DELETED) {
            continue;
        }

        while (j < columndata2.size() && columndata2[j] < columndata1[i]) {
            j++;
        }

        if (j >= columndata2.size()) {
            break;
        }

        if (columndata2[j] == columndata1[i]) {
            result++;
        }
    }

    assert(result <= get_row_sum(r1) && result <= get_row_sum(r2));

    return result;
}

path Matrix::get_row_exemplar(int r) const {
    if (r < 0 || r >= num_rows) {
        throw out_of_range("get_row_exemplar: row index out of range");
    }

    auto riter = rowlist.begin();
    advance(riter, r);

    return riter->file_path;
}

int Matrix::get_row_file_size(int r) const {
    if (r < 0 || r >= num_rows) {
        throw out_of_range("get_row_file_size: row index out of range");
    }

    auto riter = rowlist.begin();
    advance(riter, r);

    return riter->file_size;
}

double Matrix::get_row_weight(int r) const {
    if (r < 0 || r >= num_rows) {
        throw out_of_range("get_row_file_size: row index out of range");
    }

    auto riter = rowlist.begin();
    advance(riter, r);

    return riter->weight;
}

int Matrix::get_row_sum(int r) const {
    if (r < 0 || r >= num_rows) {
        throw out_of_range("get_row_sum: row index out of range");
    }

    auto riter = rowlist.begin();
    advance(riter, r);

    return riter->row_sum;
}

vector<RowElem>::iterator Matrix::rowlist_begin() {
    return rowlist.begin();
}

vector<RowElem>::iterator Matrix::rowlist_end() {
    return rowlist.end();
}

COL_DATA::iterator Matrix::column_begin(int row) {
    if ((row < 0) || (row >= num_rows)) {
        throw out_of_range("columnbegin: index out of range");
    }

    auto riter = rowlist.begin();
    advance(riter, row);

    return riter->column.begin();
}

COL_DATA::iterator Matrix::column_end(int row) {
    if ((row < 0) || (row >= num_rows)) {
        throw out_of_range("columnbegin: index out of range");
    }

    auto riter = rowlist.begin();
    advance(riter, row);

    return riter->column.end();
}
