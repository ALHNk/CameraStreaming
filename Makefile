CC = gcc
CXX = g++

OPENCV_CFLAGS := $(shell pkg-config --cflags opencv4)
OPENCV_LIBS := $(shell pkg-config --libs opencv4)

CFLAGS = -Wall -Wextra -O2
CXXFLAGS = -Wall -Wextra -O2 $(OPENCV_CFLAGS)

LDFLAGS = $(OPENCV_LIBS)

TARGET = streamer

SRC_C = main.c camera.c udp.c tcp.c
SRC_CPP = defish.cpp

OBJ_C = $(SRC_C:.c=.o)
OBJ_CPP = $(SRC_CPP:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ_C) $(OBJ_CPP)
	$(CXX) $(OBJ_C) $(OBJ_CPP) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ_C) $(OBJ_CPP) $(TARGET)