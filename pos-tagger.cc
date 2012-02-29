#include <fst/fstlib.h>
#include <fst/script/fstscript.h>
#include "decoder.h"
#include "features.h"

using namespace macaon;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "usage: " << argv[0] << " <crfpp-text-model>\n";
        return 1;
    }
    fst::StdVectorFst input;
    fst::StdVectorFst output;
    std::vector<std::vector<std::string> > words;
    std::vector<std::vector<std::string> > features;
    Decoder decoder(argv[1]);
    decoder.loadAutomaton(input, words);
    for(std::vector<std::vector<std::string> >::const_iterator word = words.begin(); word != words.end(); word++) {
        std::vector<std::string> word_features;
        FeatureGenerator::get_pos_features((*word)[0], word_features);
        /*for(std::vector<std::string>::const_iterator feature = word_features.begin(); feature != word_features.end(); feature++) {
            std::cerr << *feature << " ";
        }
        std::cerr << std::endl;*/
        features.push_back(word_features);
    }
    decoder.decode(features, input, output, false);
    fst::StdVectorFst best;
    fst::ShortestPath(output, &best);
    fst::RmEpsilon(&best);
    fst::TopSort(&best);
    fst::script::PrintFst(best, std::cout, "stdout", output.InputSymbols(), output.OutputSymbols());
    //output.Write("");
}
