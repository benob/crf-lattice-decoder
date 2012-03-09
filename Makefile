CPPFLAGS:=-Wall -g -lfst
all: pos-tagger pos-tagger-rescore pos-tagger-with-lexicon apply_template_crfsuite crfpp_decode test_nbest test_cdb test
%: %.cc
	$(CXX) $(CPPFLAGS) -o $@ $<
