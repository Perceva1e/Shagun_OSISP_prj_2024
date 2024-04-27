EXEC = defender
CLIB = -lX11

$(EXEC): main.cpp libary.cpp
	g++ -o $@ $^ $(CLIB)

clean:
	rm -f $(EXEC) 
