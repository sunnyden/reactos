PEFIXUP_BASE = $(TOOLS_BASE)
PEFIXUP_BASE_ = $(PEFIXUP_BASE)$(SEP)

PEFIXUP_INT = $(INTERMEDIATE_)$(PEFIXUP_BASE)
PEFIXUP_INT_ = $(PEFIXUP_INT)$(SEP)
PEFIXUP_OUT = $(OUTPUT_)$(PEFIXUP_BASE)
PEFIXUP_OUT_ = $(PEFIXUP_OUT)$(SEP)

PEFIXUP_TARGET = \
	$(PEFIXUP_OUT_)pefixup$(EXEPOSTFIX)

PEFIXUP_SOURCES = \
	$(PEFIXUP_BASE_)pefixup.c

PEFIXUP_OBJECTS = \
	$(addprefix $(INTERMEDIATE_), $(PEFIXUP_SOURCES:.c=.o))

PEFIXUP_HOST_CFLAGS = $(TOOLS_CFLAGS)

PEFIXUP_HOST_LFLAGS = $(TOOLS_LFLAGS)

.PHONY: pefixup
pefixup: $(PEFIXUP_TARGET)

$(PEFIXUP_TARGET): $(PEFIXUP_OBJECTS) | $(PEFIXUP_OUT)
	$(ECHO_LD)
	${host_gcc} $(PEFIXUP_OBJECTS) $(PEFIXUP_HOST_LFLAGS) -o $@

$(PEFIXUP_INT_)pefixup.o: $(PEFIXUP_BASE_)pefixup.c | $(PEFIXUP_INT)
	$(ECHO_CC)
	${host_gcc} $(PEFIXUP_HOST_CFLAGS) -c $< -o $@

.PHONY: pefixup_clean
pefixup_clean:
	-@$(rm) $(PEFIXUP_TARGET) $(PEFIXUP_OBJECTS) 2>$(NUL)
clean: pefixup_clean
