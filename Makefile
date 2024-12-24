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
DESTDIR = /usr/local/bin

CFLAGS += -MMD -Wall -Wextra -Wpedantic -std=c99 -O2
CFLAGS += -Iinclude

INC = $(wildcard include/*.h)
SRC = $(wildcard src/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)

.PHONY: default all debug release install clean

default: all

all: $(TARGET)

debug: CFLAGS += -g -O0
debug: clean all

release: CFLAGS += -DNDEBUG
release: clean all

install: release
	install -m 755 $(TARGET) $(DESTDIR)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(OBJ)

clean:
	$(RM) $(TARGET) $(OBJ) $(DEP)

include Makefile.dev-tools

-include $(DEP)
