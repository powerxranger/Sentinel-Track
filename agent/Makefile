# Cross-platform Makefile for SentinelTrack

# Detect platform
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread
INCLUDES = -Iinclude

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    # Windows (MinGW)
    PLATFORM = WINDOWS
    LIBS = -lsqlite3 -lpsapi -liphlpapi -lws2_32
    TARGET = sentineltrack.exe
    RM = del /Q
    MKDIR = mkdir
else ifeq ($(UNAME_S),Darwin)
    # macOS
    PLATFORM = MACOS
    LIBS = -lsqlite3
    TARGET = sentineltrack
    RM = rm -f
    MKDIR = mkdir -p
    # macOS specific flags
    CXXFLAGS += -mmacosx-version-min=10.15
    ifeq ($(UNAME_M),arm64)
        CXXFLAGS += -arch arm64
    endif
else
    # Linux
    PLATFORM = LINUX
    LIBS = -lsqlite3
    TARGET = sentineltrack
    RM = rm -f
    MKDIR = mkdir -p
endif

SRCDIR = src
INCDIR = include
OBJDIR = obj
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

.PHONY: all clean install

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET) for $(PLATFORM)..."
	@$(CXX) $(OBJECTS) -o $@ $(LIBS)
	@echo "Build complete for $(PLATFORM)!"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	@echo "Compiling $< for $(PLATFORM)..."
	@$(CXX) $(CXXFLAGS) $(INCLUDES) -DPLATFORM_$(PLATFORM) -c $< -o $@

$(OBJDIR):
	@$(MKDIR) $(OBJDIR)

clean:
	@echo "Cleaning build files..."
	@$(RM) $(OBJDIR)/*.o 2>/dev/null || true
	@$(RM) $(TARGET) 2>/dev/null || true
	@echo "Clean complete!"

# Platform-specific install targets
ifeq ($(OS),Windows_NT)
install: $(TARGET)
	@echo "Installing $(TARGET) to C:\Program Files\SentinelTrack..."
	@copy $(TARGET) "C:\Program Files\SentinelTrack\" 2>nul || echo "Run as administrator to install system-wide"
else ifeq ($(UNAME_S),Darwin)
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installation complete!"
else
install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	@sudo cp $(TARGET) /usr/local/bin/
	@sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installation complete!"
endif

# Dependencies
$(OBJDIR)/main.o: $(INCDIR)/ProcessMonitor.h $(INCDIR)/NetworkMonitor.h $(INCDIR)/EventLogger.h $(INCDIR)/AnomalyDetector.h $(INCDIR)/PlatformUtils.h
$(OBJDIR)/ProcessMonitor.o: $(INCDIR)/ProcessMonitor.h $(INCDIR)/PlatformUtils.h
$(OBJDIR)/NetworkMonitor.o: $(INCDIR)/NetworkMonitor.h
$(OBJDIR)/EventLogger.o: $(INCDIR)/EventLogger.h $(INCDIR)/ProcessMonitor.h $(INCDIR)/NetworkMonitor.h $(INCDIR)/PlatformUtils.h
$(OBJDIR)/AnomalyDetector.o: $(INCDIR)/AnomalyDetector.h $(INCDIR)/ProcessMonitor.h $(INCDIR)/NetworkMonitor.h
$(OBJDIR)/PlatformUtils.o: $(INCDIR)/PlatformUtils.h