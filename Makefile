# Makefile for Intel HEX parser
#

CFLAGS += -g
	
example: example.c intelhex.c intelhex.h
	${CC} ${CFLAGS} $^ -o $@
	
.PHONY: test clean

test: clean example
	@echo '---- parse example.hex ----'
	./example example.hex
	@echo '---- parse gcc_output.hex ----'
	./example gcc_output.hex
	
clean:
	rm -f example

