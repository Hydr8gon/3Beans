NAME     := 3beans
BUILD    := build
SOURCES  := src src/desktop
ARGS     := -O3 -flto -std=c++11
LIBS     := $(shell wx-config --libs std,gl) -lGL
INCLUDES := $(shell wx-config --cxxflags std,gl)

CPPFILES := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
HFILES   := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.h))
OFILES   := $(patsubst %.cpp,$(BUILD)/%.o,$(CPPFILES))

all: $(NAME)

$(NAME): $(OFILES)
	g++ -o $@ $(ARGS) $^ $(LIBS)

$(BUILD)/%.o: %.cpp $(HFILES) $(BUILD)
	g++ -c -o $@ $(ARGS) $(INCLUDES) $<

$(BUILD):
	for dir in $(SOURCES); \
	do \
	mkdir -p $(BUILD)/$$dir; \
	done

clean:
	rm -rf $(BUILD)
	rm -f $(NAME)
