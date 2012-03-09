#pragma once
#include <string>
#include <tr1/unordered_map>
#include <vector>
#include <stdio.h>
#include <errno.h>
#include "template.h"

namespace macaon {
    class CRFModel {
        std::string name;
        std::vector<CRFPPTemplate> templates;
        int version;
        double cost_factor;
        int maxid;
        int xsize;
        std::tr1::unordered_map<std::string, int> features;
        std::vector<double> weights;
        bool loaded;
    public:
        std::tr1::unordered_map<std::string, int> labels;
        int window_offset;
        int window_length;
        CRFModel() : loaded(false) {}
        CRFModel(const std::string &filename) : loaded(false) { readCRFPPTextModel(filename); }

        void trivialModel() {
            window_offset = 1;
            window_length = 2;
            templates.clear();
            templates.push_back(CRFPPTemplate("B"));
            templates.push_back(CRFPPTemplate("U%x[0,0]"));
            templates.push_back(CRFPPTemplate("U%x[0,0]/%x[-1,0]"));
            labels.clear();
            labels["0"] = 0;
            labels["1"] = 1;
            weights.clear();
            weights.push_back(0.1); // a 0
            weights.push_back(0.2); // a 1
            weights.push_back(0.3); // b 0
            weights.push_back(0.4); // b 1
            weights.push_back(0.5); // 0 0
            weights.push_back(0.6); // 0 1
            weights.push_back(0.7); // 1 0
            weights.push_back(0.8); // 1 1
            weights.push_back(0.9); // aa 0
            weights.push_back(1.0); // aa 1
            weights.push_back(1.1); // ab 0
            weights.push_back(1.2); // ab 1
            weights.push_back(1.3); // ba 0
            weights.push_back(1.4); // ba 1
            weights.push_back(1.5); // bb 0
            weights.push_back(1.6); // bb 1
            features.clear();
            features["Ua"] = 0;
            features["Ub"] = 2;
            features["B"] = 4;
            features["Ua/a"] = 8;
            features["Ua/b"] = 10;
            features["Ub/a"] = 12;
            features["Ub/b"] = 14;
            loaded = true;
        }

        void readCRFPPTextModel(const std::string &filename, const std::string& binary_cdb = "") {
            name = filename;
            FILE* fp = fopen(filename.c_str(), "r");
            if(!fp) {
                fprintf(stderr, "ERROR: %s, %s\n", filename.c_str(), strerror(errno));
                return;
            }
            char line[1024];
            int section = 0;
            int header_num = 0;
            int line_num = 0;
            while(NULL != fgets(line, 1024, fp)) {
                line_num ++;
                if(line[0] == '\n') {
                    section ++;
                } else {
                    line[1023] = '\0';
                    line[strlen(line) - 1] = '\0'; // chomp
                    if(section == 0) { // header
                        char* space = line;
                        while(*space != ' ' && *space != '\0') space ++;
                        if(header_num == 0) version = strtol(space + 1, NULL, 10);
                        else if(header_num == 1) cost_factor = strtod(space + 1, NULL);
                        else if(header_num == 2) maxid = strtol(space + 1, NULL, 10);
                        else if(header_num == 3) xsize = strtol(space + 1, NULL, 10);
                        else {
                            fprintf(stderr, "ERROR: unexpected header line %d in %s\n", line_num, filename.c_str());
                            fclose(fp);
                            return;
                        }
                        header_num ++;
                    } else if (section == 1) { // labels
                        int next_id = labels.size();
                        labels[std::string(line)] = next_id;
                    } else if (section == 2) { // templates
                        templates.push_back(CRFPPTemplate(line));
                    } else if (section == 3) { // feature indexes
                        char* space = line;
                        while(*space != ' ' && *space != '\0') space ++;
                        *space = '\0';
                        int index = strtol(line, NULL, 10);
                        features[std::string(space + 1)] = index;
                    } else if (section == 4) { // weights
                        weights.push_back(strtod(line, NULL));
                    } else {
                        fprintf(stderr, "ERROR: too many sections in %s\n", filename.c_str());
                        fclose(fp);
                        return;
                    }
                }
            }
            fclose(fp);
            int max_template_offset = 0;
            int min_template_offset = 9;
            for(std::vector<CRFPPTemplate>::const_iterator i = templates.begin(); i != templates.end(); i++) {
                if(i->type == CRFPPTemplate::BIGRAM && min_template_offset > -1) min_template_offset = -1; // account for label bigram 
                for(std::vector<TemplateItem>::const_iterator j = i->items.begin(); j != i->items.end(); j++) {
                    if(j->line < min_template_offset) min_template_offset = j->line;
                    if(j->line > max_template_offset) max_template_offset = j->line;
                }
            }
            window_offset = - min_template_offset;
            window_length = max_template_offset - min_template_offset + 1;
            loaded = true;
        }

        bool is_loaded() const {
            return loaded;
        }

        double rescore(const std::vector<std::vector<std::string> > &input, const std::vector<int> &context, const std::vector<int> &context_tags) {
            double output = 0;
            if((int) context.size() != window_length) return 0;
            //std::cerr << context[window_offset] << std::endl;
            if(context[window_offset] < 0) return 0;
            const int label = context_tags[window_offset]; //ilabels[input[context[window_offset]][input[context[window_offset]].size() - 1]];
            int previous = -1;
            if(window_length > 1 && context[window_offset - 1] >=0) previous = context_tags[window_offset - 1]; //labels[input[context[window_offset - 1]][input[context[window_offset - 1]].size() - 1]];
            for(std::vector<CRFPPTemplate>::const_iterator i = templates.begin(); i != templates.end(); i++) {
                std::string feature = i->applyToClique(input, context, window_offset);
                //std::cerr << "feature: " << feature << std::endl;
                std::tr1::unordered_map<std::string, int>::const_iterator found = features.find(feature);
                if(found != features.end()) {
                    if(found->second >= 0 && found->second < (int) weights.size()) {
                        if(i->type == CRFPPTemplate::UNIGRAM) output += weights[found->second + label];
                        else if(previous != -1) output += weights[found->second + label + labels.size() * previous];
                    }
                }
            }
            return output;
        }

        double transition(int previous, int label) {
            std::tr1::unordered_map<std::string, int>::const_iterator found = features.find("B");
            if(found != features.end()) {
                return weights[found->second + label + labels.size() * previous];
            }
            return 0;
        }

        std::vector<double> emissions(const std::vector<std::vector<std::string> > &input, const std::vector<int> &context) {
            std::vector<double> output(labels.size());
            if((int) context.size() != window_length) return output;
            if(context[window_offset] == -1) return output;
            for(std::vector<CRFPPTemplate>::const_iterator i = templates.begin(); i != templates.end(); i++) {
                std::string feature = i->applyToClique(input, context, window_offset);
                //std::cerr << " " << feature;
                std::tr1::unordered_map<std::string, int>::const_iterator found = features.find(feature);
                if(found != features.end()) {
                    if(found->second >= 0 && found->second < (int) weights.size()) {
                        if(i->type == CRFPPTemplate::UNIGRAM) 
                            for(size_t label = 0; label < labels.size(); label++) 
                                output[label] += weights[found->second + label];
                    }
                }
                //else std::cerr << "*";
            }
            //std::cerr << "\n";
            return output;
        }
    };
}
