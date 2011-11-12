ANTLR_M4_PATH ?= ../../../antlr-m4
M4PATH := $(ANTLR_M4_PATH):$(M4_PATH)
export M4PATH
GRAMMAR_OPTIONS += backtrack=true output=AST
GRAMMAR_BASENAME := dwarfidl
include $(ANTLR_M4_PATH)/Makerules
