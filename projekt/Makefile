# --- KONFIGURACJA ---

# Kompilator
CC = gcc

# Flagi kompilatora:
# -Wall -Wextra : włączają większość ostrzeżeń (bardzo pomocne!)
# -g            : dodaje informacje dla debuggera (żebyś widział gdzie jest błąd w gdb)
CFLAGS = -Wall -Wextra -g

# Nazwa Twojego gotowego programu
TARGET = sop_backup

# Lista plików obiektowych. 
# TERAZ: masz tylko main.o
# PÓŹNIEJ: dopiszesz tutaj: main.o backup.o utils.o
OBJS = main.o file_watcher_reccursive.o 

# --- REGUŁY BUDOWANIA ---

# Domyślna reguła (wpisujesz 'make' i to się wykonuje)
all: $(TARGET)

# Linkowanie: łączy pliki .o w jeden program wykonywalny
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Kompilacja: zamienia każdy plik .c na plik .o
# % to "wildcard" - pasuje do każdej nazwy
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Sprzątanie: usuwa pliki tymczasowe i program (wpisz 'make clean')
clean:
	rm -f $(OBJS) $(TARGET)

# Zabezpieczenie, żeby make nie mylił nazw reguł z plikami
.PHONY: all clean