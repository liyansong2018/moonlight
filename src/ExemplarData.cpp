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
 * \date May 7, 2015
 *
 * \author Liam Hayes + Jonathan Milford
 * \date Feb 2017
 */

#include <boost/archive/binary_iarchive.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/support/date_time.hpp>

#include "ExemplarData.h"
#include "Matrix.h"

using namespace std;
using namespace boost::filesystem;
using namespace boost::serialization;
namespace src = boost::log::sources;

CORPUS_DATA initialise_corpus_data(Matrix &matrix) {
    CORPUS_DATA corpus_data;
    for (auto it = matrix.rowlist_begin(); it != matrix.rowlist_end(); it++) {
        RowElem element = *it;
        ExemplarData ex_data;
        ex_data.file_path = element.file_path;
        ex_data.file_size = element.file_size;
        ex_data.score_rowsum = element.row_sum;
        ex_data.score_block_target = 0.0;
        ex_data.score_unitarian = 0.0;
        ex_data.selected_greedy_rowsum = false;
        corpus_data.push_back(ex_data);
    }

    return corpus_data; // move
}

///////////////////////////////////////////////////////////////////////
// Exemplar Data Implementation
///////////////////////////////////////////////////////////////////////

// default constructor
ExemplarData::ExemplarData()
    : file_size(0), file_path(path("")), selected_greedy_rowsum(false),
      score_rowsum(0.0), score_unitarian(0.0), score_block_target(0.0) {
}

// copy constructor
ExemplarData::ExemplarData(const ExemplarData &orig) {
    this->file_size = orig.file_size;
    this->file_path = orig.file_path;
    this->selected_greedy_rowsum = orig.selected_greedy_rowsum;
    this->score_rowsum = orig.score_rowsum;
    this->score_unitarian = orig.score_unitarian;
    this->score_block_target = orig.score_block_target;
}

// copy assignment
ExemplarData &ExemplarData::operator=(const ExemplarData &rhs) {
    // protect against self assignment eg "mymatrix = mymatrix";
    if (this != &rhs) {
        this->file_size = rhs.file_size;
        this->file_path = rhs.file_path;
        this->selected_greedy_rowsum = rhs.selected_greedy_rowsum;
        this->score_rowsum = rhs.score_rowsum;
        this->score_unitarian = rhs.score_unitarian;
        this->score_block_target = rhs.score_block_target;
    }

    return *this;
}

// move constructor
ExemplarData::ExemplarData(const ExemplarData &&orig) noexcept {
    this->file_size = orig.file_size;
    this->file_path = orig.file_path;
    this->selected_greedy_rowsum = orig.selected_greedy_rowsum;
    this->score_rowsum = orig.score_rowsum;
    this->score_unitarian = orig.score_unitarian;
    this->score_block_target = orig.score_block_target;
}

// move assignment
ExemplarData &ExemplarData::operator=(const ExemplarData &&rhs) noexcept {
    // protect against self assignment eg "mymatrix = mymatrix";
    if (this != &rhs) {
        this->file_size = rhs.file_size;
        this->file_path = rhs.file_path;
        this->selected_greedy_rowsum = rhs.selected_greedy_rowsum;
        this->score_rowsum = rhs.score_rowsum;
        this->score_unitarian = rhs.score_unitarian;
        this->score_block_target = rhs.score_block_target;
    }

    return *this;
}

ExemplarData::ExemplarData(const path &exemplardatafile) {
    src::severity_logger<> &mylog = my_logger::get();
    ExemplarData object;

    // check if file exists and is usable
    if (exists(exemplardatafile) && is_regular_file(exemplardatafile)) {
        // attempt to restore it
        std::ifstream object_file_in(exemplardatafile.native());
        BOOST_LOG(mylog) << "Reading exemplar meta data in from file: "
                         << exemplardatafile;
        boost::archive::binary_iarchive sin(object_file_in);

        // read class state from archive
        sin >> object;
        BOOST_LOG(mylog) << "Finished reading in ExemplarData content...";

        // archive and stream closed when destructors are called - hence inner
        // block
    } else {
        BOOST_LOG_SEV(mylog, error) << "ExemplarData data file does not exist "
                                       "or is not a regular file!";
        throw runtime_error(
            "ExemplarData data file does not exist or is not a regular file");
    }

    // move assign to *this
    *this = object;
}

string ExemplarData::csv_print() const {
    ostringstream csv;

    csv << this->file_path.filename() << ", " << this->file_size << ", "
        << this->selected_greedy_rowsum << ", " << std::setprecision(6)
        << this->score_rowsum << ", " << this->score_unitarian << ", "
        << this->score_block_target;

    return csv.str();
}

///////////////////////////////////////////////////////////////////////
// Utility Functions
///////////////////////////////////////////////////////////////////////

// Header for printing corpus data to CSV
static const string CSV_HEADER = "index, file, trace_file_size, "
                                 "selected_greedy, score_rowsum, "
                                 "score_unitarian, score_block_target";

void csv_print(const path &fpath, const CORPUS_DATA &data) {
    std::ofstream fout(fpath.native());

    fout << CSV_HEADER << endl;

    for (unsigned int i = 0; i < data.size(); i++) {
        ExemplarData exdata = data[i];

        fout << i << ", " << exdata.csv_print() << endl << flush;
    }
}
