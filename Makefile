TARGET = sserver

SOURCES = \
	code.cpp \
	file_io.cpp \
	interface.cpp \
	main.cpp \
	poll.cpp \
	log.cpp \
	request.cpp \
	server.cpp \
	socket.cpp \
	sleep.cpp \

LIBS = \
	boost_filesystem \
	boost_thread \
	boost_program_options \
	boost_regex \

TESTS = \
	tests/test_erase_iterator.cpp \
	tests/test_fd.cpp \
	tests/test_file.cpp \
	tests/test_file_io.cpp \
	tests/test_interface.cpp \
	tests/test_log.cpp \
	tests/test_message.cpp \
	tests/test_pool.cpp \
	tests/test_request.cpp \
	tests/test_socket.cpp \
	tests/test_sleep.cpp \
	tests/test_tempfile.cpp \

TESTS_LIBS = \
	boost_unit_test_framework \

CXXFLAGS = -Wall -Werror -Wno-long-long -pedantic -O2 -g -MD
LDFLAGS += $(addprefix -l,${LIBS})
TESTS_LDFLAGS += $(addprefix -l,${TESTS_LIBS})

all: ${TARGET} check

${TARGET}: ${SOURCES:.cpp=.o} Makefile
	${LINK.cpp} -o $@ $(filter %.o,$^)

check: ${addprefix run_,${notdir ${TESTS:.cpp=}}}

clean:
	${RM} ${SOURCES:.cpp=.o} ${SOURCES:.cpp=.d} ${TARGET}
	${RM} ${TESTS:.cpp=} ${TESTS:.cpp=.o} ${TESTS:.cpp=.d}

define TEST_template
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
