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

.PHONY: all clean
