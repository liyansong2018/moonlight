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
 * \date 7th May 2015
 *
 * \author Liam Hayes + Jonathan Milford
 * \date Feb 2017
 */

#ifndef EXEMPLAR_DATA_H
#define EXEMPLAR_DATA_H

#include <boost/serialization/vector.hpp>
#include <boost/serialization/version.hpp>

#include "moonlight.h"

class Matrix;

/**
 * \brief Objects of this class record useful meta-data about the exemplar
 * that we may have use for.
 *
 * \todo some of the fields are duplicated across classes. Not so smart.
 */
class ExemplarData {
    ///////////////////////////////////////////////////////////////////////
    // serialisation
    ///////////////////////////////////////////////////////////////////////

    friend class boost::serialization::access;
    friend std::ostream &operator<<(std::ostream &os,
                                    const ExemplarData &element);

    template <class Archive>
    void save(Archive &ar, const unsigned int version) const {
        // we don't serialise 'path' objects but the string representation
        std::string fname = file_path.native();
        ar &file_size;
        ar &fname; // string version of path
        ar &selected_greedy_rowsum;
        ar &score_rowsum;
        ar &score_unitarian;
        ar &score_block_target;
    }

    template <class Archive> void load(Archive &ar, unsigned int version) {
        // we don't serialise 'path' objects but the string representation
        std::string fname;
        ar &file_size;
        ar &fname; // string version of path
        this->file_path = boost::filesystem::path(fname); // recreate
        ar &selected_greedy_rowsum;
        ar &score_rowsum;
        ar &score_unitarian;
        ar &score_block_target;
    }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

public:
    /** Default constructor */
    ExemplarData();

    /**
     * \brief Useful constructor
     *
     * Restore ExemplarData from serialised data file.
     */
    ExemplarData(const boost::filesystem::path &exemplardatafile);

    /** Copy constructor */
    ExemplarData(const ExemplarData &orig);

    /** Copy assignment */
    ExemplarData &operator=(const ExemplarData &orig);

    /** Move constructor */
    ExemplarData(const ExemplarData &&orig) noexcept;

    /** Move assignment */
    ExemplarData &operator=(const ExemplarData &&orig) noexcept;

    /** Print an exemplar's meta-data in CSV format. */
    std::string csv_print() const;

    /** File size in bytes */
    int file_size;

    /** Path to the exemplar file */
    boost::filesystem::path file_path;

    /** True iff the greedy rowsum OSCP algorithm chose this exemplar */
    bool selected_greedy_rowsum;

    /** Exemplar's basic block/rowsum score */
    double score_rowsum;

    /** Unitarian score: number of unitarian blocks in this exemplar */
    double score_unitarian;

    /** Block target score: number of target basic blocks in this exemplar */
    double score_block_target;
};

BOOST_CLASS_VERSION(ExemplarData, 0)

using CORPUS_DATA = std::vector<ExemplarData>;

/**
 * \brief Utility method to create an initial CORPUS_DATA object from the
 * initial matrix object.
 *
 * We want a place to store all the meta-data about each of the exemplars we
 * are interested in. The matrix we have just instantiated has some of the
 * information and as we later run our distillation algorithms over the matrix
 * we will collect more.
 *
 * \param matrix the initial matrix object before any distillation operations
 *  have occurred
 * \return a new CORPUS_DATA object with partial initialisation of exemplar
 * data objects
 */
CORPUS_DATA initialise_corpus_data(Matrix &matrix);

/** Print the corpus data to a CSV file */
void csv_print(const boost::filesystem::path &fpath, const CORPUS_DATA &data);

#endif /* EXEMPLAR_DATA_H */
