all: cbmpviewer colortest

cbmpviewer: cbmpviewer.c
	gcc -O2 -Wall -o cbmpviewer cbmpviewer.c -lm

debug: cbmpviewer.c
	gcc -DDEBUG -O2 -Wall -o cbmpviewer cbmpviewer.c -lm

colortest:
	gcc -O2 -Wall -o colortest colortest.c -lm

cbmpviewer.c: cbmpviewer.h
colortest.c: cbmpviewer.h

clean:
	rm -f cbmpviewer colortest

install:
	install -m 755 cbmpviewer /usr/local/bin/

