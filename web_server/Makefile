CC := gcc
CFLAGS := -Wall -Wextra -Wpedantic -I. -std=c99

BUILDDIR := build
TARGETS := server

.PHONY: all clean

all: $(TARGETS)

%: %.c
	@mkdir -p $(BUILDDIR)
	@clang-format -i $<
	$(CC) $(CFLAGS) $< -o $(BUILDDIR)/$@

clean:
	$(RM) -r $(BUILDDIR)
