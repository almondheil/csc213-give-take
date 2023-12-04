CC     := clang
CFLAGS := -Wall -g #-fsanitize=address

.PHONY: all clean zip format

all: give take

give: give.c message.c utils.c filereader.c logging.c
	${CC} ${CFLAGS} -lpthread -o $@ $^

take: take.c message.c utils.c filereader.c
	${CC} ${CFLAGS} -o $@ $^

clean:
	rm -f give take give-take.zip

zip: clean
	zip -r give-take.zip . -x .git/\* .vscode/\* .clang-format .gitignore tags LICENSE watch-give.sh .nfs\*

format:
	clang-format -i --style=file $(wildcard *.c) $(wildcard *.h)
