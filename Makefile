EXECUTABLE := communicator
CROSS_COMPILE := 
#CFLAGS :=-g  -Wall -D__GNU__  
#CFLAGS := -Wall -D__GNU__ -O2 -std=gnu99 --sysroot=/home/llemmx/sysroots/mdm9607 -S
CXXFLAGS := $(CFLAGS)
CC := $(CROSS_COMPILE)gcc
STRIP := $(CROSS_COMPILE)strip
FPDIR := em_source_42
FPLIB := -L. -lpthread -lxml2 -lrt
INC   := -I ./ -I /usr/include/libxml2
CFLAGS := -Wall -DG_DEBUG -O2 -std=gnu99 $(INC)

SOURCE := $(wildcard *.c) $(wildcard *.cc) $(wildcard $(FPDIR)/*.c) 
OBJS := $(patsubst %.c,%.o,$(patsubst %.cc,%.o, $(SOURCE)))

$(EXECUTABLE) : $(OBJS)
	$(CROSS_COMPILE)$(CC) -o $(EXECUTABLE) $(OBJS) $(FPLIB) $(INC)
#	$(STRIP) --strip-all $(EXECUTABLE)


dest : $(OBJS)
	$(CROSS_COMPILE)$(CC) -o $(EXECUTABLE) $(OBJS) $(FPLIB) $(INC)

#	$(STRIP) --strip-all $(EXECUTABLE)
	gzip $(EXECUTABLE) -f 

clean:
	rm  -f $(OBJS)
	rm  -f $(EXECUTABLE)
	rm  -f *.s

cleanall:
	rm  -f $(OBJS)
	rm  *.h~
	rm  *.c~
	rm  *.d
	rm  *.s
	rm  $(EXECUTABLE)
