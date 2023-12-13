CC     := clang
CFLAGS := -Wall -g

.PHONY: all clean zip format

all: give take

give: give.c message.c utils.c filereader.c socket.c logging.c
	${CC} ${CFLAGS} -lpthread -o $@ $^

take: take.c message.c utils.c filereader.c socket.c
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -f give take give-take.zip

zip: clean
	zip -r give-take.zip . -x .git/\* .vscode/\* .clang-format .gitignore tags LICENSE .nfs\* .github/\*

format:
	clang-format -i --style=file $(wildcard *.c) $(wildcard *.h)
