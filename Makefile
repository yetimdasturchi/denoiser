CC      := cc
CFLAGS  := -O2 -std=c99 -Wall -Wextra -pedantic
INCLUDES:= -Iinclude
LDFLAGS := -lm

BENCH ?= 1
ifeq ($(BENCH),1)
  CFLAGS += -DBENCH=1
  SRC_EXTRA := src/bench.c
else
  CFLAGS += -DBENCH=0
  SRC_EXTRA :=
endif

SRC := src/wav.c src/fft.c src/denoise.c src/main.c $(SRC_EXTRA)
OBJ := $(SRC:.c=.o)
BIN := denoise

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJ) $(BIN)

.PHONY: all clean