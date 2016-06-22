raw upd packet sender
===

usage:

```
$ make
$ sudo ./raw_udp src_addr src_port dst_addr dst_port < what_you_want_to_say.txt
```

or

```
$ echo -n "what you want to say" | sudo ./raw_udp 1.2.3.4 1234 127.0.0.1 1234
```
