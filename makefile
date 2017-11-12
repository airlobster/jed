
SHELL=/bin/bash
CC=gcc
SRC_DIR=./src
INFRA_DIR=./src/infra
CFLAGS=-ggdb -lm -I $(SRC_DIR) -I $(INFRA_DIR)
DISASSEMBLE=objdump -d
TARGET=jed
ASM_TARGET=$(TARGET).asm

all: $(TARGET)

$(TARGET): $(SRC_DIR)/*.c $(SRC_DIR)/*.h $(INFRA_DIR)/*.c $(INFRA_DIR)/*.h $(SRC_DIR)/help
	$(SHELL) tostring -n help <$(SRC_DIR)/help >$(SRC_DIR)/help.h
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC_DIR)/*.c $(INFRA_DIR)/*.c
	rm -f $(SRC_DIR)/help.h
	rm -rf jed.dSYM

clean:
	rm -f $(TARGET)
	rm -f $(ASM_TARGET)

asm:	$(TARGET)
	$(DISASSEMBLE) $(TARGET) > $(ASM_TARGET)

