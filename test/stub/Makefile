EXECUTABLE = build/EXECUTABLE_NAME_STUB

all: $(EXECUTABLE)

$(EXECUTABLE): build
	$(MAKE) -C build -j4

build:
	mkdir -p build && \
		cd build && \
		cmake ..

run: $(EXECUTABLE)
	$(EXECUTABLE)

.PHONY: $(EXECUTABLE)

clean:
	rm -rf build
	rm -rf *txt.user
