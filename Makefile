OPTFLAGS = -O3 -flto
CXXFLAGS = -std=c++14 -Wall -Wextra -pedantic $(OPTFLAGS)
LDFLAGS = $(OPTFLAGS)
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
TARGET = mktree
OBJS = mktree.o
LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

$(TARGET): $(OBJS)

clean:
	$(RM) $(TARGET) $(OBJS)

.PHONY: clean
