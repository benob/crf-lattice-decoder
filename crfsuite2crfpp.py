#!/usr/bin/env python

# This program converts a crfsuite model dump to a crfpp text model that can be used by crfpp_decode (warning: it won't work with crfpp itself)
import sys, re

if len(sys.argv) != 2:
    print >>sys.stderr, "usage: crfsuite dump <model> | %s <template>\n" % sys.argv[0]
    sys.exit(1)

labels = []
label_map = {}

features = []
feature_map = {}

weights = []

section = None
for line in sys.stdin:
    line = line.strip()
    if line.strip() == "}":
        section = None
    else:
        found = re.match(r'^(FILEHEADER|LABELS|ATTRIBUTES|TRANSITIONS|STATE_FEATURES) = {$', line)
        tokens = line.strip().split()
        if found:
            section = found.group(1)
        elif section == "FILEHEADER":
            pass
        elif section == "LABELS":
            label_map[tokens[1]] = len(labels)
            labels.append(tokens[1])
        elif section == "ATTRIBUTES":
            if len(tokens) < 2:
                tokens.append("")
            feature_map[tokens[1]] = len(weights)
            weights.extend(["0" for x in range(len(labels))])
            features.append(tokens[1])
        elif section == "TRANSITIONS":
            previous = tokens[1]
            label = re.sub(r':$', '', tokens[3])
            score = tokens[4]
            if "B" not in feature_map:
                feature_map["B"] = len(weights)
                features.append("B")
                weights.extend(["0" for x in range(len(labels) * len(labels))])
            index = feature_map["B"] + label_map[previous] * len(labels) + label_map[label]
            weights[index] = score
        elif section == "STATE_FEATURES":
            if len(tokens) < 5:
                tokens = [tokens[0], "", tokens[1], tokens[2], tokens[3]]
            feature = tokens[1]
            label = re.sub(r':$', '', tokens[3])
            score = tokens[4]
            index = feature_map[feature] + label_map[label]
            weights[index] = score

print "version: 100"
print "cost-factor: 1"
print "maxid: %d" % len(features)
print "xsize: 2"
print

for label in labels:
    print label
print

# templates
for line in open(sys.argv[1]):
    print line.strip()
print

for feature, index in feature_map.items():
    print index, feature
print

print >>sys.stderr, len(weights)
for weight in weights:
    print weight
