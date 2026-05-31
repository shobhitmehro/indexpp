CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra
LDFLAGS  :=

INCLUDES := -I. -I/opt/homebrew/opt/cereal/include

BUILDDIR := build
TARGET   := $(BUILDDIR)/index++
SRCS     := main.cpp src/index.cpp src/models.cpp src/word2vec.cpp src/eval.cpp
OBJS     := $(addprefix $(BUILDDIR)/, $(SRCS:.cpp=.o))

.PHONY: all clean debug asan

all: $(TARGET)

debug: CXXFLAGS += -g -O0
debug: $(TARGET)

asan: CXXFLAGS += -g -O0 -fsanitize=address -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address
asan: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILDDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILDDIR)
