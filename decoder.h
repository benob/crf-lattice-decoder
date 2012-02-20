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

        Decoder() {}
        Decoder(const std::string &filename) { model.readCRFPPTextModel(filename); }

        void loadAutomaton(fst::StdVectorFst &automaton, std::vector<std::vector<std::string> > &features) {
            fst::SymbolTable states("states");
            fst::SymbolTable symbols("features");
            symbols.AddSymbol("<eps>", 0);
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
            size_t shift; // number of missing arcs at start state
            Context() {}
            Context(State _inputState, State _outputState, size_t _length) : inputState(_inputState), outputState(_outputState), length(_length), shift(_length) {} // seq is empty
            Context& operator=(const Context& other) {
                inputState = other.inputState;
                outputState = other.outputState;
                length = other.length;
                seq = other.seq;
                shift = other.shift;
                return *this;
            }
            void getContextFeatures(std::vector<int> &output) const {
                for(size_t i = 0; i < shift; i++) {
                    output.push_back(-1);
                }
                for(std::list<fst::StdArc>::const_iterator i = seq.begin(); i != seq.end(); i++) {
                    output.push_back(i->ilabel - 1);
                }
                for(size_t i = 0; i < length - seq.size() - shift; i++) {
                    output.push_back(-1);
                }
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
                if(shift > 0) shift--;
                if(seq.size() > length) seq.pop_front();
            }
            void consume() {
                if(shift > 0) shift--;
                else if(seq.size() > 0) seq.pop_front();
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
            int64 ilabel(size_t window_offset) const {
                if(shift > window_offset || seq.size() <= window_offset) return 0; // epsilon transition
                std::list<fst::StdArc>::const_iterator i = seq.begin();
                for(size_t j = 0; j < window_offset - shift; j++) {
                    i++;
                }
                //return seq[window_offset + shift].ilabel;
                return i->ilabel;
            }
            int64 olabel(size_t window_offset) const {
                // TODO: incorrect
                return seq.back().ilabel;
            }
        };

        void decode(const std::vector<std::vector<std::string> > &features, const fst::StdVectorFst &input, fst::StdVectorFst &output, bool rescore=false) {

            std::tr1::unordered_map<Context, State, Context::LabelsHash, Context::LabelsEqual> outputStates;
            std::list<Context> queue; // queue all unprocessed contexts

            State outputStart = 0;
            output.AddState();
            output.SetStart(outputStart);
            queue.push_back(Context(input.Start(), outputStart, model.window_length));

            while(queue.size() > 0) {
                Context current = queue.front();
                current.print();
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
                    /*if(input.Final(arc.nextstate) != fst::StdArc::weight::Zero()) {
                        output.SetFinal(arcEndState, input.Final(arc.nextstate));
                    }*/
                    std::vector<int> context_features;
                    next.getContextFeatures(context_features);
                    if(rescore) {
                        double score = model.score(features, context_features);
                        output.AddArc(arcStartState, fst::StdArc(next.ilabel(model.window_offset), next.olabel(model.window_offset), -score, arcEndState));
                    } else {
                        int64 ilabel = next.ilabel(model.window_offset);
                        std::cerr << "arc.ilabel=" << arc.ilabel << " next.ilabel=" << ilabel << " window_offset=" << model.window_offset << " next.shift=" << next.shift << std::endl;
                        if(ilabel == 0) {
                            output.AddArc(arcStartState, fst::StdArc(0, 0, 0, arcEndState));
                        } else {
                            std::vector<double> scores = model.emissions(features, context_features);
                            for(size_t label = 0; label < scores.size(); label++) {
                                //if(scores[label] != 0 && label != 0)
                                    output.AddArc(arcStartState, fst::StdArc(ilabel, label + 1, -scores[label], arcEndState));
                            }
                        }
                    }
                }
                if(input.Final(inputState) != fst::StdArc::Weight::Zero()) {
                    if(current.seq.size() > 0) {
                        current.consume();
                        std::tr1::unordered_map<Context, State>::iterator found = outputStates.find(current);
                        int arcEndState = output.NumStates();
                        if(found == outputStates.end()) {
                            outputStates[current] = arcEndState;
                            output.AddState();
                            current.outputState = arcEndState;
                            queue.push_back(current);
                        } else {
                            arcEndState = found->second;
                        }
                        std::vector<int> context_features;
                        current.getContextFeatures(context_features);
                        int64 ilabel = current.ilabel(model.window_offset);
                        std::cerr << " current.ilabel=" << ilabel << " window_offset=" << model.window_offset << " current.shift=" << current.shift << std::endl;
                        if(ilabel == 0) {
                            output.AddArc(arcStartState, fst::StdArc(0, 0, 0, arcEndState));
                        } else {
                            std::vector<double> scores = model.emissions(features, context_features);
                            for(size_t label = 0; label < scores.size(); label++) {
                                //if(scores[label] != 0 && label != 0)
                                output.AddArc(arcStartState, fst::StdArc(ilabel, label + 1, -scores[label], arcEndState));
                            }
                        }
                    } else {
                        output.SetFinal(arcStartState, input.Final(inputState));
                    }
                }
            }
            if(!rescore) {
                fst::StdVectorFst transitions;
                for(size_t label = 0; label < model.labels.size() + 1; label++) {
                    transitions.AddState();
                    if(label > 0) {
                        transitions.SetFinal(label, 0);
                    }
                }
                fst::SymbolTable labels("labels");
                labels.AddSymbol("<eps>", 0);
                for(std::tr1::unordered_map<std::string, int>::const_iterator label = model.labels.begin(); label != model.labels.end(); label++) {
                    labels.AddSymbol(label->first, label->second + 1);
                }
                transitions.SetStart(0);
                for(size_t label = 0; label < model.labels.size(); label++) {
                    transitions.AddArc(0, fst::StdArc(label + 1, label + 1, 0, label + 1));
                    for(size_t previous = 0; previous < model.labels.size(); previous++) {
                        double score = model.transition(previous, label);
                        //if(score != 0 && previous != 0) 
                        transitions.AddArc(previous + 1, fst::StdArc(label + 1, label + 1, -score, label + 1));
                    }
                }
                output.SetInputSymbols(input.InputSymbols());
                //output.SetOutputSymbols(&labels);
                transitions.SetOutputSymbols(&labels);
                fst::StdComposeFst result(output, transitions); // TODO
                output = result;
            } else {
                output.SetInputSymbols(input.InputSymbols());
                output.SetOutputSymbols(input.OutputSymbols());
            }
        }

    };
}
