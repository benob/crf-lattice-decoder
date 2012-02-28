CPPFLAGS:=-g -Wall -lfst
all: test apply_template_crfsuite crfpp_decode test_nbest
%: %.cc
	$(CXX) $(CPPFLAGS) -o $@ $<
