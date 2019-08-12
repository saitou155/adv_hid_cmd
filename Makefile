# for Windows (mingw)
ifeq ($(OS),Windows_NT)
ifeq ($(MSYSTEM),MINGW32)
CFLAGS	= -g -Wall 
LIBS	= 
# Reference compiled libraries in local directory (32bit)
CFLAGS	+= -I./hidapi/win32/include/
LIBS	+= -L./hidapi/win32/lib/ -lhidapi -lsetupapi
else
CFLAGS	= -g -Wall 
LIBS	= 
# Reference compiled libraries in local directory (64bit)
CFLAGS	+= -I./hidapi/x64/include/
LIBS	+= -L./hidapi/x64/lib/ -lhidapi -lsetupapi
endif
endif

# for Linux
ifeq ($(shell uname),Linux)
CFLAGS	= -g -Wall 
LIBS	= 
# sudo apt-get install libhidapi-dev
CFLAGS	+= $(shell pkg-config --cflags hidapi-hidraw)
LIBS	+= $(shell pkg-config --libs hidapi-hidraw)
endif

# for MacOSX
ifeq ($(shell uname),Darwin)
CFLAGS	= -g -Wall 
LIBS	= 
# brew install hidapi
CFLAGS	+= -I$(shell brew --prefix hidapi)/include
LIBS	+= -L$(shell brew --prefix hidapi)/lib -lhidapi
endif

adv_hid_cmd:

adv_hid_cmd.o:	adv_hid_cmd.c
	$(CC) $(CFLAGS) -c adv_hid_cmd.c -o adv_hid_cmd.o

adv_hid_cmd:	adv_hid_cmd.o
	$(CC) $(CFLAGS) adv_hid_cmd.o -o adv_hid_cmd $(LIBS)

clean:
	$(RM) adv_hid_cmd
	$(RM) adv_hid_cmd.o
