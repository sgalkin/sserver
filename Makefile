TARGET = sserver

SOURCES = \
	main.cpp \

LIBS = \
	boost_thread \

CXXFLAGS = -Wall -Werror -Wno-long-long -pedantic -O2 -g

LDFLAGS += $(addprefix -l,${LIBS})

${TARGET}: ${SOURCES:.cpp=.o} Makefile
	${LINK.cpp} -o $@ $<

all: ${TARGET}
	./$<

clean:
	${RM} ${SOURCES:.cpp=.o} ${TARGET}
