TARGET = aesdsocket

CC = $(CROSS_COMPILE)gcc

CFLAGS = -Wall -Werror -Wextra -g3 -O0 -ggdb

LDFLAGS = 

SRC = aesdsocket.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

