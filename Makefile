CPPFLAGS:=-g -Wall -lfst
all: test apply_template crfpp_decode test_nbest
%: %.cc
	$(CXX) $(CPPFLAGS) -o $@ $<
