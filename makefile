CC = cc
CFLAGS = -std=c99 -Wall -pedantic -fsanitize=address,undefined -fsanitize-recover=address,undefined -fno-omit-frame-pointer
BUILD_DIR = build
SRCS = example.c ghost.c
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))
TARGET = $(BUILD_DIR)/example

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
