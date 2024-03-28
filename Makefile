EXEC = cppaudiocapture

CLIB = -I./lib/portaudio/include ./lib/portaudio/lib/.libs/libportaudio.a -lrt -lasound -ljack -pthread

$(EXEC): main.cpp
	g++ -o $@ $^ $(CLIB)

install-deps:
	mkdir -p lib
	
	cd lib/portaudio && ./configure && $(MAKE) -j
.PHONY: install-deps

uninstall-deps:
	cd lib/portaudio && $(MAKE) uninstall
	rm -rf lib/portaudio
.PHONY: uninstall-deps

clean:
	rm -f $(EXEC)
.PHONY: clean