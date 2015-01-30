# Makefile for Intel HEX parser
#

CFLAGS += -g
	
example: example.c intelhex.c intelhex.h
	${CC} ${CFLAGS} $^ -o $@
	
.PHONY: test clean

test: clean example
	@echo '---- parse example.hex ----'
	./example example.hex
	@echo '---- parse nrf51822.hex ----'
	./example nrf51822.hex
	
clean:
	rm -f example

