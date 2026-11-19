CXX ?= g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2

SRCS = main.cpp \
	src/board.cpp src/boardRepresentation.cpp src/util.cpp src/fen.cpp src/move.cpp \
	src/ai/eval.cpp src/movegen.cpp src/ai/chess_search.cpp

OBJDIR = build
BINDIR = bin

OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)
DEPS = $(OBJS:.o=.d)

ifeq ($(OS),Windows_NT)
    SHELL = cmd.exe
    EXE = .exe
    MKDIR = if not exist "$(subst /,\,$(1))" mkdir "$(subst /,\,$(1))"
    RMDIR = if exist "$(subst /,\,$(1))" rmdir /S /Q "$(subst /,\,$(1))"
    RUN_TESTS = scripts\run_tests.bat
    RUN_BIN = $(subst /,\,$(TARGET))
else
    EXE =
    MKDIR = mkdir -p $(1)
    RMDIR = rm -rf $(1)
    RUN_TESTS = sh scripts/run_tests.sh
    RUN_BIN = $(TARGET)
endif

TARGET = $(BINDIR)/chesp32$(EXE)

all: $(TARGET)

$(TARGET): $(OBJS)
	@$(call MKDIR,$(BINDIR))
	$(CXX) -o $@ $^

$(OBJDIR)/%.o: %.cpp
	@$(call MKDIR,$(dir $@))
	$(CXX) $(CXXFLAGS) -Iinclude -MMD -MP -c $< -o $@

-include $(DEPS)

.PHONY: all clean test perft

clean:
	@$(call RMDIR,$(OBJDIR))
	@$(call RMDIR,$(BINDIR))

test: $(TARGET)
	@$(RUN_TESTS)

perft: $(TARGET)
	$(RUN_BIN) --perft-suite
