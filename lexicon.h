#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <tr1/unordered_map>
#include <fst/fstlib.h>
#include "utils.h"

namespace macaon {
    class Lexicon {
        bool loaded;

        fst::SymbolTable wordSymbols;
        fst::SymbolTable tagSymbols;

        std::vector<std::vector<int64> > tagsForWord;
        std::tr1::unordered_map<int64, int> tagsForWordEntry;

    public:
        Lexicon() : loaded(false), wordSymbols("words"), tagSymbols("tags") {
            wordSymbols.AddSymbol("<eps>", 0);
            tagSymbols.AddSymbol("<eps>", 0);
        }

        Lexicon(const std::string& filename) : loaded(false), wordSymbols("words"), tagSymbols("tags") {
            wordSymbols.AddSymbol("<eps>", 0);
            tagSymbols.AddSymbol("<eps>", 0);
            Load(filename);
        }

        const fst::SymbolTable& WordSymbols() const {
            return wordSymbols;
        }

        const fst::SymbolTable& TagSymbols() const {
            return tagSymbols;
        }

        bool Load(const std::string &filename) {
            tagsForWord.push_back(std::vector<int64>()); // keep space for unk word tags
            loaded = false;
            std::tr1::unordered_map<std::string, int> known;
            std::ifstream input(filename.c_str());
            if(!input.is_open()) {
                std::cerr << "ERROR: could not open " << filename << " in Lexicon::Load()" << std::endl;
                return false;
            }
            while(!input.eof()) {
                std::string line;
                std::getline(input, line);
                if(input.eof()) break;
                std::string word;
                std::string::size_type end_of_word = line.find('\t');
                word = line.substr(0, end_of_word);
                int64 wordId = wordSymbols.AddSymbol(word);
                std::string signature = line.substr(end_of_word + 1);
                std::tr1::unordered_map<std::string, int>::const_iterator found = known.find(signature);
                if(found == known.end()) {
                    int id = tagsForWord.size();
                    known[signature] = id;
                    tagsForWordEntry[wordId] = id;
                    std::vector<std::string> tokens;
                    Tokenize(signature, tokens, "\t");
                    std::vector<int64> tagset;
                    for(std::vector<std::string>::const_iterator i = tokens.begin(); i != tokens.end(); i++) {
                        tagset.push_back(tagSymbols.AddSymbol(*i));
                    }
                    tagsForWord.push_back(tagset);
                } else {
                    tagsForWordEntry[wordId] = found->second;
                }
            }
            loaded = true;
            return loaded;
        }

        const std::vector<int64> &GetTagsForWord(int64 word) const {
            if(!IsLoaded()) {
                std::cerr << "ERROR: Lexicon::GetTagsForWord(" << wordSymbols.Find(word) << ") called on empty lexicon" << std::endl;
            }
            std::tr1::unordered_map<int64, int>::const_iterator found = tagsForWordEntry.find(word);
            if(found == tagsForWordEntry.end()) {
                return tagsForWord[0]; // all tags by convention
            } else {
                return tagsForWord[found->second];
            }
        }

        bool IsLoaded() const {
            return loaded;
        }

        void AddTags(fst::StdVectorFst &input) const {
            if(!IsLoaded()) {
                std::cerr << "ERROR: Lexicon::AddTags() called on empty lexicon" << std::endl;
                return;
            }
            if(input.InputSymbols() != &wordSymbols) {
                std::cerr << "ERROR: input symbols incompatible with Lexicon::AddTags() " << std::endl;
                return;
            }
            for(int64 state = 0; state < input.NumStates(); state++) {
                std::vector<fst::StdArc> arcs;
                for(fst::ArcIterator<fst::StdVectorFst> aiter(input, state); !aiter.Done(); aiter.Next()) {
                    const fst::StdArc &arc = aiter.Value();
                    const std::vector<int64> &wordTags = GetTagsForWord(arc.ilabel);
                    for(std::vector<int64>::const_iterator i = wordTags.begin(); i != wordTags.end(); i++) {
                        arcs.push_back(fst::StdArc(arc.ilabel, *i, arc.weight, arc.nextstate));
                    }
                }
                input.DeleteArcs(state);
                for(std::vector<fst::StdArc>::const_iterator i = arcs.begin(); i != arcs.end(); i++) {
                    input.AddArc(state, *i);
                }
            }
        }
    };
}
