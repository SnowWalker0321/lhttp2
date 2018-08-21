CORE_OBJ_FLAGS := -std=c++11

HTTP2_SRCS := $(wildcard *.cc)
HTTP2_OBJS := $(HTTP2_SRCS:.cc=.o)

%.o: %.cc
	g++ -o $@ -c $< $(CORE_OBJ_FLAGS)