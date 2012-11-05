cbmpviewer: cbmpviewer.c
	gcc -O2 -Wall -o cbmpviewer cbmpviewer.c

debug: cbmpviewer.c
	gcc -DDEBUG -O2 -Wall -o cbmpviewer cbmpviewer.c

cbmpviewer.c: cbmpviewer.h

clean:
	rm cbmpviewer

install:
	install -m 755 cbmpviewer /usr/local/bin/

