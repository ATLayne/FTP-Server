CC = gcc
CFLAGS = -Wall -g

TARGET = uftp_client

all: $(TARGET)

$(TARGET): uftp_client.c
	$(CC) $(CFLAGS) -o $(TARGET) uftp_client.c;

run: $(TARGET)
	clear && ./$(TARGET) localhost 5050

clean:
	rm -f $(TARGET)