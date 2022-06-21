all: perftrace_cli perftrace_srv

perftrace_cli: perftrace_cli.c
	gcc perftrace_cli.c -o perftrace_cli ./lib/sqlite/sqlite3.c -lpthread -ldl -lm -O2 
perftrace_srv: perftrace_srv.c
	gcc perftrace_srv.c -o perftrace_srv -lpthread -lm -O2

.PHONY : clean
clean:
	rm -f perftrace_cli perftrace_srv