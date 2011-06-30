MYREADLINE = ../myreadline
CFLAGS += -I$(MYREADLINE)
LDFLAGS += -L$(MYREADLINE)

.PHONY: clean run_tests
all: myserver1 myserver2 run_tests

myserver1: myserver1.o
	$(CC) $(LDFLAGS) $+ $(MYREADLINE)/myreadline.a -o $@
	
asynclib.a: async_io.o async.o mlist.o hstack.o
	$(AR) $(ARFLAGS) $@ $^
	
myserver2: myserver2.o async_rl.o asynclib.a
	$(CC) $(LDFLAGS) $+ $(MYREADLINE)/myreadline.a -o $@

test_async: test_async.o async_rl.c asynclib.a
	$(CC) $(LDFLAGS) $+ -o $@
	
run_tests: test_async
	./test_async
	./test_server.py

clean:
	rm -f *.o *.so *.a myserver1 myserver2 test_async
	rm -rf *.dSYM
