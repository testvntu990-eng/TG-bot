CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include
LIBS = -lcurl

TARGET = bot
SOURCES = bot.cpp

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: run clean
