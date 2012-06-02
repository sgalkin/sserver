TARGET = sserver

SOURCES = \
	main.cpp \

LIBS = \
	boost_filesystem \
	boost_thread \
	boost_program_options \

CXXFLAGS = -Wall -Werror -Wno-long-long -pedantic -O2 -g

LDFLAGS += $(addprefix -l,${LIBS})

all: ${TARGET}
	./$< -c config/sserver.conf

${TARGET}: ${SOURCES:.cpp=.o} Makefile
	${LINK.cpp} -o $@ $<

clean:
	${RM} ${SOURCES:.cpp=.o} ${TARGET}
