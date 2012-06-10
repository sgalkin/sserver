TARGET = sserver

SOURCES = \
	interface.cpp \
	main.cpp \
	log.cpp \
	server.cpp \
	socket.cpp \

LIBS = \
	boost_filesystem \
	boost_thread \
	boost_program_options \

TESTS = \
	tests/test_erase_iterator.cpp \
	tests/test_fd.cpp \
	tests/test_interface.cpp \
	tests/test_log.cpp \
	tests/test_message.cpp \
	tests/test_pool.cpp \
	tests/test_socket.cpp \
	tests/test_tempfile.cpp \

#	tests/test_traits.cpp \

TESTS_LIBS = \
	boost_unit_test_framework \

CXXFLAGS = -Wall -Werror -Wno-long-long -pedantic -O2 -g -MD
LDFLAGS += $(addprefix -l,${LIBS})
TESTS_LDFLAGS += $(addprefix -l,${TESTS_LIBS})

all: ${TARGET} check
	./$< -c config/sserver.conf

${TARGET}: ${SOURCES:.cpp=.o} Makefile
	${LINK.cpp} -o $@ $(filter %.o,$^)

check: ${addprefix run_,${notdir ${TESTS:.cpp=}}}

clean:
	${RM} ${SOURCES:.cpp=.o} ${SOURCES:.cpp=.d} ${TARGET}
	${RM} ${TESTS:.cpp=} ${TESTS:.cpp=.o} ${TESTS:.cpp=.d}

#			$$(findstring $$(patsubst tests/test_%.o,%.o,$(1:.cpp=.o)),$$(SOURCES:.cpp=.o))
define TEST_template
#TODO: filter-out main
$(1:.cpp=): $(1:.cpp=.o) \
			$$(filter-out main.o,$$(SOURCES:.cpp=.o)) \
			Makefile
	$${LINK.cpp} $$(TESTS_LDFLAGS) $$(filter %.o,$$^) -o $$@

run_$(notdir $(1:.cpp=)): $(1:.cpp=)
	./$$^
endef

$(foreach test,$(TESTS),$(eval $(call TEST_template,$(test))))

-include $(SOURCES:.cpp=.d)
-include $(TESTS:.cpp=.d)

.PHONY: check clean all
