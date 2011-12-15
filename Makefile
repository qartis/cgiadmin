WARNINGS=-Wall -Wextra -Wcast-align -Wcast-qual \
         -Wchar-subscripts -Wcomment -Wconversion -Wdisabled-optimization \
         -Wfloat-equal -Wformat -Wformat=2 -Wformat-nonliteral \
         -Wformat-security -Wformat-y2k -Wimport -Winit-self -Winline \
         -Winvalid-pch -Wlong-long -Wmissing-braces -Wmissing-format-attribute \
         -Wmissing-noreturn -Wpacked -Wparentheses -Wpointer-arith \
         -Wredundant-decls -Wreturn-type -Wshadow -Wsign-compare \
         -Wstrict-aliasing -Wstrict-aliasing=2 -Wswitch -Wswitch-default \
         -Wswitch-enum -Wtrigraphs -Wuninitialized -Wunknown-pragmas \
         -Wunreachable-code -Wunused -Wunused-function -Wunused-label \
         -Wunused-parameter -Wunused-value -Wunused-variable -Wwrite-strings
FLAGS=-g -rdynamic
CC=gcc
CFLAGS=-c $(WARNINGS) $(FLAGS)
LDFLAGS=$(FLAGS) -lssl -luuid -lsqlite3 -lcurl -lcrypto
OBJS=$(patsubst %.c,%.o,$(wildcard *.c))
TARGET=app.cgi

all: $(TARGET)

$(OBJS) : $(wildcard *.h) Makefile

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
