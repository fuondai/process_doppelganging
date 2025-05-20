CXX = g++
CXXFLAGS = -std=c++17 -Wall
LDFLAGS = -static

# Windows specific
ifeq ($(OS),Windows_NT)
    CXXFLAGS += -D_WIN32
    LDFLAGS += -lgdi32 -luser32
endif

TARGET = RansomePicuter
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(TARGET).exe *.o

.PHONY: all clean 