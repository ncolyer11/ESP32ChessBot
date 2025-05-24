SHELL = cmd.exe

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2

SRCS = main.cpp \
	src/board.cpp src/boardRepresentation.cpp src/util.cpp src/fen.cpp src/move.cpp \
	src/ai/eval.cpp src/movegen.cpp src/ai/chess_search.cpp

OBJDIR = build
DEPDIR = build
BINDIR = bin

OBJS = $(SRCS:src/ai/%.cpp=$(OBJDIR)/src/ai/%.o)
OBJS := $(OBJS:src/%.cpp=$(OBJDIR)/src/%.o)
OBJS := $(OBJS:main.cpp=$(OBJDIR)/main.o)

DEPS = $(OBJS:.o=.d)

TARGET = $(BINDIR)\chesp32.exe

all: $(TARGET)

$(TARGET): $(OBJS) | $(BINDIR)
	$(CXX) -o $@ $^

$(OBJDIR)/main.o: main.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -c $< -o $@

$(OBJDIR)/src/%.o: src/%.cpp | $(OBJDIR)/src
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -c $< -o $@

$(OBJDIR)/src/ai/%.o: src/ai/%.cpp | $(OBJDIR)/src/ai
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -c $< -o $@

$(OBJDIR):
	mkdir $(OBJDIR)

$(OBJDIR)/src:
	mkdir $(OBJDIR)\src

$(OBJDIR)/src/ai:
	mkdir $(OBJDIR)\src\ai

$(BINDIR):
	mkdir $(BINDIR)

-include $(DEPS)

.PHONY: all clean test

clean:
	del /S /Q $(OBJDIR)\*.o
	del /S /Q $(OBJDIR)\*.d
	del $(TARGET)

test: $(TARGET)
	@cmd.exe /c .\scripts\run_tests.bat
