all:
.SILENT:
PRECMD=echo "  $@" ; mkdir -p "$(@D)" ;

ifeq (,$(EGG_SDK))
  EGG_SDK:=../egg2
endif
EGGDEV:=$(EGG_SDK)/out/eggdev

# Unlike a typical Egg game, we do some custom preprocessing of data.
TOOL_CC:=gcc -c -MMD -O3 -Isrc -I$(EGG_SDK)/src -DUSE_native=1
TOOL_LD:=gcc
TOOL_LDPOST:=
TOOL_EXE:=out/tool
TOOL_OPT_ENABLE:=serial fs
TOOL_OPT_CFILES:=$(filter $(addprefix $(EGG_SDK)/src/opt/,$(addsuffix /%,$(TOOL_OPT_ENABLE))),$(shell find $(EGG_SDK)/src/opt -name '*.c'))
TOOL_CFILES:=$(filter src/tool/%.c,$(shell find src/tool -name '*.c'))
TOOL_OFILES:=$(patsubst src/tool/%.c,mid/tool/%.o,$(TOOL_CFILES)) $(patsubst $(EGG_SDK)/src/opt/%.c,mid/tool/opt/%.o,$(TOOL_OPT_CFILES))
-include $(TOOL_OFILES:.o=.d)
$(TOOL_EXE):$(TOOL_OFILES);$(PRECMD) $(TOOL_LD) -o$@ $(TOOL_OFILES) $(TOOL_LDPOST)
mid/tool/%.o:src/tool/%.c|$(TOC_H);$(PRECMD) $(TOOL_CC) -o$@ $<
mid/tool/opt/%.o:$(EGG_SDK)/src/opt/%.c;$(PRECMD) $(TOOL_CC) -o$@ $<

DICT_SRC:=$(shell find src/data/dict -name '*.pre')
DICT_DST:=$(DICT_SRC:.pre=.dict)
src/data/dict/%.dict:src/data/dict/%.pre $(TOOL_EXE);$(PRECMD) $(TOOL_EXE) -o$@ $<

all:$(DICT_DST);$(EGGDEV) build
clean:;rm -rf mid out $(DICT_DST)
run:$(DICT_DST);$(EGGDEV) run
web-run:all;$(EGGDEV) serve --htdocs=out/spellingbee-web.zip --project=.
edit:;$(EGGDEV) serve \
  --htdocs=/data:src/data \
  --htdocs=EGG_SDK/src/web \
  --htdocs=EGG_SDK/src/editor \
  --htdocs=src/editor \
  --htdocs=/synth.wasm:EGG_SDK/out/web/synth.wasm \n  --htdocs=/build:out/spellingbee-web.zip \
  --htdocs=/out:out \
  --writeable=src/data \
  --project=.

