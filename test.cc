#include <iostream>
#include "decoder.h"
#include "model.h"
#include "lexicon.h"

using namespace macaon;
using namespace std;

int main(int argc, char** argv) {
    /*CRFModel model;
    model.readCRFPPTextModel("../crf_eval/chunker/chunking.model.txt");
    cout << model.window_offset << " " << model.window_length << endl;
    for(std::vector<CRFPPTemplate>::const_iterator i = model.templates.begin(); i != model.templates.end(); i++) {
        cout << i->items.size() << " " << *i << endl;
    }*/
    /*CRFModel model;
    model.trivialModel();

    vector<vector<string> > features;
    vector<string> first;
    first.push_back("a");
    first.push_back("1");
    features.push_back(first);
    vector<string> second;
    second.push_back("b");
    second.push_back("0");
    features.push_back(second);
    vector<int> clique;
    clique.push_back(1);

    double score = model.score(features, clique);
    cout << score << endl;*/

    /*fst::StdVectorFst input;
    fst::StdVectorFst output;
    std::vector<std::vector<std::string> > features;
    Decoder decoder;
    decoder.model.trivialModel();
    decoder.loadAutomaton(input, features);
    decoder.decode(features, input, output, false);
    output.Write("");*/

    // test tag lexicon
    Lexicon lexicon("../macaon-fr-word-tag.txt");
    const std::vector<int64> tags = lexicon.GetTagsForWord(19222);
    for(std::vector<int64>::const_iterator i = tags.begin(); i != tags.end(); i++) {
        std::cout << *i << endl;
    }
}
