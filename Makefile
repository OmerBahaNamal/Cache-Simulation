# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

# entry point for the program and target name
C_SRCS = src/main.c
CPP_SRCS = src/run_simulation.cpp

# Object files
C_OBJS = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)

# target name
TARGET := src/simulation

# Additional flags for the compiler
CXXFLAGS := -std=c++14  -I$(SYSTEMC_HOME)/include -L$(SYSTEMC_HOME)/lib -lsystemc -lm


# ---------------------------------------
# CONFIGURATION END
# ---------------------------------------


# Default to release build for both app and library
all: debug

# Rule to compile .c files to .o files
src/%.o: src/%.c src/helper_structs/result.h src/helper_structs/request.h
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile .cpp files to .o files
src/%.o: src/%.cpp src/helper_structs/cache_line.hpp src/modules/cpu.hpp src/modules/direct_mapped_cache.hpp \
			src/modules/four_way_cache.hpp src/helper_structs/result.h src/helper_structs/request.h
	$(CXX) $(CXXFLAGS) -c $< -o $@


# Debug build
debug: CXXFLAGS += -g
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O2
release: $(TARGET)

# Rule to link object files to executable
$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) -o $(TARGET)

# clean up
clean:
	rm -f $(TARGET)
	rm -rf src/*.o

.PHONY: all debug release clean