TARGET = sserver

SOURCES = \
	main.cpp \
	log.cpp \

LIBS = \
	boost_filesystem \
	boost_thread \
	boost_program_options \

TESTS = \
	tests/test_log.cpp \

TESTS_LIBS = \
	boost_unit_test_framework \

CXXFLAGS = -Wall -Werror -Wno-long-long -pedantic -O2 -g -MD
LDFLAGS += $(addprefix -l,${LIBS})
TESTS_LDFLAGS += $(addprefix -l,${TESTS_LIBS})

all: ${TARGET}
	./$< -c config/sserver.conf

${TARGET}: ${SOURCES:.cpp=.o} Makefile
	${LINK.cpp} -o $@ $(filter %.o,$^)

check: ${addprefix run_,${notdir ${TESTS:.cpp=}}}

clean:
	${RM} ${SOURCES:.cpp=.o} ${SOURCES:.cpp=.d} ${TARGET}
	${RM} ${TESTS:.cpp=} ${TESTS:.cpp=.o} ${TESTS:.cpp=.d}

define TEST_template
$(1:.cpp=): $(1:.cpp=.o) \
			$$(findstring $$(patsubst tests/test_%.o,%.o,$(1:.cpp=.o)),$$(SOURCES:.cpp=.o)) \
			Makefile
	$${LINK.cpp} $$(TESTS_LDFLAGS) $$(filter %.o,$$^) -o $$@

run_$(notdir $(1:.cpp=)): $(1:.cpp=)
	./$$<
endef

$(foreach test,$(TESTS),$(eval $(call TEST_template,$(test))))

-include $(SOURCES:.cpp=.d)
-include $(TESTS:.cpp=.d)
