# Copyright 2024 <lhearachel@proton.me>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
TARGET = $(shell basename $(CURDIR))
DESTDIR ?= $(HOME)/.local
BINDEST = $(DESTDIR)/bin
MANDEST = $(DESTDIR)/share/man/man1

CFLAGS += -MMD -Wall -Wextra -Wpedantic -std=c17
CFLAGS += -Wno-keyword-macro
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-deprecated-declarations
CFLAGS += -Iinclude

INC = $(wildcard include/*.h)
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)

MANP = docs/metang.1

VERSION_H = include/version.h

.PHONY: default all debug release install clean version

default: debug

debug: CFLAGS += -g -O0
debug: CFLAGS += -fsanitize=address
debug: LDFLAGS += -fsanitize=address
debug: LDFLAGS += -fsanitize-trap
debug: clean $(TARGET)

release: CFLAGS += -DNDEBUG -O3
release: CFLAGS += -DDEQUE_NDEBUG
release: clean $(TARGET)

install: release
	mkdir -p $(BINDEST)
	install -m 755 $(TARGET) $(BINDEST)
	mkdir -p $(MANDEST)
	install -m 644 $(MANP) $(MANDEST)

uninstall:
	rm -rf $(BINDEST)/$(TARGET) $(MANDEST)/$(MANP)

version: $(VERSION_H)

$(VERSION_H): VERSION
	./tools/version.sh $< $@

$(TARGET): $(VERSION_H) $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

clean:
	$(RM) $(TARGET) $(OBJ) $(DEP) $(VERSION_H)

include Makefile.dev-tools

-include $(DEP)
