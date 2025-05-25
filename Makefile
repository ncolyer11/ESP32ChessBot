SHELL = cmd.exe

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2
PROFILE_CXXFLAGS = -std=c++20 -Wall -Wextra -pg -g # -g for debug info with gprof

SRCS = main.cpp \
    src/board.cpp src/boardRepresentation.cpp src/util.cpp src/fen.cpp src/move.cpp \
    src/ai/eval.cpp src/movegen.cpp src/ai/chess_search.cpp

OBJDIR = build
DEPDIR = build
BINDIR = bin

# Default objects for regular build
OBJS = $(SRCS:src/ai/%.cpp=$(OBJDIR)/src/ai/%.o)
OBJS := $(OBJS:src/%.cpp=$(OBJDIR)/src/%.o)
OBJS := $(OBJS:main.cpp=$(OBJDIR)/main.o)

# Objects for profiling build
PROFILE_OBJS = $(SRCS:src/ai/%.cpp=$(OBJDIR)/src/ai/%.profile.o)
PROFILE_OBJS := $(PROFILE_OBJS:src/%.cpp=$(OBJDIR)/src/%.profile.o)
PROFILE_OBJS := $(PROFILE_OBJS:main.cpp=$(OBJDIR)/main.profile.o)

TARGET = $(BINDIR)\chesp32.exe
PROFILE_TARGET = $(BINDIR)\chesp32_profile.exe

all: $(TARGET)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) -o $@ $^

$(OBJDIR)/main.o: main.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -c $< -o $@

$(OBJDIR)/src/%.o: src/%.cpp | $(OBJDIR)/src
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -c $< -o $@

$(OBJDIR)/src/ai/%.o: src/ai/%.cpp | $(OBJDIR)/src/ai
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -c $< -o $@

# --- Profiling Rules ---
profile: $(PROFILE_TARGET)
	@echo Running program for profiling...
	@del gmon.out 2>NUL || rem # Delete existing gmon.out if it exists (ignore error if not found)
	$(PROFILE_TARGET) # Execute the profiled build
	@echo Analyzing gmon.out...
	gprof $(PROFILE_TARGET) gmon.out > profile_report.txt
	@echo Profile report generated: profile_report.txt
	@start profile_report.txt # Open the report (Windows specific)

$(PROFILE_TARGET): $(PROFILE_OBJS) | $(BINDIR)
	$(CXX) $(PROFILE_CXXFLAGS) -o $@ $^

$(OBJDIR)/main.profile.o: main.cpp | $(OBJDIR)
	$(CXX) $(PROFILE_CXXFLAGS) -Iinclude -MMD -c $< -o $@

$(OBJDIR)/src/%.profile.o: src/%.cpp | $(OBJDIR)/src
	$(CXX) $(PROFILE_CXXFLAGS) -Iinclude -MMD -c $< -o $@

$(OBJDIR)/src/ai/%.profile.o: src/ai/%.cpp | $(OBJDIR)/src/ai
	$(CXX) $(PROFILE_CXXFLAGS) -Iinclude -MMD -c $< -o $@
# --- End Profiling Rules ---

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/src:
	mkdir $(OBJDIR)\src

$(OBJDIR)/src/ai:
	mkdir $(OBJDIR)\src\ai

$(BINDIR):
	mkdir $(BINDIR)

-include $(DEPS)

.PHONY: all clean test profile clean_profile

clean:
	del /S /Q $(OBJDIR)\*.o
	del /S /Q $(OBJDIR)\*.d
	del $(TARGET)
	del $(PROFILE_TARGET) 2>NUL || rem # Clean profile target too

clean_profile:
	del /S /Q $(OBJDIR)\*.profile.o
	del gmon.out 2>NUL || rem
	del profile_report.txt 2>NUL || rem
	del $(PROFILE_TARGET) 2>NUL || rem

test: $(TARGET)
	@cmd.exe /c .\scripts\run_tests.bat
