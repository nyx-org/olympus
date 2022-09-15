include src/build.mk
include make/config.mk

$(BUILD_DIR)/%.c.o: src/%.c
	@$(MKCWD)
	$(CC) $(CFLAGS) -c -o $@ $<

define BIN_TEMPLATE

$(1)_NAME = $$(shell echo $(1) | tr A-Z a-z)

$(1)_PKG = src/$$($(1)_NAME)

$(1)_SRC = $$(wildcard $$($(1)_PKG)/*.c)

$(1)_OBJ = $$(patsubst src/%,$(BUILD_DIR)/%.o, $$($(1)_SRC))

$(1)_BIN = $(BUILD_DIR)/$$($(1)_NAME).elf

DEPENDENCIES += $$($(1)_OBJ:.o=.d)
ALL += $$($(1)_BIN)

$$($(1)_BIN): $$($(1)_OBJ)
	@$$(MKCWD)
	$(LD) $(LINK_FLAGS) -o $$@ $$($(1)_OBJ)

endef

$(foreach bin, $(SERVERS), $(eval $(call BIN_TEMPLATE,$(bin))))

-include $(DEPENDENCIES)

.DEFAULT_GOAL = all
all: $(ALL)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
