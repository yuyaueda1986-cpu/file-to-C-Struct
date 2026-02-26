CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -Iinclude
AR      = ar
ARFLAGS = rcs

LIB_SRCS = src/ftcs_parser.c src/ftcs_core.c
LIB_OBJS = $(LIB_SRCS:.c=.o)
LIB      = libftcs.a

EXAMPLE_SRC  = example/sample_mapping.c
EXAMPLE_BIN  = example/sample_loader

EXAMPLE2_SRC = example2/sensor_loader.c
EXAMPLE2_BIN = example2/sensor_loader

CXX       = g++
CXXFLAGS  = -Wall -Wextra -std=c++17 -Iinclude
GTEST_LIBS = -lgtest -lgtest_main -lpthread

TEST_SRC  = test/test_ftcs.cpp
TEST_BIN  = test/test_ftcs
TEST_DATA_DIR = $(abspath test/data)

.PHONY: all example example2 test clean

all: $(LIB)

$(LIB): $(LIB_OBJS)
	$(AR) $(ARFLAGS) $@ $^

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

example: $(EXAMPLE_BIN)

$(EXAMPLE_BIN): $(EXAMPLE_SRC) $(LIB)
	$(CC) $(CFLAGS) -Iexample -o $@ $< -L. -lftcs -lrt

example2: $(EXAMPLE2_BIN)

$(EXAMPLE2_BIN): $(EXAMPLE2_SRC) $(LIB)
	$(CC) $(CFLAGS) -Iexample2 -o $@ $< -L. -lftcs -lrt

test: $(TEST_BIN)
	$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC) $(LIB)
	$(CXX) $(CXXFLAGS) -DTEST_DATA_DIR='"$(TEST_DATA_DIR)"' \
	    -o $@ $< -L. -lftcs $(GTEST_LIBS)

clean:
	rm -f $(LIB_OBJS) $(LIB) $(EXAMPLE_BIN) $(EXAMPLE2_BIN) $(TEST_BIN)
