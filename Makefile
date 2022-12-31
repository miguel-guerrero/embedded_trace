LOG?=example_out.log
APP_NAME=example
APP=bin/$(APP_NAME)/main
TIME_STAMP_RATE_MHZ?=1600  # tick rate of time-stamp, (my cpu is 1.6 GHz)
CFLAGS=-g -O0 -I emblog -I $(APP_NAME)
RUNDIR=rundir
GEN_LOG=scripts/gen_log.py 
TRACE2VCD=scripts/trace2vcd.pl 

SRC=\
  $(APP_NAME)/main.c \
  emblog/debug.c \
  emblog/debug_hw_specific.c \
  emblog/emb_assert.c \
  emblog/emb_log.c \
  emblog/log.c \

OBJ=\
  bin/$(APP_NAME)/main.o \
  bin/$(APP_NAME)/emb_log.o \
  bin/$(APP_NAME)/log.o \
  bin/emblog/debug.o \
  bin/emblog/debug_hw_specific.o \
  bin/emblog/emb_assert.o \

# main targets

build: bin/emblog bin/$(APP_NAME) $(APP)

run: $(RUNDIR)/$(LOG)

rpt: $(RUNDIR)/$(APP_NAME).rpt

vcd: $(RUNDIR)/$(APP_NAME).vcd

test: clean rpt vcd
	diff example/msgs_auto.h.old example/msgs_auto.h
	diff -r rundir.old rundir

test1:
	make -C tests/test1 run

# run application and generate log

$(RUNDIR)/$(LOG): build $(RUNDIR) $(APP)
	cd $(RUNDIR) && ../$(APP) > $(LOG)

# post-processing

$(RUNDIR)/$(APP_NAME).rpt : $(RUNDIR)/$(LOG) $(GEN_LOG)
	$(GEN_LOG) --msgs $(APP_NAME)/msgs.txt --freq_in_mhz $(TIME_STAMP_RATE_MHZ) --output_style=rpt --hex_log $< --out_rpt $@

$(RUNDIR)/$(APP_NAME).trace : $(RUNDIR)/$(LOG) $(GEN_LOG)
	$(GEN_LOG) --msgs $(APP_NAME)/msgs.txt --freq_in_mhz $(TIME_STAMP_RATE_MHZ) --output_style=vcd --hex_log $< --out_rpt $@

$(RUNDIR)/$(APP_NAME).vcd : $(RUNDIR)/$(APP_NAME).trace $(TRACE2VCD)
	$(TRACE2VCD) -event_ps 10 -in $< -out $@

waves: $(RUNDIR)/$(APP_NAME).vcd
	gtkwave $< &

.phony: build run rpt test


$(RUNDIR):
	mkdir -p $@

bin/emblog:
	mkdir -p $@

bin/$(APP_NAME):
	mkdir -p $@

$(APP_NAME)/msgs_auto.h: $(APP_NAME)/msgs.txt $(GEN_LOG)
	$(GEN_LOG) --msgs $< --hdrs $@

bin/emblog/%.o: emblog/%.c emblog/*.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/$(APP_NAME)/%.o: $(APP_NAME)/%.c $(APP_NAME)/msgs_auto.h emblog/*.h
	$(CC) $(CFLAGS) -c $< -o $@

bin/$(APP_NAME)/%.o: emblog/%.c $(APP_NAME)/msgs_auto.h emblog/*.h
	$(CC) $(CFLAGS) -c $< -o $@

$(APP): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

ctags:
	ctags -R

clean:
	$(RM) -r $(APP_NAME)/msgs_auto.h bin $(RUNDIR) tags scripts/.mypy_cache
	make -C tests/test1 clean
