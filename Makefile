NAME := 3beans
BUILD := build
SRCS := src src/desktop
ARGS := -O3 -flto -std=c++11 -DLOG_LEVEL=3
LIBS := $(shell wx-config --libs std,gl) -lGL
INCS := $(shell wx-config --cxxflags std,gl)

CPPFILES := $(foreach dir,$(SRCS),$(wildcard $(dir)/*.cpp))
HFILES := $(foreach dir,$(SRCS),$(wildcard $(dir)/*.h))
OFILES := $(patsubst %.cpp,$(BUILD)/%.o,$(CPPFILES))

all: $(NAME)

$(NAME): $(OFILES)
	g++ -o $@ $(ARGS) $^ $(LIBS)

$(BUILD)/%.o: %.cpp $(HFILES) $(BUILD)
	g++ -c -o $@ $(ARGS) $(INCS) $<

$(BUILD):
	for dir in $(SRCS); do mkdir -p $(BUILD)/$$dir; done

clean:
	rm -rf $(BUILD)
	rm -f $(NAME)
