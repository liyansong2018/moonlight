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
 * \date March 4, 2015
 *
 * \author Liam Hayes + Jonathan Milford
 * \date Feb 2017
 */

#ifndef CORPUS_H
#define CORPUS_H

#include <map>
#include <vector>

#include <boost/filesystem/path.hpp>

#include "moonlight.h"

/**
 * \brief Utility class for corpus file management.
 *
 * A corpus file object is simply a container for the file path, file size and
 * row index. We can compare two corpus files relationally based on its file
 * _size_.
 */
class CorpusFile {
public:
    CorpusFile(boost::filesystem::path f, int size);

    /** Default constructor **/
    CorpusFile();

    /** Path to the corpus file */
    boost::filesystem::path file_path;

    /** Size of the file in bytes */
    int file_size;

    /**
     * brief Two corpus file objects are equivalent iff their file path, index
     * and size are equivalent.
     */
    bool operator==(const CorpusFile &other) const;

    /**
     * Corpus file A is less than B iff A.size < B.size
     */
    bool operator<(const CorpusFile &other) const;

    /**
     * Corpus file A is less than or equal than B iff A.size <= B.size
     */
    bool operator<=(const CorpusFile &other) const;

    /**
     * Corpus file A is greater than B iff A.size > B.size
     */
    bool operator>(const CorpusFile &other) const;

    /**
     * Corpus file A is greater than or equal to B iff A.size >= B.size
     */
    bool operator>=(const CorpusFile &other) const;

    /**
     * Corpus file A is NOT equivalent to B iff NOT(A == B)
     */
    bool operator!=(const CorpusFile &other) const;
};

///////////////////////////////////////////////////////////////////////
// Utility Function Declarations
///////////////////////////////////////////////////////////////////////

/**
 * \brief Get a vector of the corpus files.
 *
 * The caller must provide a path to the directory containing the corpus files
 * and a regex string pattern specifying the pattern of the exemplar files in
 * the corpus.
 *
 * \param path path to the corpus directory
 * \param pattern regex pattern specifying the corpus files
 * \return a vector of CorpusFile objects no particular ordering
 */
std::vector<CorpusFile> get_file_list(const boost::filesystem::path &p,
                                      const std::string &pattern);

/**
 * \brief Return the row data associated with the exemplar file.
 *
 * The resulting vector will contain ones and zeros denoting if the exemplar
 * file used the corresponding basic block or not.
 *
 * \param exemplar_path path to the exemplar file
 * \return exemplar data
 * \todo could return UINT8 valarray instead of INT32 vector for performance
 */
ROW get_exemplar_data(const boost::filesystem::path &exemplar_path);

/**
 * \brief Return the data associated with a weight file.
 *
 * A weight file contains a mapping of an example file name to its weight
 * value.
 *
 * \param weight_file path to the weights file
 * \return weights data
 */
std::map<std::string, double>
get_weight_data(const boost::filesystem::path &weight_file);

#endif /* CORPUS_H */
