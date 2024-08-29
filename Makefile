# Nome do compilador
CC = gcc

# Flags de compilação
CFLAGS = -Wall -pthread

# Nome do executável
TARGET = TrabalhoSOCaixaEletronico

# Arquivos fonte
SRC = Trab2SO.c

# Arquivos objeto
OBJ = $(SRC:.c=.o)

# Regra padrão: compilar o executável
all: $(TARGET)

# Regra para compilar o executável
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Regra para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpeza dos arquivos objetos e executáveis
clean:
	rm -f $(OBJ) $(TARGET)

