#pragma once

#include <string>
#include <vector>

namespace macaon {
    // http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
    static void Tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " ")
    {
        std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
        std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
        tokens.clear();
        while (std::string::npos != pos || string::npos != lastPos)
        {
            tokens.push_back(str.substr(lastPos, pos - lastPos));
            lastPos = str.find_first_not_of(delimiters, pos);
            pos = str.find_first_of(delimiters, lastPos);
        }
    }

    template <class A> class ILabelMapper {
        const fst::SymbolTable* oldSymbols;
        fst::SymbolTable* newSymbols;
    public:
        typedef A FromArc;
        typedef A ToArc;

        ILabelMapper(const fst::SymbolTable* _oldSymbols, fst::SymbolTable* _newSymbols) : oldSymbols(_oldSymbols), newSymbols(_newSymbols) {}
        A operator()(const A &arc) { return A(newSymbols->AddSymbol(oldSymbols->Find(arc.ilabel)), arc.olabel, arc.weight, arc.nextstate); }
        fst::MapFinalAction FinalAction() const { return fst::MAP_NO_SUPERFINAL; }
        fst::MapSymbolsAction InputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }
        fst::MapSymbolsAction OutputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const { return props; }
    };

    template <class A> class TrimSymbolsMapper {
    public:
        fst::SymbolTable inputSymbols;
        fst::SymbolTable outputSymbols;
        const fst::SymbolTable* oldInputSymbols;
        const fst::SymbolTable* oldOutputSymbols;
        typedef A FromArc;
        typedef A ToArc;

        TrimSymbolsMapper(const fst::SymbolTable* _oldInputSymbols, const fst::SymbolTable* _oldOutputSymbols) : inputSymbols("input"), outputSymbols("output"), oldInputSymbols(_oldInputSymbols), oldOutputSymbols(oldOutputSymbols) {}
        A operator()(const A &arc) { return A(inputSymbols.AddSymbol(oldInputSymbols->Find(arc.ilabel)), 
                outputSymbols.AddSymbol(oldOutputSymbols->Find(arc.olabel)), arc.weight, arc.nextstate); }
        fst::MapFinalAction FinalAction() const { return fst::MAP_NO_SUPERFINAL; }
        fst::MapSymbolsAction InputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS; }
        fst::MapSymbolsAction OutputSymbolsAction() const { return fst::MAP_COPY_SYMBOLS;}
        uint64 Properties(uint64 props) const { return props; }
    };

    void MapILabels(fst::StdVectorFst& input, fst::SymbolTable* newSymbols) {
        ILabelMapper<fst::StdArc> mapper(input.InputSymbols(), newSymbols);
        fst::ArcMap(&input, &mapper);
        input.SetInputSymbols(newSymbols);
    }

    void TrimSymbolTables(fst::StdVectorFst& input) {
        TrimSymbolsMapper<fst::StdArc> mapper(input.InputSymbols(), input.OutputSymbols());
        fst::ArcMap(&input, &mapper);
        input.SetInputSymbols(&mapper.inputSymbols);
        input.SetOutputSymbols(&mapper.outputSymbols);
    }
}
