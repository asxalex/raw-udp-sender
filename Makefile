#
# Makefile
# alex, 2016-06-21 15:48
#

target=raw_udp

all:
	gcc -g $(target).c -o $(target)

install:
	sudo mv $(target) /bin/

uninstall:
	sudo rm /bin/$(target)

clean:
	rm -rf *.o $(target)
	


# vim:ft=make
#
