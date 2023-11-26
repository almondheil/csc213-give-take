CC     := clang
CFLAGS := -Wall -Werror -g -fsanitize=address
#CFLAGS := -O3

.PHONY: all clean zip format

all: give take

give: give.c message.c utils.c
	${CC} ${CFLAGS} -lpthread -o $@ $^

take: take.c message.c utils.c
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -f give take give-take.zip

zip: clean
	zip -r give-take.zip . -x .git/\* .vscode/\* .clang-format .gitignore tags

format:
	clang-format -i --style=file $(wildcard *.c) $(wildcard *.h)
