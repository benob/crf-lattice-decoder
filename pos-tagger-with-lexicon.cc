#include <fst/fstlib.h>
#include <fst/script/fstscript.h>
#include "decoder.h"
#include "features.h"
#include "lexicon.h"

using namespace macaon;
using namespace fst;

struct OutputMapper {
    typedef StdArc FromArc;
    typedef StdArc ToArc; 
    const std::vector<std::vector<std::string> > &features;
    SymbolTable symbols;
    OutputMapper(std::vector<std::vector<std::string> > &_features) : features(_features), symbols("words") {
        symbols.AddSymbol("<eps>");
    }
    StdArc operator()(const StdArc &arc) {
        if(arc.ilabel == 0) {
            return arc;
        } else {
            int64 ilabel = symbols.AddSymbol(features[arc.ilabel - 1][0]);
            return StdArc(ilabel, arc.olabel, arc.weight, arc.nextstate);
        }
    }
    MapFinalAction FinalAction() const { return MAP_NO_SUPERFINAL; } 
    MapSymbolsAction InputSymbolsAction() const { return MAP_CLEAR_SYMBOLS; }
    MapSymbolsAction OutputSymbolsAction() const { return MAP_COPY_SYMBOLS; } 
    uint64 Properties(uint64 props) const { return props; }
};

int main(int argc, char** argv) {
    if(argc != 3) {
        std::cerr << "usage: " << argv[0] << " <crfpp-text-model> <lexicon>\n";
        return 1;
    }
    StdVectorFst input;
    StdVectorFst output;
    std::vector<std::vector<std::string> > words;
    std::vector<std::vector<std::string> > features;
    Decoder decoder(argv[1]);
    Lexicon lexicon(argv[2]);
    decoder.loadAutomaton(input, words, false);
    lexicon.AddTags(input);
    for(std::vector<std::vector<std::string> >::const_iterator word = words.begin(); word != words.end(); word++) {
        std::vector<std::string> word_features;
        FeatureGenerator::get_pos_features((*word)[0], word_features);
        features.push_back(word_features);
    }
    decoder.decode(features, input, output, true);
    output.Write("");
}
