#!/bin/bash
python=`(which pypy || which python2 || which python) 2>/dev/null`
cat ptb.train.conll07 | cut -f2,5 | $python french-tagger/add-morpho-features.py | ../apply_template_crfsuite ptb.template > ptb.train.conll07.crfsuite
cat ptb.dev.conll07 | cut -f2,5 | $python french-tagger/add-morpho-features.py | ../apply_template_crfsuite ptb.template > ptb.dev.conll07.crfsuite
cat ptb.test.conll07 | cut -f2,5 | $python french-tagger/add-morpho-features.py | ../apply_template_crfsuite ptb.template > ptb.test.conll07.crfsuite
crfsuite learn -m ptb.conll07.model.crfsuite -p feature.minfreq=2 -e2 -a l2sgd ptb.train.conll07.crfsuite ptb.dev.conll07.crfsuite
crfsuite dump ptb.conll07.model.crfsuite | $python ../crfsuite2crfpp.py ptb.template > ptb.model.txt
