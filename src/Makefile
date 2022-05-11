TARGET=demoBin

SRC=sdlPlayAudio.c

CC=gcc

CFLAGS=-I../include/
#CFLAGS+=-g

LDFLAGS=-L../lib/

LDFLAGS+=-lavutil -lavformat -lavcodec -lswscale -lavdevice -lSDL2

$(TARGET):$(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

clean:
	rm $(TARGET) -rf
