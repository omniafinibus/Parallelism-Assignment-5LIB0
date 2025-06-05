TARGET = arm_img_proc
CFLAGS = -O0 -g -Wall --pedantic
# you can add these to CFLAGS when running on your own computer (not: Arm or RISC-V on PYNQ):
# -fsanitize=address -fno-omit-frame-pointer 
HEADERS = $(wildcard *.h)
SOURCES = $(wildcard *.c )
OBJECTS = $(SOURCES:.c=.o)

all $(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) -lm

run: $(TARGET)
	./$(TARGET) test-images/*.bmp

clean:
	rm -rf $(OBJECTS) $(TARGET) *.dSYM
	sudo rm -f out-test-images/*.bmp
