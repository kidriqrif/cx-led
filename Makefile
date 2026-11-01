# Needs the Ndless SDK: nspire-gcc, nspire-ld, genzehn and make-prg on PATH.
DEBUG = FALSE

GCC = nspire-gcc
LD = nspire-ld
GENZEHN = genzehn

GCCFLAGS = -Wall -W -marm -MMD -MP
LDFLAGS =
ZEHNFLAGS = --name "cx-led" --author "kidriqrif" --version 100 \
	--notice "Status LED colours and flash patterns" \
	--240x320-support true --uses-lcd-blit true

ifeq ($(DEBUG),FALSE)
GCCFLAGS += -Os
else
GCCFLAGS += -O0 -g
endif

EXE = cxled
OBJS = $(patsubst %.c,%.o,$(wildcard src/*.c))

all: $(EXE).tns

%.o: %.c
	$(GCC) $(GCCFLAGS) -c $< -o $@

$(EXE).elf: $(OBJS)
	$(LD) $^ -o $@ $(LDFLAGS)

$(EXE).tns: $(EXE).elf
	$(GENZEHN) --input $^ --output $@.zehn $(ZEHNFLAGS)
	make-prg $@.zehn $@
	rm $@.zehn

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(EXE).elf $(EXE).tns $(EXE).tns.zehn

-include $(OBJS:.o=.d)

.PHONY: all clean
