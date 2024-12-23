all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $@" ; mkdir -p "$(@D)" ;

ifneq (,$(strip $(filter clean,$(MAKECMDGOALS))))
clean:;rm -rf mid out
else

OPT_ENABLE:=stdlib graf text rom
PROJNAME:=spellingbee
PROJRDNS:=com.aksommerville.spellingbee
ENABLE_SERVER_AUDIO:=
BUILDABLE_DATA_TYPES:=dict

ifndef EGG_SDK
  EGG_SDK:=../egg
endif

include $(EGG_SDK)/etc/tool/common.mk

TOOL_EXE:=out/tool
TOOL_OPT_ENABLE:=serial fs rom
TOOL_OPT_CFILES:=$(filter $(addprefix $(EGG_SDK)/src/opt/,$(addsuffix /%,$(TOOL_OPT_ENABLE))),$(shell find $(EGG_SDK)/src/opt -name '*.c'))
TOOL_CFILES:=$(filter src/tool/%.c,$(CFILES))
TOOL_OFILES:=$(patsubst src/tool/%.c,mid/tool/%.o,$(TOOL_CFILES)) $(patsubst $(EGG_SDK)/src/opt/%.c,mid/tool/opt/%.o,$(TOOL_OPT_CFILES))
-include $(TOOL_OFILES:.o=.d)
$(TOOL_EXE):$(TOOL_OFILES);$(PRECMD) $(LD) -o$@ $(TOOL_OFILES) $(LDPOST)
mid/tool/%.o:src/tool/%.c|$(TOC_H);$(PRECMD) $(CC) -o$@ $<
mid/tool/opt/%.o:$(EGG_SDK)/src/opt/%.c;$(PRECMD) $(CC) -o$@ $<
mid/data/%:src/data/% $(TOOL_EXE);$(PRECMD) $(TOOL_EXE) -o$@ $< --toc=$(TOC_H)

endif
