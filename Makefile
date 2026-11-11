CXX ?= g++
CXXFLAGS ?= -O2 -Wall -std=c++17 $(shell pkg-config --cflags gtkmm-3.0 libcurl sqlite3)
LDFLAGS ?= $(shell pkg-config --libs gtkmm-3.0 libcurl sqlite3) -pthread

SRCS = src/main.cpp \
       src/app_window.cpp \
       src/audio_recorder.cpp \
       src/whisper_client.cpp \
       src/history_db.cpp \
       src/fft_visualizer.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = asi-echoclip

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)
	ln -sf $(TARGET) whisper-clipboard

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) whisper-clipboard

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
DATADIR ?= $(PREFIX)/share

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	ln -sf $(TARGET) $(DESTDIR)$(BINDIR)/whisper-clipboard
	install -d $(DESTDIR)$(DATADIR)/asi-echoclip
	install -m 644 resources/style.css $(DESTDIR)$(DATADIR)/asi-echoclip/style.css
	install -d $(DESTDIR)$(DATADIR)/applications
	install -m 644 resources/asi-echoclip.desktop $(DESTDIR)$(DATADIR)/applications/asi-echoclip.desktop
	install -d $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps
	install -m 644 resources/asi-echoclip.svg $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/asi-echoclip.svg

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(BINDIR)/whisper-clipboard
	rm -rf $(DESTDIR)$(DATADIR)/asi-echoclip
	rm -f $(DESTDIR)$(DATADIR)/applications/asi-echoclip.desktop
	rm -f $(DESTDIR)$(DATADIR)/icons/hicolor/scalable/apps/asi-echoclip.svg

.PHONY: all clean install uninstall
