#pragma once
#include <vector>
#include <string>

namespace macaon {
    class FeatureGenerator {
        static void prefixes(const std::string &word, int n, std::vector<std::string> &output) {
            int length = word.length();
            for(int i = 1; i < n; i++) {
                if(length >= i) output.push_back(word.substr(0, i));
                else output.push_back("__null__");
            }
        }
        static void suffixes(const std::string &word, int n, std::vector<std::string> &output) {
            int length = word.length();
            for(int i = 1; i < n; i++) {
                if(length >= i) output.push_back(word.substr(length - i - 1, i));
                else output.push_back("__null__");
            }
        }
        static void wordClasses(const std::string &word, std::vector<std::string> &output) {
            bool containsNumber = false;
            bool containsSymbol = false;
            for(int i = 0; i < (int) word.length(); i++) {
                if(!containsNumber && word.at(i) >= '0' && word.at(i) <= '9') containsNumber = true;
                if(!containsSymbol && !((word.at(i) >= '0' && word.at(i) <= '9') || (word.at(i) >= 'a' && word.at(i) <= 'z') || (word.at(i) >= 'A' && word.at(i) <= 'Z'))) containsSymbol = true;
            }
            if(containsNumber) output.push_back("Y");
            else output.push_back("N");
            if(word.length() >= 2 && word.at(0) >= 'A' && word.at(0) <= 'Z' && word.at(1) >= 'a' && word.at(1) <= 'z') output.push_back("Y");
            else output.push_back("N");
            if(containsSymbol) output.push_back("Y");
            else output.push_back("N");
        }
        static std::vector<std::string> get_pos_features(const std::string &word) {
            std::vector<std::string> output;
            wordClasses(word, output);
            prefixes(word, 4, output);
            suffixes(word, 4, output);
            return output;
        }
    };
}
