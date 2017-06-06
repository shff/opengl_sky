CFLAGS = -std=c99 -Wall -Wextra -Werror -pedantic -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-align -Wwrite-strings -Wredundant-decls -Wcast-qual -Wnested-externs -Winline -Wno-long-long -Wconversion -Wstrict-prototypes -fwrapv -fno-strict-aliasing -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
CINCS = -I/Users/shf/.brew/opt/glfw3/include /Users/shf/.brew/opt/glfw3/lib/libglfw3.a -Wl,-framework,OpenGL -Wl,-framework,Cocoa -Wl,-framework,IOKit -Wl,-framework,CoreVideo

all: main test

main: main.c
	@clang $(CFLAGS) $(CINCS) $^ -o $@

test:
	@du -h ./main
	@./main
	@rm main
