MYREADLINE = ../myreadline
CFLAGS += -I$(MYREADLINE)
LDFLAGS += -L$(MYREADLINE)

all: myserver1

myserver1: myserver1.o
	$(CC) $(LDFLAGS) $+ $(MYREADLINE)/myreadline.a -o $@
	
clean:
	rm -f *.o *.so *.a myserver1