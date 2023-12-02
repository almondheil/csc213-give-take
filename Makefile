CC     := clang
CFLAGS := -Wall -g -fsanitize=address

.PHONY: all clean zip format

all: give take

# TODO remove this rule
file: file.c utils.c
	${CC} ${CFLAGS} -o $@ $^

give: give.c message.c utils.c
	${CC} ${CFLAGS} -lpthread -o $@ $^

take: take.c message.c utils.c
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -f give take give-take.zip

zip: clean
	zip -r give-take.zip . -x .git/\* .vscode/\* .clang-format .gitignore tags LICENSE watch-give.sh .nfs\*

format:
	clang-format -i --style=file $(wildcard *.c) $(wildcard *.h)
