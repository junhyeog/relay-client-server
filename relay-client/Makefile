TARGET=relay-client
LDLIBS=-lpthread

$(TARGET): $(TARGET).o
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	rm -f $(TARGET) *.o