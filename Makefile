CXXFLAGS += -std=c++11
LDFLAGS += -static $(shell pkg-config --cflags libcurl)
LDLIBS += $(shell pkg-config --libs libcurl)

.PHONY:	all

all:	test

