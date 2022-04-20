TARGET=demoBin

SRC=remuxing.c

CC=gcc

CFLAGS=-I../include/

LDFLAGS=-L../lib/

LDFLAGS+=-lavutil -lavformat -lavcodec

$(TARGET):$(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

clean:
	rm $(TARGET) -rf
