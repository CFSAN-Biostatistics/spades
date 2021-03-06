//***************************************************************************
//* Copyright (c) 2015 Saint Petersburg State University
//* Copyright (c) 2011-2014 Saint Petersburg Academic University
//* All Rights Reserved
//* See file LICENSE for details.
//***************************************************************************

#include "utils/standard_base.hpp"
#include "utils/simple_tools.hpp"
#include "utils/logger/log_writers.hpp"

#include "utils/path_helper.hpp"
#include "io/reads/file_reader.hpp"
#include "io/reads/wrapper_collection.hpp"
#include "io/reads/osequencestream.hpp"

void create_console_logger() {
    logging::logger *log = logging::create_logger("", logging::L_INFO);
    log->add_writer(std::make_shared<logging::console_writer>());
    logging::attach_logger(log);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        cout << "Usage: reference_fixer <reference path> <output path>" << endl;
        return 1;
    }
    create_console_logger();
    string fn = argv[1];
    path::CheckFileExistenceFATAL(fn);
    string out_fn = argv[2];
    auto reader = make_shared<io::NonNuclCollapsingWrapper>(make_shared<io::FileReadStream>(fn));
    io::SingleRead read;
    std::stringstream ss;
    while (!reader->eof()) {
        (*reader) >> read;
        ss << read.GetSequenceString();
    }
    io::SingleRead concat("concat", ss.str());
    io::osequencestream out(out_fn);
    out << concat;
    return 0;
}
