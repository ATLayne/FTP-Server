CC = gcc
CFLAGS = -Wall -g

TARGET = uftp_server

all: $(TARGET)

$(TARGET): uftp_server.c
	$(CC) $(CFLAGS) -o $(TARGET) uftp_server.c;

run: $(TARGET)
	clear && ./$(TARGET) 5050

clean:
	rm -f $(TARGET)