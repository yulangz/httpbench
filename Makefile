CFLAGS=	-Wall -ggdb -W -O
CC=		gcc
LIBS=
LDFLAGS=
EXECTUABLE=		httpbench

PREFIX=	/usr
VERSION=1.0
TMPDIR=/tmp/httpbench-$(VERSION)

# 最终目标
target=		$(EXECTUABLE)
all:$(target)

# 生成.o文件
%.o:%.c
	$(CC) -c $< -o $@ $(CFLAGS)

# 目标可执行文件
$(target): httpbench.o build_header.o circular_queue.o parse_url.o utils.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@  $(LIBS)

install: $(target)
	install -s $(target) $(DESTDIR)$(PREFIX)/local/bin
	install -s $(target) $(DESTDIR)$(PREFIX)/bin

clean:
	-rm -f *.o $(target)

.PHONY: clean all install
