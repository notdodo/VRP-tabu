CXX=g++
RM=rm -rf
BIN_NAME=VRP

#-s  Remove all symbol table and relocation information from the executable.
INCLUDE=-I actor/ -I lib/
SECFLAG= -Wextra -Werror -fstack-protector-all -Wstack-protector --param ssp-buffer-size=4
CPPFLAGS=-s -std=gnu++11 -Wall -O2 $(INCLUDE)
LD=-lpthread

SRCS=$(wildcard *.cpp actor/*.cpp lib/*.cpp lib/dist/*.cpp)
# notdir: get the filename from sources
OBJS_DIR=obj
OBJS=$(addprefix $(OBJS_DIR)/,$(patsubst %.cpp,%.o,$(notdir $(SRCS))))

all: $(OBJS_DIR) $(BIN_NAME)

$(OBJS_DIR):
	mkdir $(OBJS_DIR)

$(OBJS_DIR)/%.o: %.cpp
	        $(CXX) $(CPPFLAGS) -c $< -o $@

$(OBJS_DIR)/%.o: actor/%.cpp
	        $(CXX) $(CPPFLAGS) -c $< -o $@

$(OBJS_DIR)/%.o: lib/%.cpp
	        $(CXX) $(CPPFLAGS) -c $< -o $@

$(BIN_NAME) : $(OBJS)
	$(CXX) -o $@ $^ $(LD)

debug:
	$(CXX) -g $(CPPFLAGS) -o $(BIN_NAME) $(OBJS)

.PHONY : all clean
clean:
	$(RM) $(OBJS) $(OBJS_DIR)

dist-clean: clean
	$(RM) $(BIN_NAME)
