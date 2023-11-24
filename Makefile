CC     := clang
CFLAGS := -Wall -g -fsanitize=address -lpthread

.PHONY: all clean zip

all: give take

give: give.c message.c
	${CC} ${CFLAGS} -o $@ $^

take: take.c message.c
	${CC} ${CFLAGS} -o $@ $^

clean:
	@echo "Cleaning files..."
	@rm -f give take give-take.zip

zip: clean
	@echo "Zipping files up for submission..."
	@zip -q -r give-take.zip . -x .git/\* .vscode/\* .clang-format .gitignore tags
	@echo "All done!"
