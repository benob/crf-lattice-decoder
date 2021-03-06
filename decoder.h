#include <fst/fstlib.h>
#include <tr1/unordered_map>
#include <list>
#include "model.h"
#include "utils.h"

namespace macaon {

    typedef int64 State;

    struct Decoder {

        CRFModel model;

        Decoder() {}
        Decoder(const std::string &filename) { model.readCRFPPTextModel(filename); }

        void loadAutomaton(fst::StdVectorFst &automaton, std::vector<std::vector<std::string> > &features, bool hasTags = false) {
            fst::SymbolTable states("states");
            fst::SymbolTable symbols("features");
            symbols.AddSymbol("<eps>", 0);
            while(true) {
                std::string line;
                std::getline(std::cin, line);
                if(std::cin.eof()) break;
                std::vector<std::string> tokens;
                Tokenize(line, tokens, " \t");
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
                    int64 isymbol = 0, osymbol = 0;
                    if(hasTags) {
                        isymbol = symbols.Find(columns);
                        if(isymbol == -1) {
                            features.push_back(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
                            isymbol = symbols.AddSymbol(columns, features.size());
                        }
                        std::tr1::unordered_map<std::string, int>::const_iterator tag = model.labels.find(tokens[tokens.size() - 1]);
                        if(tag != model.labels.end()) {
                            osymbol = tag->second + 1;
                        } else {
                            std::cerr << "error: unknown tag \"" << tokens[tokens.size() - 1] << "\" found in input\n";
                            exit(1);
                        }
                    } else {
                        isymbol = symbols.Find(columns);
                        if(isymbol == -1) {
                            features.push_back(std::vector<std::string>(tokens.begin() + 2, tokens.end()));
                            isymbol = symbols.AddSymbol(columns, features.size());
                        }
                        osymbol = isymbol;
                    }
                    automaton.AddArc(fromState, fst::StdArc(isymbol, osymbol, 0, toState));
                }
            }
            automaton.SetInputSymbols(&symbols);
            //automaton.SetOutputSymbols(&symbols);
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
            void getContextFeaturesOLabel(std::vector<int> &output) const {
                for(size_t i = 0; i < shift; i++) {
                    output.push_back(-1);
                }
                for(std::list<fst::StdArc>::const_iterator i = seq.begin(); i != seq.end(); i++) {
                    output.push_back(i->olabel - 1);
                }
                for(size_t i = 0; i < length - seq.size() - shift; i++) {
                    output.push_back(-1);
                }
            }
            void print(CRFModel &model, const std::vector<std::vector<string> > & features) {
                cerr << "state: in=" << inputState << " out=" << outputState << " labels:";
                std::vector<int> labels; 
                getContextFeatures(labels);
                for(std::vector<int>::const_iterator i = labels.begin(); i != labels.end(); i++) {
                    cerr << " " << *i;
                }
                cerr << " scores:";
                std::vector<double> scores = model.emissions(features, labels);
                for(std::vector<double>::const_iterator i = scores.begin(); i != scores.end(); i++) {
                    cerr << " " << *i;
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
                //cerr << "consume: shift=" << shift << " seq.size=" << seq.size() << endl;
                if(shift > 0) shift--;
                else if(seq.size() > 0) seq.pop_front();
                //cerr << "> : shift=" << shift << " seq.size=" << seq.size() << endl;
            }
            struct LabelsHash {
                size_t operator()(const Context& a) const {
                    size_t output = a.inputState;
                    std::list<fst::StdArc>::const_iterator i = a.seq.begin(); 
                    if(a.seq.size() == a.length) i++;
                    for(; i != a.seq.end(); i++) output ^= i->ilabel ^ i->olabel;
                    output ^= a.shift;
                    return output;
                }
            };
            struct LabelsEqual {
                int operator()(const Context& a, const Context& b) const {
                    if(a.inputState != b.inputState) return false;
                    if(a.seq.size() != b.seq.size()) return false;
                    if(a.shift != b.shift) return false;
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
                //cerr << "ilabel: shift=" << shift << " window_offset=" << window_offset << " seq.size=" << seq.size() << "\n";
                if(shift > window_offset || seq.size() + shift <= window_offset) return 0; // epsilon transition
                std::list<fst::StdArc>::const_iterator i = seq.begin();
                for(size_t j = 0; j < window_offset - shift; j++) {
                    i++;
                }
                //return seq[window_offset + shift].ilabel;
                return i->ilabel;
            }
            int64 olabel(size_t window_offset) const {
                if(shift > window_offset || seq.size() + shift <= window_offset) return 0; // epsilon transition
                std::list<fst::StdArc>::const_iterator i = seq.begin();
                for(size_t j = 0; j < window_offset - shift; j++) {
                    i++;
                }
                return i->olabel;
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
                //current.print(model, features);
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
                    int64 ilabel = next.ilabel(model.window_offset);
                    //std::cerr << "arc.ilabel=" << arc.ilabel << " next.ilabel=" << ilabel << " window_offset=" << model.window_offset << " next.shift=" << next.shift << std::endl;
                    if(ilabel == 0) {
                        output.AddArc(arcStartState, fst::StdArc(0, 0, 0, arcEndState));
                    } else {
                        if(rescore) {
                            std::vector<int> context_tags;
                            next.getContextFeaturesOLabel(context_tags);
                            double score = model.rescore(features, context_features, context_tags);
                            output.AddArc(arcStartState, fst::StdArc(next.ilabel(model.window_offset), next.olabel(model.window_offset), -score, arcEndState));
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
                        if(current.seq.size() == 0) {
                            output.SetFinal(arcStartState, input.Final(inputState));
                            continue;
                        }
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
                        //std::cerr << " current.ilabel=" << ilabel << " window_offset=" << model.window_offset << " current.shift=" << current.shift << std::endl;
                        if(ilabel == 0) {
                            output.AddArc(arcStartState, fst::StdArc(0, 0, 0, arcEndState));
                        } else {
                            if(rescore) {
                                std::vector<int> context_tags;
                                current.getContextFeaturesOLabel(context_tags);
                                double score = model.rescore(features, context_features, context_tags);
                                output.AddArc(arcStartState, fst::StdArc(current.ilabel(model.window_offset), current.olabel(model.window_offset), -score, arcEndState));
                            } else {
                                std::vector<double> scores = model.emissions(features, context_features);
                                for(size_t label = 0; label < scores.size(); label++) {
                                    //if(scores[label] != 0 && label != 0)
                                    output.AddArc(arcStartState, fst::StdArc(ilabel, label + 1, -scores[label], arcEndState));
                                }
                            }
                        }
                    } else {
                        output.SetFinal(arcStartState, input.Final(inputState));
                    }
                }
            }
            fst::SymbolTable labels("labels");
            labels.AddSymbol("<eps>", 0);
            for(std::tr1::unordered_map<std::string, int>::const_iterator label = model.labels.begin(); label != model.labels.end(); label++) {
                labels.AddSymbol(label->first, label->second + 1);
            }
            if(!rescore) {
                fst::StdVectorFst transitions;
                for(size_t label = 0; label < model.labels.size() + 1; label++) {
                    transitions.AddState();
                    if(label > 0) {
                        transitions.SetFinal(label, 0);
                    }
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
                output.SetOutputSymbols(&labels);
            }
        }

    };
}
