# Używamy CC, aby Makefile był przenośny (na Macu wywoła Clang, na Linuxie GCC)
CC=gcc
# Usuwamy -fanalyzer (nieobsługiwane przez Clang), dodajemy -pthread
override CFLAGS += -Wall -Wextra -g -O0 -fsanitize=address,undefined -pthread
LDFLAGS = -pthread

ifdef CI
override CFLAGS += -Werror
endif

.PHONY: clean all

all: sop-pool

sop-pool: sop-pool.c header.h
	$(CC) $(CFLAGS) -o sop-pool sop-pool.c $(LDFLAGS)

clean:
	rm -rf sop-pool sop-pool.dSYM