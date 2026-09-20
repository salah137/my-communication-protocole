CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -I.

TARGET = test_runner
SRCS = test.c test_communication_handler.c

test: $(SRCS) test_communication_handler.h
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)