CPPFLAGS:=-Wall -g -lfst
all: pos-tagger apply_template_crfsuite crfpp_decode test_nbest test_cdb test
%: %.cc
	$(CXX) $(CPPFLAGS) -o $@ $<
