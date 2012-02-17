#include "decoder.h"
#include "model.h"
#include <iostream>

using namespace macaon;
using namespace std;

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "usage: " << argv[0] << " <crfpp-text-model>\n";
        return 1;
    }
    fst::StdVectorFst input;
    fst::StdVectorFst output;
    std::vector<std::vector<std::string> > features;
    Decoder decoder(argv[1]);
    decoder.loadAutomaton(input, features);
    decoder.decode(features, input, output, false);
    output.Write("");
}
