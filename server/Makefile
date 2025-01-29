CC = gcc
CFLAGS = -Wall -g

TARGET = uftp_server

all: $(TARGET)

$(TARGET): uftp_server.c
	$(CC) $(CFLAGS) -o $(TARGET) uftp_server.c

run: $(TARGET)
	rm -f $(TARGET) && $(CC) $(CFLAGS) -o $(TARGET) uftp_server.c && clear && ./$(TARGET) 5050

clean:
	rm -f $(TARGET)