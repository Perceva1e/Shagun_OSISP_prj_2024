EXEC = defender

CLIB = -lX11

$(EXEC): main.cpp
	g++ -o $@ $^ $(CLIB)


clean:
	rm -f $(EXEC)