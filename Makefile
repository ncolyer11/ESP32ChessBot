SHELL=cmd.exe

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRCS = main.cpp board.cpp movegen.cpp move.cpp search.cpp eval.cpp opening.cpp util.cpp fen.cpp
OBJDIR = build
DEPDIR = build

OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)
DEPS = $(SRCS:%.cpp=$(DEPDIR)/%.d)
TARGET = chesp32.exe

all: $(TARGET)

# Rule to link the target
$(TARGET): $(OBJS)
	$(CXX) -o $@ $^

# Ensure OBJDIR exists before compiling
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -c $< -o $@

# Pattern rule to create the object/dependency folder
$(OBJDIR):
	mkdir $(OBJDIR)

# Include dependency files if they exist
-include $(DEPS)

.PHONY: all clean test

# Clean rule
clean:
	del /Q $(OBJDIR)\*.o $(DEPDIR)\*.d $(TARGET)

test: $(TARGET)
	@cmd.exe /c run_tests.bat

