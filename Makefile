TOOL_NAME = untrackerd
untrackerd_FILES = untrackerd.c
untrackerd_FRAMEWORKS = CoreFoundation
untrackerd_LDFLAGS = -lsqlite3 -Wl,-stack_size,1000

ADDITIONAL_CFLAGS = -std=c99

include framework/makefiles/common.mk
include framework/makefiles/tool.mk
