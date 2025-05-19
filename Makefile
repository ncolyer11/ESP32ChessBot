CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

SRCS = main.cpp board.cpp movegen.cpp search.cpp eval.cpp opening.cpp util.cpp
OBJDIR = build
DEPDIR = build

OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)
DEPS = $(SRCS:%.cpp=$(DEPDIR)/%.d)
TARGET = diyChessEngine.exe

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
	@echo "Running test suite..."
	@pass=0; fail=0; \
	for t in tests/*.in; do \
		name=$${t%.in}; \
		echo -n "Testing $${name}... "; \
		./$(TARGET) < $$t > tmp.out; \
		if diff -q tmp.out $${name}.out >/dev/null; then \
			echo "PASSED"; pass=$$((pass+1)); \
		else \
			echo "FAILED"; fail=$$((fail+1)); \
			echo "Diff:"; diff tmp.out $${name}.out; \
		fi \
	done; \
	rm -f tmp.out; \
	echo "$$pass passed, $$fail failed."; \
	test $$fail -eq 0

