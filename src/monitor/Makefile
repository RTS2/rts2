all: monitor nmonitor

include ../../make.conf

LDFLAGS += -lncurses -lpthread -lm

monitor: monitor.o ../utils/devcli.a

nmonitor: nmonitor.o ../utils/devcli.a
