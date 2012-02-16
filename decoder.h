#include <fst/fstlib.h>
#include <tr1/unordered_map>
#include <list>
#include "model.h"

namespace macaon {

    typedef int64 State;

    // http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
    static void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " ")
    {
        std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
        while (std::string::npos != pos || string::npos != lastPos)
        {
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            lastPos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, lastPos);
        }
    }

    struct Decoder {

        CRFModel model;

        void loadAutomaton(fst::StdVectorFst &automaton, std::vector<std::vector<std::string> > &features) {
            fst::SymbolTable states("states");
            fst::SymbolTable symbols("features");
            while(true) {
                std::string line;
                std::getline(std::cin, line);
                if(std::cin.eof()) break;
                std::vector<std::string> tokens;
                tokenize(line, tokens, " \t");
                if(tokens.size() == 1) {
                    int64 fromState = states.Find(tokens[0]);
                    if(fromState == -1) {
                        fromState = automaton.AddState();
                        states.AddSymbol(tokens[0], fromState);
                    }
                    automaton.SetFinal(fromState, 0);
                } else {
                    int64 fromState = states.Find(tokens[0]);
                    if(fromState == -1) {
                        fromState = automaton.AddState();
                        states.AddSymbol(tokens[0], fromState);
                    }
                    int64 toState = states.Find(tokens[1]);
                    if(toState == -1) {
                        toState = automaton.AddState();
                        states.AddSymbol(tokens[1], toState);
                    }
                    const std::string columns = line.substr(tokens[0].length() + tokens[1].length() + 2);  // WARNING: assume single spaces
                    int64 symbol = symbols.Find(columns);
                    if(symbol == -1) {
                        features.push_back(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
                        symbol = symbols.AddSymbol(columns, features.size());
                    }
                    automaton.AddArc(fromState, fst::StdArc(symbol, symbol, 0, toState));
                }
            }
            automaton.SetInputSymbols(&symbols);
            automaton.SetOutputSymbols(&symbols);
            automaton.SetStart(0);
        }

        struct Context {
            State inputState;
            State outputState;
            std::list<fst::StdArc> seq;
            size_t length;
            Context() {}
            Context(State _inputState, State _outputState, size_t _length) : inputState(_inputState), outputState(_outputState), length(_length) {}
            Context& operator=(const Context& other) {
                inputState = other.inputState;
                outputState = other.outputState;
                length = other.length;
                seq = other.seq;
                return *this;
            }
            void print() {
                cerr << "state: in=" << inputState << " out=" << outputState << " labels:";
                for(std::list<fst::StdArc>::const_iterator i = seq.begin(); i != seq.end(); i++) {
                    cerr << " " << i->ilabel;
                }
                cerr << endl;
            }
            void push(const fst::StdArc& arc) {
                seq.push_back(arc);
                inputState = arc.nextstate;
                if(seq.size() > length) seq.pop_front();
            }
            struct LabelsHash {
                size_t operator()(const Context& a) const {
                    size_t output = a.inputState;
                    std::list<fst::StdArc>::const_iterator i = a.seq.begin(); 
                    if(a.seq.size() == a.length) i++;
                    for(; i != a.seq.end(); i++) output ^= i->ilabel ^ i->olabel;
                    return output;
                }
            };
            struct LabelsEqual {
                int operator()(const Context& a, const Context& b) const {
                    if(a.inputState != b.inputState) return false;
                    if(a.seq.size() != b.seq.size()) return false;
                    std::list<fst::StdArc>::const_iterator i = a.seq.begin();
                    std::list<fst::StdArc>::const_iterator j = b.seq.begin();
                    if(a.seq.size() == a.length) i++;
                    if(b.seq.size() == b.length) j++;
                    for(; i != a.seq.end() && j != b.seq.end(); i++, j++) {
                        if(i->ilabel != j->ilabel) return false;
                        if(i->olabel != j->olabel) return false;
                    }
                    return true;
                }
            };
            fst::StdArc::Weight weight() const {
                return seq.back().weight;
            }
            int64 ilabel() const {
                return seq.back().ilabel;
            }
            int64 olabel() const {
                return seq.back().ilabel;
            }
        };

        void decode(const std::vector<std::vector<std::string> > &features, const fst::StdVectorFst &input, fst::StdVectorFst &output) {

            std::tr1::unordered_map<Context, State, Context::LabelsHash, Context::LabelsEqual> outputStates;
            std::list<Context> queue; // queue all unprocessed contexts

            State outputStart = 0;
            output.AddState();
            output.SetStart(outputStart);
            queue.push_back(Context(input.Start(), outputStart, model.window_length));

            while(queue.size() > 0) {
                Context current = queue.front();
                queue.pop_front();
                State inputState = current.inputState;
                State arcStartState = current.outputState;
                for(fst::ArcIterator<fst::StdVectorFst> aiter(input, inputState); !aiter.Done(); aiter.Next()) {
                    const fst::StdArc &arc = aiter.Value();
                    Context next = current;
                    next.push(arc);
                    std::tr1::unordered_map<Context, State>::iterator found = outputStates.find(next);
                    int arcEndState = output.NumStates();
                    if(found == outputStates.end()) {
                        outputStates[next] = arcEndState;
                        output.AddState();
                        next.outputState = arcEndState;
                        queue.push_back(next);
                    } else {
                        arcEndState = found->second;
                    }
                    if(input.Final(arc.nextstate) != arc.weight.Zero()) {
                        output.SetFinal(arcEndState, input.Final(arc.nextstate));
                    }
                    std::vector<int> context_features;
                    for(size_t i = 0; i < model.window_length - next.seq.size(); i++) {
                        context_features.push_back(-1);
                    }
                    for(std::list<fst::StdArc>::const_iterator i = next.seq.begin(); i != next.seq.end(); i++) {
                        context_features.push_back(i->ilabel - 1);
                    }
                    double score = model.score(features, context_features);
                    //next.print();
                    //std::cerr << score << std::endl;
                    output.AddArc(arcStartState, fst::StdArc(next.ilabel(), next.olabel(), score, arcEndState));
                }
            }
            output.SetInputSymbols(input.InputSymbols());
            output.SetOutputSymbols(input.OutputSymbols());
        }

    };
}
