CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -I. -std=c99

BUILDDIR := build
TARGETS := server client

.PHONY: all clean

all: $(TARGETS)

%: %.c
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) $< -o $(BUILDDIR)/$@

clean:
	$(RM) -r $(BUILDDIR)
