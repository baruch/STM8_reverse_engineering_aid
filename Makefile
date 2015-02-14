SRC=main.c
OBJ=$(SRC:.c=.rel)
SDCC=sdcc -lstm8 -mstm8

all: firmware.ihx

firmware.ihx: $(OBJ)
	$(SDCC) --out-fmt-ihx -o $@ $<

%.rel: %.c
	$(SDCC) -c -o $@ $<

clean:
	-rm -f *.rel *.ihx *.lk *.map *.rst *.lst *.asm *.sym tags

.PHONY: all clean
