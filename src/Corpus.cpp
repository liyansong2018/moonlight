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

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/regex.hpp> // don't use g++ -std11 regex!!!

#include "Corpus.h"
#include "moonlight.h"

using namespace std;
using namespace boost::filesystem;
namespace src = boost::log::sources;

///////////////////////////////////////////////////////////////////////
// Corpus File Implementation
///////////////////////////////////////////////////////////////////////

CorpusFile::CorpusFile(path f, int size) : file_path(f), file_size(size) {
}

CorpusFile::CorpusFile() : CorpusFile(path(""), 0) {
}

///////////////////////////////////////////////////////////////////////
// Utility Function Declarations
///////////////////////////////////////////////////////////////////////

vector<CorpusFile> get_file_list(const path &directory, const string &pattern) {
    // use Boost REGEX not GNU G++ 2011 standard library!
    boost::regex r(pattern);
    vector<CorpusFile> myfilelist;

    try {
        if (exists(directory) && is_directory(directory)) {
            // open each file
            directory_iterator finished = directory_iterator();
            directory_iterator dir_object = directory_iterator(directory);
            while (dir_object != finished) {
                path f = *dir_object;
                // is it an ordinary file?
                if (is_regular_file(f)) {
                    // check if its a corpus file
                    string fqfn(f.filename().native());
                    if (boost::regex_search(fqfn, r)) {
                        // create a new CorpusFile object
                        int fsize = file_size(dir_object->path());
                        CorpusFile exemplar(absolute(f), fsize);
                        // add it to the list
                        myfilelist.push_back(exemplar);
                    }
                }
                dir_object++;
            }
        } else {
            // not file IO problems - user supplied data problems
            cerr << "Does not exist or is not a directory:" << endl;
            cerr << directory << endl;
            throw runtime_error("Corpus directory does not exist.");
        }
    } catch (const filesystem_error &ex) {
        // for IO problems
        throw runtime_error(
            "Problem processing the corpus data. File IO problems?");
        cerr << ex.what() << endl;
    }

    return myfilelist;
}

ROW get_exemplar_data(const path &exemplar) {
    int bytes = file_size(exemplar);
    ROW result(8 * bytes, 0);
    string fname = absolute(exemplar).native();

    int col = 0;
    char x; // signed in gnu g++

    std::ifstream input(fname, std::ifstream::in | std::ifstream::binary);

    for (int b = 0; b < bytes; b++) {
        // we need to use 'ifstream.get()' method to read a byte at a time but
        // there is no unsigned char read method. Hence the jiggery pokery with
        // casts below...
        input.get(x);
        unsigned char datum = static_cast<unsigned char>(x);
        unsigned char mask = 128; // binary 1000-0000

        // we need to expand the bit compression encoding into a byte level
        // representation
        for (int i = 0; i < 8; i++) {
            int value = ((datum & mask) > 0) ? 1 : 0;
            result[col] = value;
            col++;
            mask >>= 1;
        }
    }
    input.close();

    return result;
}

map<string, double> get_weight_data(const path &weight_file) {
    src::severity_logger<> &mylog = my_logger::get();
    map<string, double> weight_map;
    string line;
    std::ifstream f(weight_file.string(), std::ifstream::in);

    while (getline(f, line)) {
        // convert line to a string stream for easy parsing
        istringstream ss(line);
        string name;
        double weight;

        if (!(ss >> name >> weight)) {
            BOOST_LOG(mylog)
                << "Bad format in exemplar weight file. Dying now.";
            assert(!("Bad format in exemplar weight file. Dying now."));
        }

        weight_map.insert(pair<string, double>(name, weight));
    }

    return weight_map;
}

///////////////////////////////////////////////////////////////////////
// Corpus File Operators
///////////////////////////////////////////////////////////////////////

bool CorpusFile::operator==(const CorpusFile &other) const {
    return (this->file_path == other.file_path) &&
           (this->file_size == other.file_size);
}

bool CorpusFile::operator<(const CorpusFile &other) const {
    return this->file_size < other.file_size;
}

bool CorpusFile::operator<=(const CorpusFile &other) const {
    return this->file_size <= other.file_size;
}

bool CorpusFile::operator>(const CorpusFile &other) const {
    return this->file_size > other.file_size;
}

bool CorpusFile::operator>=(const CorpusFile &other) const {
    return this->file_size >= other.file_size;
}

bool CorpusFile::operator!=(const CorpusFile &other) const {
    return !(*this == other);
}
