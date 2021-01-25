# $Id: mk.exe.inc,v 1.2 2002/08/26 21:32:17 te Exp $
#
# Tamer Embaby <tsemba@menanet.net>
#

!include <ntwin32.mak>

SRCDIR = c:\src
BINDIR = $(SRCDIR)\bin
SYSLIB = $(SRCDIR)\lib
INCDIR = $(SRCDIR)\include

CC		= cl
LINK		= link
INC		= -I. -I$(INCDIR)
OPTIMIZE	= -O2
CFLAGS		= -c -W3 -nologo
LIB		= $(LIB);$(SYSLIB)

!if defined (DEBUG) && "$(DEBUG)" == "YES"
OPTIMIZE	= -Zi
LINK_OPTS	= -debug
!else
OPTIMIZE	= -O2
LINK_OPTS	=
!endif

!if "$(DEFFILE)" == ""
DEFFILE = $(TRGT)
!endif

!ifdef UCFLAGS
CFLAGS		= $(CFLAGS) $(UCFLAGS)
!endif

!if defined (VFILE)
all		: $(VFILE) $(TRGT).exe
!else
all		: $(TRGT).exe
!endif

# all		: $(TRGT).exe

.c.obj		:
	$(CC) $(CFLAGS) $(INC) $(DEFINE) $(OPTIMIZE) $<

.rc.res		:
	$(rc) $(rcvars) -r $<

objs		: $(OBJS)

$(TRGT).exe	: $(OBJS) $(RES)
	$(LINK)						\
		-nologo					\
		-out:$(TRGT).exe			\
		-map:$(TRGT).map			\
		-machine:IX86				\
		$(LINK_OPTS)				\
		$(guilibs) $(RES) $(OBJS) $(EXTRALIBS)
	
	copy $(TRGT).exe $(BINDIR)

rcs		:
	if not exist .\RCS\nul mkdir .\RCS

co		:
	co -l $(RCS)

#-m"RCS"
ci		: clean
	ci $(RCS)

force		: clean all

clean		: 
	@del $(OBJS)
	@if exist *.exe del *.exe
	@if exist *.obj del *.obj
	@if exist *.res del *.res
	@if exist *.map del *.map
	@if exist *.pdb del *.pdb
	@if exist *.ilk del *.ilk

## EOF ##
