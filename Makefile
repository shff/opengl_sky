CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wredundant-decls -Wcast-qual -Wnested-externs -Winline -Wno-long-long -Wconversion -Wstrict-prototypes -fwrapv -fno-strict-aliasing -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error -Wno-deprecated-declarations
CINCS = -I/usr/local/lib/glfw3/include -lglfw -Wl,-framework,OpenGL -Wl,-framework,Cocoa -Wl,-framework,IOKit -Wl,-framework,CoreVideo

all: main test

main: main.c
	@clang $(CFLAGS) $(CINCS) $^ -o $@

mainmm: main.mm
	@clang -fmodules -fcxx-modules -std=c++17 -lc++ -Wno-deprecated-declarations main.mm -o main && ./main

test:
	@du -h ./main
	@./main
	@rm main
