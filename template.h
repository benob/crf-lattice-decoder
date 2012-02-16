#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <string.h>
#include <stdlib.h>

namespace macaon {

    struct TemplateItem {
        int line;
        int column;
        std::string prefix;
        TemplateItem(const int _line, const int _column, const std::string &_prefix) : line(_line), column(_column), prefix(_prefix) { }
        friend std::ostream &operator<<(std::ostream &, const TemplateItem & );
    };

    struct CRFPPTemplate {
        enum TemplateType {
            UNIGRAM,
            BIGRAM,
        };
        std::string text;
        TemplateType type;
        int size;
        std::string suffix;
        std::vector<TemplateItem> items;
        CRFPPTemplate() {}
        CRFPPTemplate(const char* input) { read(input); }
        friend std::ostream &operator<<(std::ostream &, const CRFPPTemplate & );

        std::string apply(const std::vector<std::vector<std::string> > &clique, int offset) const {
            std::string output;
            for(std::vector<TemplateItem>::const_iterator i = items.begin(); i != items.end(); i++) {
                output += i->prefix;
                int column = i->column;
                int line = i->line;
                if(line + offset >= 0 && line + offset < (int) clique.size()) {
                    if(column >= 0 && column < (int) clique[line].size()) {
                        output += clique[line][column];
                    } else {
                        std::cerr << "ERROR: invalid column " << column << " in template \"" << text << "\"\n";
                        return "";
                    }
                } else {
                    output += "_B";
                    output += line;
                }
            }
            output += suffix;
            return output;
        }

        std::string applyToClique(const std::vector<std::vector<std::string> > &features, const std::vector<int> &clique, int offset) const {
            std::string output;
            for(std::vector<TemplateItem>::const_iterator i = items.begin(); i != items.end(); i++) {
                output += i->prefix;
                int column = i->column;
                int line = i->line;
                if(line + offset >= 0 && line + offset < (int) clique.size() && clique[line + offset] >=0) {
                    if(column >= 0 && column < (int) features[clique[line + offset]].size()) {
                        output += features[clique[line + offset]][column];
                    } else {
                        std::cerr << "ERROR: invalid column " << column << " in template \"" << text << "\"\n";
                        return "";
                    }
                } else {
                    output += "_B";
                    output += line;
                }
            }
            output += suffix;
            return output;
        }

        void read(const char* input) {
            text = input;
            size = 0;
            const char* current = input;
            const char* gap_start = NULL, *gap_end = NULL, *line_start = NULL, *column_start = NULL;
            int state = 0;
            gap_start = current;
            /* template is a succession of %x[-?\d+,\d+] which must be replaced by corresponding 
             * features at the given line, column relative to the current example.
             * They are parsed with a rudimentary state machine, and stored in the template.
             */
            if(*current == 'U') type = UNIGRAM;
            else if(*current == 'B') type = BIGRAM;
            else {
                std::cerr << "ERROR: unexpected template type \"" << input << "\"\n";
                return;
            }
            while(*current != '\0') {
                if(state == 0 && *current == '%') { state ++; gap_end = current; }
                else if(state == 1 && *current == 'x') { state ++; }
                else if(state == 2 && *current == '[') state ++;
                else if(state == 3 && (*current == '-' || (*current >= '0' && *current <= '9'))) { state ++; line_start = current; }
                else if(state == 4 && (*current >= '0' && *current <= '9'));
                else if(state == 4 && *current == ',') { state ++; }
                else if(state == 5 && (*current >= '0' && *current <= '9')) { state ++; column_start = current; }
                else if(state == 6 && (*current >= '0' && *current <= '9'));
                else if(state == 6 && *current == ']') {
                    state = 0;
                    char* gap = strndup(gap_start, gap_end - gap_start);
                    int column = strtol(column_start, NULL, 10);
                    int line = strtol(line_start, NULL, 10);
                    items.push_back(TemplateItem(line, column, std::string(gap)));
                    free(gap);
                    size++;
                    gap_start = current + 1;
                } else state = 0;
                current ++;
            }
            suffix = gap_start; // add trailing text
        }
    };


    std::ostream &operator<<(std::ostream &output, const macaon::TemplateItem &item) {
        output << item.prefix << "%x[" << item.line << "," << item.column << "]";
        return output;
    }

    std::ostream &operator<<(std::ostream &output, const macaon::CRFPPTemplate &featureTemplate) {
        for(std::vector<macaon::TemplateItem>::const_iterator i = featureTemplate.items.begin(); i != featureTemplate.items.end(); i++) {
            output << (*i);
        }
        output << featureTemplate.suffix;
        return output;
    }

}
