TEST_PROGS := seccomp_bpf
CC = $(CROSS_COMPILE)gcc
LDFLAGS += -lpthread

all: $(TEST_PROGS)

run_tests:
	./seccomp_bpf

clean:
	$(RM) $(TEST_PROGS)
