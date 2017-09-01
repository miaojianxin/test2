
ifeq ($(arch),32)
	ARCH := -m32
	BITS := 32	
else
	ARCH:= -m64
	BITS:=64
	LIBEXT=64
endif

ifeq ($(release),y)
	OUTTYPE := -O2
	OUTCONFIG := release
else
	OUTCONFIG := debug
endif

TOPDIR:=..
OUTDIR=$(TOPDIR)/bin
OBJDIR=$(OUTDIR)/$(BITS)/$(OUTCONFIG)
SRCDIR:=$(TOPDIR)/alimq

DIRS:=$(SRCDIR)

SOURCE := $(foreach dir,$(DIRS), $(wildcard $(dir)/*.cpp))
OBJS := $(filter-out dllmain.o,$(patsubst %.cpp,%.o, $(notdir $(SOURCE))))

OUT :=libaliyunmq$(LIBEXT).so
LIBS := onsclient4cpp
LIBPATH := -L$(TOPDIR)/libs/lib-boost-static
INCLUDEPATH :=  -I$(TOPDIR)/include $(addprefix -I ,$(DIRS))

CC= g++
CFLAGS := -g -fPIC -Wno-deprecated  $(ARCH) $(OUTTYPE)
LDFLAGS := -shared -fPIC
CXXFLAGS := $(CFLAGS) $(INCLUDEPATH)

.PHONY : build rebuild clean echoo createdir

all:createdir build
createdir:
	mkdir -p $(OBJDIR)
build:$(OUT)
rebuild:clean build

clean:
	$(RM) $(OBJS) $(OUTDIR)/$(OUT)

echoo:
#	echo $(SOURCE)
	@echo $(OBJS)

$(OUT) : $(OBJS)
	$(CC) -o $(OUTDIR)/$(OUT) $(LDFLAGS) $(OBJS) $(LIBPATH) $(addprefix -l ,$(LIBS))

%.o : %.cpp
	$(CC) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@
