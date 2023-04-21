.PHONY: all run clean

all: build/verilog_parser

build/verilog_parser:
	mkdir -p build
	gcc -o build/verilog_parser -ggdb src/main.c src/common.c src/tokenizer.c

run: build/verilog_parser
	./build/verilog_parser ./input/01_simple.v

clean:
	rm -rf build
