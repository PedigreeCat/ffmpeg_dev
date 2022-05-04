TARGET=demoBin

SRC=encodeAudioDemo.c

CC=gcc

CFLAGS=-I../include/
#CFLAGS+=-g

LDFLAGS=-L../lib/

LDFLAGS+=-lavutil -lavformat -lavcodec -lswscale

$(TARGET):$(SRC)
	$(CC) $(SRC) $(CFLAGS) $(LDFLAGS) -o $(TARGET)

clean:
	rm $(TARGET) -rf
