CLIBS = -llua5.3 -lsqlite3 -lm -lpthread -lcrypto -lssl
CFLAGS = -Wall -Wextra
CFILES = index.c
HFILES = file.h net.h file.h parse.h render.h config.h perr.h

index: $(CFILES) $(HFILES)
	gcc $(CGLAGS) $^ -o $@ $(CLIBS)

debug: index.c
	gcc $(CGLAGS) -D DEBUG -g $^ -o $@ $(CLIBS)
