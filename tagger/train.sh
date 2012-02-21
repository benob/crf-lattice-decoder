cat ftb.train.conll07 | cut -f2,5 | pypy add-morpho-features.py | ../apply_template pos-enhanced.template > ftb.train.conll07.crfsuite
cat ftb.dev.conll07 | cut -f2,5 | pypy add-morpho-features.py | ../apply_template pos-enhanced.template > ftb.dev.conll07.crfsuite
cat ftb.test.conll07 | cut -f2,5 | pypy add-morpho-features.py | ../apply_template pos-enhanced.template > ftb.test.conll07.crfsuite
crfsuite learn -m ftb.conll07.model.crfsuite -p feature.minfreq=2 -e2 -a l2sdg ftb.train.conll07.crfsuite ftb.dev.conll07.crfsuite
crfsuite dump ftb.conll07.model.crfsuite | pypy ../crfsuite2crfpp.py pos-enhanced.template > ftb.model.txt
