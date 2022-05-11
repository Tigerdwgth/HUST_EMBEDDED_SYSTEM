EXENAME := lab5
EXESRCS := main.c
EXESRCS += audio_util.c
EXESRCS += OpenPDMFilter.c

include ../common/rules.mk


audio_test: audio_util.c OpenPDMFilter.c audio_test.c
	gcc -ggdb audio_util.c OpenPDMFilter.c audio_test.c -o audio_test

