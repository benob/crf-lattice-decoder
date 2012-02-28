#!/bin/bash
python=`(which pypy || which python2 || which python) 2>/dev/null`
cat ftb.train.conll07 | cut -f2,5 | $python french-tagger/add-morpho-features.py | ../apply_template_crfsuite ftb.template > ftb.train.conll07.crfsuite
cat ftb.dev.conll07 | cut -f2,5 | $python french-tagger/add-morpho-features.py | ../apply_template_crfsuite ftb.template > ftb.dev.conll07.crfsuite
cat ftb.test.conll07 | cut -f2,5 | $python french-tagger/add-morpho-features.py | ../apply_template_crfsuite ftb.template > ftb.test.conll07.crfsuite
crfsuite learn -m ftb.conll07.model.crfsuite -p feature.minfreq=2 -e2 -a l2sgd ftb.train.conll07.crfsuite ftb.dev.conll07.crfsuite
crfsuite dump ftb.conll07.model.crfsuite | $python ../crfsuite2crfpp.py ftb.template > ftb.model.txt
