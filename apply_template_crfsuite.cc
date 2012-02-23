#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "template.h"

// http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
static void tokenize(const std::string& str, std::vector<std::string>& tokens, const std::string& delimiters = " ")
{
    std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    std::string::size_type pos     = str.find_first_of(delimiters, lastPos);
    while (std::string::npos != pos || std::string::npos != lastPos)
    {
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        lastPos = str.find_first_not_of(delimiters, pos);
        pos = str.find_first_of(delimiters, lastPos);
    }
}

int main(int argc, char** argv) {
    if(argc != 2) {
        std::cerr << "usage: cat <input> | " << argv[0] << " <template>\n";
        return 1;
    }
    std::vector<macaon::CRFPPTemplate> templates;
    std::ifstream templateFile(argv[1]);
    while(!templateFile.eof()) {
        std::string line;
        std::getline(templateFile, line);
        if(templateFile.eof()) break;
        macaon::CRFPPTemplate current(line.c_str());
        if(current.type != macaon::CRFPPTemplate::BIGRAM) templates.push_back(current);
        //std::cerr << templates.back() << std::endl;
    }
    std::vector<std::vector<std::string> > lines;
    while(!std::cin.eof()) {
        std::string line;
        std::getline(std::cin, line);
        if(std::cin.eof()) break;
        std::vector<std::string> tokens;
        tokenize(line, tokens, " \t");
        if(tokens.size() == 0) {
            for(int position = 0; position < (int) lines.size(); position++) {
                std::cout << lines[position][lines[position].size() - 1];
                for(std::vector<macaon::CRFPPTemplate>::const_iterator i = templates.begin(); i != templates.end(); i++) {
                    std::string feature = i->apply(lines, position);
                    std::cout << "\t" << feature;
                }
                /*if(position == 0) std::cout << "\t__BOS__";
                if(position == (int) lines.size() - 1) std::cout << "\t__EOS__";*/
                std::cout << std::endl;
            }
            std::cout << std::endl;
            lines.clear();
        } else {
            lines.push_back(tokens);
        }
    }
}
