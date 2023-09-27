TESTS = $(wildcard *_test)
TESTS_CLEAN = $(addprefix clean_,$(TESTS))
TARGET_IS_TEST = $(filter test,$(MAKECMDGOALS))
TARGET_IS_RUN = $(filter run,$(MAKECMDGOALS))

ifneq ($(TARGET_IS_TEST),)
  ifeq ($(TEST_NAME),)
    $(error Missing parameter TEST_NAME)
  endif
endif

ifneq ($(TARGET_IS_RUN),)
  $(info Got the following tests: $(TESTS))
endif

$(info $(TESTS_CLEAN))

run: $(TESTS)
	$(info SUCCESS)

$(TESTS):
	$(info --------------------------------------------------)
	$(info Running $@)
	$(info --------------------------------------------------)
	$(MAKE) -C $@ run

clean: $(TESTS_CLEAN)

$(TESTS_CLEAN):
	$(info $@)
	$(MAKE) -C $(subst clean_,,$@) clean

.PHONY: clean run $(TESTS) $(TESTS_CLEAN)

test:
	cp -r stub $(TEST_NAME)
	find ./$(TEST_NAME) -type f | xargs -n 1 sed -i "s/EXECUTABLE_NAME_STUB/$(TEST_NAME)/g"

