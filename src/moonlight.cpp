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
 * \date February 2015
 *
 * \author Liam Hayes + Jonathan Milford
 * \date Feb 2017
 */

#include <chrono>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/program_options.hpp>

#include "ExemplarData.h"
#include "Matrix.h"
#include "OSCPSolver.h"
#include "moonlight.h"

using namespace std;
using namespace boost::filesystem;
using namespace boost;
using namespace std::chrono;
namespace keywords = boost::log::keywords;
namespace logging = boost::log;
namespace po = boost::program_options;
namespace sinks = boost::log::sinks;
namespace src = boost::log::sources;

/**
 * \brief Utility function to process the command line arguments and set
 * default behaviours.
 */
void command_line_processing(int argc, char **argv);

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

static string pattern;
static path directory;
static path matrixfile;
static path resultfile;
static path analyticsfile;
static string runname;
static path weight_file;
static bool ignore_matrixfile;
static bool large_data;
static bool greedy;

/////////////////////////////////////////////////////////////////////////////
// Utility Functions
/////////////////////////////////////////////////////////////////////////////

/** \brief Configure the logger */
BOOST_LOG_GLOBAL_LOGGER_INIT(my_logger, src::logger_mt) {
    src::severity_logger<> lg(keywords::severity = info);
    logging::add_file_log(keywords::file_name = "moonlight.log",
                          keywords::auto_flush = true,
                          keywords::open_mode = std::ios::out,
                          keywords::format = "[%TimeStamp%] %Message%");
    logging::add_common_attributes();
    logging::add_console_log(std::cout, boost::log::keywords::format =
                                            "[%TimeStamp%] %Message%");

    return lg;
}

/////////////////////////////////////////////////////////////////////////////
// moonlight Main
/////////////////////////////////////////////////////////////////////////////

/**
 * Main entry point.
 *
 * \param argc number of command line arguments
 * \param argv array of character arrays
 * \return program termination status. Zero is normal termination.
 */
int main(int argc, char **argv) {
    // set up logging and other external variables
    src::severity_logger<> &mylog = my_logger::get();

    // set up command line options
    command_line_processing(argc, argv);

    ////////////////////////////////////////////////////////////////
    // Start processing...
    ////////////////////////////////////////////////////////////////

    OSCPSolver solver;
    Matrix matrix;

    // parse data into a matrix
    if (!ignore_matrixfile && exists(matrixfile) &&
        is_regular_file(matrixfile)) {
        // if we've serialised it before use that...
        BOOST_LOG(mylog) << "Matrix data appears to have been serialised to "
                         << "disk. Attempting to restore it.";
        Matrix dmat(matrixfile); // construct from file
        BOOST_LOG(mylog) << "Matrix restored from file.";
        matrix = dmat; // move assign
    } else {
        // we need to parse the whole corpus to construct the matrix
        BOOST_LOG(mylog) << "Constructing matrix from corpus data";
        INDEX_LIST cols_to_ignore;

        if (large_data && !greedy) {
            cols_to_ignore =
                solver.calc_cols_to_ignore(directory, pattern, weight_file);
        }

        Matrix dmat(directory, pattern, weight_file,
                    cols_to_ignore); // construct from corpus data
        BOOST_LOG(mylog) << "Finished constructing matrix from corpus data";

        if (!ignore_matrixfile) {
            // having just constructed it - serialise to disk so we don't need
            // to do this again
            BOOST_LOG(mylog) << "Serialising matrix to disk for future "
                             << "possible use";
            std::ofstream matrix_file_out(matrixfile.native());
            boost::archive::binary_oarchive sout(matrix_file_out);
            BOOST_LOG(mylog) << "Writing matrix data to file for archiving...";
            // write class instance to archive
            // getting serialisation error since converting rowlist from a list
            // to a vector
            // see git log for when that was. TODO: fix this
            BOOST_LOG(mylog) << "Finished Writing matrix data to file.";
        }
        matrix = dmat; // move assign
        // archive and stream closed when destructors are called
    }

    // we want a place to store all the meta-data about each of the exemplars
    // we are interested in - the corpus analytics data. The matrix we have just
    // instantiated has some of the information and as we later run our
    // distillation algorithms over the matrix we will collect more.
    BOOST_LOG(mylog) << "Constructing a corpus analytics store";
    CORPUS_DATA corpus_data = initialise_corpus_data(matrix);

    // we have a useful data matrix
    BOOST_LOG(mylog) << "Solving for optimised set cover...";
    Solution result =
        solver.solve_oscp(matrix, corpus_data, runname, greedy, weight_file);

    BOOST_LOG(mylog) << "Writing corpus distillation solution to "
                     << resultfile;
    result.json_print(resultfile);
    BOOST_LOG(mylog) << "Finished writing solution";

    BOOST_LOG(mylog) << "Writing corpus analytics data to " << analyticsfile;
    csv_print(analyticsfile, corpus_data);
    BOOST_LOG(mylog) << "Finished writing analytics";

    BOOST_LOG(mylog) << "End ";

    return EXIT_SUCCESS;
}

////////////////////////////////////////////////////////////////
// Command-Line Processing
////////////////////////////////////////////////////////////////

void command_line_processing(int argc, char **argv) {
    // set up logging and other external variables
    src::severity_logger<> &mylog = my_logger::get();

    // set up command line options
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Produce help message")(
        "directory,d", po::value<string>(),
        "Path to the corpus directory containing the exemplars")(
        "name,n", po::value<string>(), "User defined name for this run")(
        "pattern,r", po::value<string>(),
        "Regex pattern for corpus files in directory")(
        "matrix,m", po::value<string>(),
        "File name to use when loading or saving matrix data on disk")(
        "analytics,a", po::value<string>(),
        "File name to use for storing corpus analytics")(
        "ignore-matrix,i",
        "Ignore an existing matrix data file and do not "
        "serialise matrix to file. Just load matrix from raw "
        "data.")("weighted,w", po::value<string>(),
                 "Absolute path to the file containing the exemplar weights")(
        "large-data,l",
        "Use less memory, matrix data will be too large in sparse form")(
        "greedy,g", "Apply the standard greedy algorithm");

    // process the command line options
    po::variables_map vm; // command line variable map
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    // set control variables
    if (vm.count("help")) {
        cout << desc << endl;
        exit(EXIT_SUCCESS);
    }

    if (vm.count("directory")) {
        directory = path(vm["directory"].as<string>());
        BOOST_LOG(mylog) << "Corpus directory is :" << directory;
    } else {
        directory = current_path();
        BOOST_LOG(mylog) << "Directory path was not set. Defaulting to "
                         << directory;
    }

    if (vm.count("name")) {
        runname = vm["name"].as<string>();
        BOOST_LOG(mylog) << "This run is named :" << runname;
    } else {
        runname = "moonlight";
        BOOST_LOG(mylog) << "Run name is not set. Defaulting to " << runname;
    }

    if (vm.count("pattern")) {
        pattern = vm["pattern"].as<string>();
        BOOST_LOG(mylog) << "Regex pattern for corpus files is : " << pattern;
    } else {
        pattern = "exemplar_";
        BOOST_LOG_SEV(mylog, info) << "Regex pattern for corpus files was not "
                                   << "set. Defaulting to : " << pattern;
    }

    if (vm.count("matrix")) {
        matrixfile = directory / path(vm["matrix"].as<string>() + ".matrix");
        BOOST_LOG(mylog) << "Loading (or saving) matrix data to file :"
                         << matrixfile;
    } else {
        matrixfile = directory / path(runname + ".matrix");
        BOOST_LOG(mylog) << "Matrix file name was not set. Defaulting to : "
                         << matrixfile;
    }

    if (vm.count("analytics")) {
        analyticsfile = directory / path(vm["analytics"].as<string>() + ".csv");
        BOOST_LOG(mylog) << "Saving corpus analytics to file :"
                         << analyticsfile;
    } else {
        analyticsfile = directory / path(runname + "_analytics.csv");
        BOOST_LOG(mylog) << "Corpus analytics file name was not set. "
                         << "Defaulting to : " << analyticsfile;
    }

    if (vm.count("ignore-matrix")) {
        ignore_matrixfile = true;
        BOOST_LOG(mylog) << "Ignoring any pre-existing matrix data file if it "
                         << "exists and not writing matrix to file";
    } else {
        ignore_matrixfile = false;
        BOOST_LOG(mylog) << "Will load a pre-existing matrix data file if it "
                         << "exists and write one if it doesn't";
    }

    if (vm.count("weighted")) {
        weight_file = path(vm["weighted"].as<string>());
    } else {
        weight_file = path(); // empty path indicates unweighted version
    }

    if (vm.count("large-data")) {
        large_data = true;
        BOOST_LOG(mylog) << "Using less memory by eliminating columns of row "
                         << "unitarians before full read in";
    } else {
        large_data = false;
        BOOST_LOG(mylog) << "Data not too large in sparse format, will read "
                         << "in as normal";
    }

    if (vm.count("greedy")) {
        greedy = true;
        BOOST_LOG(mylog) << "Using the Greedy Algorithm";
        if (large_data) {
            BOOST_LOG(mylog) << "     Note: Can't save memory (largedata == "
                             << "true) when using greedy";
        }
    } else {
        BOOST_LOG(mylog) << "Using the Reduction Algorithm";
    }

    // run name configuration of file names
    resultfile = directory / path(runname + "_solution.json");
    BOOST_LOG(mylog) << "Storing solution in file :" << resultfile;
}
