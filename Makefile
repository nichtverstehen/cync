MYREADLINE = ../myreadline
CFLAGS += -I$(MYREADLINE)
LDFLAGS += -L$(MYREADLINE)

all: myserver

myserver: main.o
	$(CC) $(LDFLAGS) $+ $(MYREADLINE)/myreadline.a -o $@
	
clean:
	rm -f *.o *.so *.a myserver