LIBRARY := memory

NAMESPACE := sp

INSTALL_PREFIX ?= /usr/local
BUILD_TYPE ?= Release
STD ?= 20

CXXFLAGS = -std=c++$(STD)
LD_FLAGS = -L$(LIB_DIR) -lgtest

ifeq ($(BUILD_TYPE), Release)
	CXXFLAGS += -Wall -Wextra -Werror 
	BUILD_DIR = build
else ifeq ($(BUILD_TYPE), Sanitize)
	CXXFLAGS += -g -fsanitize=address
	BUILD_DIR = build_fsan
else ifeq ($(BUILD_TYPE), Coverage)
	CXXFLAGS += -g -fprofile-arcs -fcoverage
	LD_FLAGS += -lgcov
	BUILD_DIR = build_cov
else ifeq ($(BUILD_TYPE), Debug)
	CXXFLAGS += -g
	BUILD_DIR = build_dbg
else
$(error Unknown build type $(BUILD_TYPE))
endif

OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib
SOURCES_DIR := src
INCLUDE_DIR := include
TESTS_DIR := tests


MEMCHECK ?= valgrind
MEMCHECK_FLAGS ?= --trace-children=yes --track-fds=yes --track-origins=yes --leak-check=full --show-leak-kinds=all --log-file=$(BUILD_DIR)/memcheck_report.txt -s

FSANITIZE := -fsanitize=address
COVERAGE :=-fprofile-arcs -ftest-coverage -lgcov

HEADERS := bit_iterator.h node_iterator.h pointer_iterator.h pool_allocator.h reserving_allocator.h reverse_iterator.h
SOURCES :=
TEST_SOURCES := test_pool_allocator.cc test_reserving_allocator.cc

HEADERS_PATH := $(addprefix $(INCLUDE_DIR)/$(NAMESPACE)/, $(HEADERS))
SOURCES_PATH := $(addprefix $(SOURCES_DIR)/$(NAMESPACE)/, $(SOURCES))
TEST_SOURCES_PATH := $(addprefix $(TESTS_DIR)/, $(TEST_SOURCES))
OBJ_PATH := $(addprefix $(OBJ_DIR)/, $(SOURCES:%.c=%.o))

TEST_EXE = unit_tests

all:

install:
	@mkdir -p $(INSTALL_PREFIX)/include/$(NAMESPACE)
	cp $(HEADERS_PATH) $(INSTALL_PREFIX)/include/$(NAMESPACE)/

uninstall:
	rm $(INSTALL_PREFIX)/include/sp/cstring.h

$(LIB_DIR)/lib$(LIBRARY).a: $(OBJ_DIR)/cstring.o
	@mkdir -p  $(LIB_DIR)
	ar -rcs $(LIB_DIR)/lib$(LIBRARY).a $(OBJ_PATH)
	ranlib $(LIB_DIR)/lib$(LIBRARY).a

tests: $(BUILD_DIR)/$(TEST_EXE) 

$(BUILD_DIR)/$(TEST_EXE): $(HEADERS_PATH) $(TEST_SOURCES_PATH)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) $(TEST_SOURCES_PATH) $(TESTS_DIR)/main.cc $(LD_FLAGS) -o $(BUILD_DIR)/$(TEST_EXE)

memcheck: $(BUILD_DIR)/$(TEST_EXE)
	$(MEMCHECK) $(MEMCHECK_FLAGS) ./$(BUILD_DIR)/$(TEST_EXE)

clean:
	rm -rf build coverage

.PHONY: all, clean, tests
