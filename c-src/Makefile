#makefile


INCLUDES=-I/usr/include\
		 -I/usr/local/include\
		 -I./3rd/libuuid-1.0.3\
		 -I./3rd/lua-5.3.4

CFLAGS=-c -Wall -g $(INCLUDES)

SRCDIRS=. ./3rd/libuuid-1.0.3 ./3rd/lua-5.3.4 ./src ./src/aev ./src/clua
SRCS=$(foreach dir,$(SRCDIRS),$(wildcard $(dir)/*.c))
OBJS=$(SRCS:.c=.o)
TARGET=mmo

all: $(TARGET)
.PHONY: all

$(TARGET): $(OBJS)
	gcc -g -Wall $(OBJS) -o $@ -lc -ldl -lm
#	rm -rf $(OBJS) $(TARGET).core

%.o: %.c
	gcc $(CFLAGS) -fPIC $< -o $@

clean:
	rm -rf $(OBJS) $(TARGET).core