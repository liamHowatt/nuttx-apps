include $(APPDIR)/Make.defs

CXXEXT := .cpp

# define MAJOR, MINOR, PATCH
include $(APPDIR)/CustomApps/olm/olm/common.mk
CXXFLAGS += -DOLMLIB_VERSION_MAJOR=$(MAJOR) -DOLMLIB_VERSION_MINOR=$(MINOR) \
            -DOLMLIB_VERSION_PATCH=$(PATCH)

CFLAGS +=   ${INCDIR_PREFIX}$(APPDIR)/CustomApps/olm/olm/lib
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/CustomApps/olm/olm/lib

# hacks
CXXFLAGS += ${INCDIR_PREFIX}$(APPDIR)/CustomApps/olm/include
# CXXFLAGS += -include $(APPDIR)/CustomApps/olm/no_static_assert.hh
CFLAGS   += -Wno-shadow -Wno-strict-prototypes
CXXFLAGS += -Wno-shadow

CSRCS +=   $(wildcard olm/src/*.c)
CSRCS +=   olm/lib/crypto-algorithms/sha256.c
CSRCS +=   olm/lib/crypto-algorithms/aes.c
CSRCS +=   olm/lib/curve25519-donna/curve25519-donna.c

CXXSRCS += $(wildcard olm/src/*.cpp)

include $(APPDIR)/Application.mk
