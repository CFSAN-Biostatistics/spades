#include "cute.h"
#include "ide_listener.h"
#include "cute_runner.h"
#include "seq_test.hpp"
#include "sequence_test.hpp"
#include "quality_test.hpp"
#include "nucl_test.hpp"
#include "ireadstream_test.hpp"
#include "online_graph_visualizer_test.hpp"
#include "similar_test.hpp"
#include "cuckoo_test.hpp"
#include "single_read_test.hpp"
#include "paired_read_test.hpp"
#include "parser_test.hpp"
#include "fastqgz_parser_test.hpp"
// TODO(mariyafomkina): Add tests for other parsers here.
#include "reader_singleread_test.hpp"
#include "reader_pairedread_test.hpp"
#include "multifile_reader_wrapper_test.hpp"


void runSuite() {
  cute::suite s;
  //TODO add your test here
  s += SeqSuite();
  s += SequenceSuite();
  s += QualitySuite();
  s += NuclSuite();
  s += IReadStreamSuite();
  s += onlineGraphVisualizerSuite();
  s += similarSuite();
  s += CuckooSuite();
  s += SingleReadSuite();
  s += PairedReadSuite();
  s += ParserSuite();
  s += FastqgzParserSuite();
  // TODO(mariyafomkina): Add tests for other parsers here.
  s += ReaderSingleReadSuite();
  s += ReaderPairedReadSuite();
  s += MultifileReaderWrapperSuite();
  cute::ide_listener lis;
  cute::makeRunner(lis)(s, "The Suite");
}

int main() {
  runSuite();
  return 0;
}
