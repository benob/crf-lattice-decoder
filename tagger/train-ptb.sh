cat ptb.train.conll07 | cut -f2,5 | pypy french-tagger/add-morpho-features.py | ../apply_template pos-enhanced.template > ptb.train.conll07.crfsuite
cat ptb.dev.conll07 | cut -f2,5 | pypy french-tagger/add-morpho-features.py | ../apply_template pos-enhanced.template > ptb.dev.conll07.crfsuite
cat ptb.test.conll07 | cut -f2,5 | pypy french-tagger/add-morpho-features.py | ../apply_template pos-enhanced.template > ptb.test.conll07.crfsuite
crfsuite learn -m ptb.conll07.model.crfsuite -p feature.minfreq=2 -e2 -a l2sgd ptb.train.conll07.crfsuite ptb.dev.conll07.crfsuite
crfsuite dump ptb.conll07.model.crfsuite | pypy ../crfsuite2crfpp.py pos-enhanced.template > ptb.conll07.model.crfpp.txt
