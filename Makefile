BUILD_DIR ?= build
CMAKE ?= cmake
RUN_INPUT ?= instances/VRP-Set-E/E-n51-k5.json
SOURCE_FILES := main.cpp $(wildcard actor/*.cpp) $(wildcard actor/*.h) $(wildcard lib/*.cpp) $(wildcard lib/*.h)
LINT_SOURCES := main.cpp $(wildcard actor/*.cpp) $(wildcard lib/*.cpp)
CLANG_FORMAT ?= $(shell sh -c 'for t in clang-format clang-format-19 clang-format-18 clang-format-17 /opt/homebrew/opt/llvm/bin/clang-format; do [ -x "$$t" ] && { echo "$$t"; exit 0; }; command -v "$$t" >/dev/null 2>&1 && { command -v "$$t"; exit 0; }; done')
CLANG_TIDY ?= $(shell sh -c 'for t in clang-tidy clang-tidy-19 clang-tidy-18 clang-tidy-17 /opt/homebrew/opt/llvm/bin/clang-tidy; do [ -x "$$t" ] && { echo "$$t"; exit 0; }; command -v "$$t" >/dev/null 2>&1 && { command -v "$$t"; exit 0; }; done')
CPPCHECK ?= $(shell command -v cppcheck 2>/dev/null)
SDKROOT ?= $(shell xcrun --sdk macosx --show-sdk-path 2>/dev/null)
NPROC ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
TIDY_EXTRA_ARGS := $(if $(SDKROOT),--extra-arg=-isysroot --extra-arg=$(SDKROOT),)

.PHONY: all configure release debug clean dist-clean
.PHONY: format format-check lint type-check check run

CLANG_TIDY_LOG := $(BUILD_DIR)/clang-tidy.log
RAPIDJSON_ERROR_REGEX := /rapidjson(-src)?/

all: release

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DVRP_WARNINGS_AS_ERRORS=ON

$(BUILD_DIR)/compile_commands.json:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release -DVRP_WARNINGS_AS_ERRORS=ON -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

release: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel

debug:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug -DVRP_WARNINGS_AS_ERRORS=ON
	$(CMAKE) --build $(BUILD_DIR) --parallel

clean:
	$(CMAKE) --build $(BUILD_DIR) --target clean

dist-clean:
	rm -rf $(BUILD_DIR)

format:
	@if [ -z "$(CLANG_FORMAT)" ]; then echo "clang-format not found (try: brew install clang-format)"; exit 127; fi
	$(CLANG_FORMAT) -i $(SOURCE_FILES)

format-check:
	@if [ -z "$(CLANG_FORMAT)" ]; then echo "clang-format not found (try: brew install clang-format)"; exit 127; fi
	$(CLANG_FORMAT) --dry-run --Werror $(SOURCE_FILES)

lint: $(BUILD_DIR)/compile_commands.json
	@if [ -z "$(CLANG_TIDY)" ]; then echo "clang-tidy not found (try: brew install llvm, then use /opt/homebrew/opt/llvm/bin/clang-tidy)"; exit 127; fi
	@mkdir -p $(BUILD_DIR)
	@printf '%s\n' $(LINT_SOURCES) | xargs -P $(NPROC) -I{} \
		sh -c '$(CLANG_TIDY) -p $(BUILD_DIR) --quiet $(TIDY_EXTRA_ARGS) "$$1"' _ {} \
		> $(CLANG_TIDY_LOG) 2>&1 || true
	@if grep -aE 'error: clang-tidy:' $(CLANG_TIDY_LOG) >/dev/null; then cat $(CLANG_TIDY_LOG); exit 1; fi
	@if grep -aE ': error: ' $(CLANG_TIDY_LOG) | grep -aE -v '$(RAPIDJSON_ERROR_REGEX)' >/dev/null; then cat $(CLANG_TIDY_LOG); exit 1; fi
	@if grep -aE ': error: ' $(CLANG_TIDY_LOG) >/dev/null; then \
		echo "clang-tidy reported findings only in external RapidJSON headers; ignoring those by policy."; \
	else \
		cat $(CLANG_TIDY_LOG); \
	fi

type-check:
	@if [ -z "$(CPPCHECK)" ]; then echo "cppcheck not found (try: brew install cppcheck)"; exit 127; fi
	$(CPPCHECK) --std=c++20 --language=c++ --enable=warning,performance,portability --inline-suppr --suppress=missingIncludeSystem --error-exitcode=1 --quiet main.cpp actor lib

check: format-check lint type-check release

run: release
	./$(BUILD_DIR)/VRP $(RUN_INPUT)
