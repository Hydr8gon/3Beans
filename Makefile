NAME := 3beans
BUILD := build
SRCS := src src/desktop
ARGS := -O3 -flto -std=c++11 -DLOG_LEVEL=0
LIBS := $(shell pkg-config --libs portaudio-2.0)
INCS := $(shell pkg-config --cflags portaudio-2.0)

APPNAME := 3Beans
PKGNAME := com.hydra.threebeans
DESTDIR ?= /usr

ifeq ($(OS),Windows_NT)
  ARGS += -static -DWINDOWS
  LIBS += $(shell wx-config-static --libs --gl-libs) -lole32 -lsetupapi -lwinmm
  INCS += $(shell wx-config-static --cxxflags)
else
  LIBS += $(shell wx-config --libs --gl-libs)
  INCS += $(shell wx-config --cxxflags)
  ifeq ($(shell uname -s),Darwin)
    ARGS += -DMACOS
    LIBS += -headerpad_max_install_names
  else
    ARGS += -no-pie
    LIBS += -lGL
  endif
endif

CPPFILES := $(foreach dir,$(SRCS),$(wildcard $(dir)/*.cpp))
HFILES := $(foreach dir,$(SRCS),$(wildcard $(dir)/*.h))
OFILES := $(patsubst %.cpp,$(BUILD)/%.o,$(CPPFILES))

all: $(NAME)

ifneq ($(OS),Windows_NT)
ifeq ($(uname -s),Darwin)

install: $(NAME)
	./mac-bundle.sh
	cp -r $(APPNAME).app /Applications/

uninstall:
	rm -rf /Applications/$(APPNAME).app

else

flatpak:
	flatpak-builder --repo=repo --force-clean build-flatpak $(PKGNAME).yml
	flatpak build-bundle repo $(NAME).flatpak $(PKGNAME)

flatpak-clean:
	rm -rf .flatpak-builder
	rm -rf build-flatpak
	rm -rf repo
	rm -f $(NAME).flatpak

install: $(NAME)
	install -Dm755 $(NAME) "$(DESTDIR)/bin/$(NAME)"
	install -Dm644 $(PKGNAME).desktop "$(DESTDIR)/share/applications/$(PKGNAME).desktop"

uninstall: 
	rm -f "$(DESTDIR)/bin/$(NAME)"
	rm -f "$(DESTDIR)/share/applications/$(PKGNAME).desktop"

endif
endif

$(NAME): $(OFILES)
	g++ -o $@ $(ARGS) $^ $(LIBS)

$(BUILD)/%.o: %.cpp $(HFILES) $(BUILD)
	g++ -c -o $@ $(ARGS) $(INCS) $<

$(BUILD):
	for dir in $(SRCS); do mkdir -p $(BUILD)/$$dir; done

clean:
	rm -rf $(BUILD)
	rm -f $(NAME)
