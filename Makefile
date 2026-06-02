BUILD_DIR ?= build
CMAKE ?= cmake
RUN_INPUT ?= instances/VRP-Set-E/E-n51-k5.json
WARNINGS_AS_ERRORS ?= ON
FETCH_NLOHMANN_JSON ?= ON
LTO ?= OFF
NATIVE ?= OFF
PROFILE ?= OFF
UNAME_S ?= $(shell uname -s)
SOURCE_FILES := main.cpp $(wildcard actor/*.cpp) $(wildcard actor/*.h) $(wildcard lib/*.cpp) $(wildcard lib/*.h)
LINT_SOURCES := main.cpp $(wildcard actor/*.cpp) $(wildcard lib/*.cpp)
CLANG_FORMAT ?= clang-format
CPPCHECK ?= cppcheck

ifeq ($(UNAME_S),Darwin)
SDKROOT ?= $(shell xcrun --sdk macosx --show-sdk-path 2>/dev/null)
MACOS_LIBCXX_INCLUDE ?= $(shell sh -c 'for p in "$$(xcrun --sdk macosx --show-sdk-path 2>/dev/null)/usr/include/c++/v1" "/Library/Developer/CommandLineTools/usr/include/c++/v1"; do [ -d "$$p" ] && { echo "$$p"; exit 0; }; done')
CLANG_TIDY_CANDIDATES := clang-tidy clang-tidy-22 clang-tidy-21 clang-tidy-20 clang-tidy-19 /opt/homebrew/opt/llvm/bin/clang-tidy /usr/local/opt/llvm/bin/clang-tidy
TIDY_DEFAULT_EXTRA_ARGS := $(if $(SDKROOT),--extra-arg=-isysroot --extra-arg=$(SDKROOT),) $(if $(MACOS_LIBCXX_INCLUDE),--extra-arg=-I$(MACOS_LIBCXX_INCLUDE),)
TIDY_JOBS ?= $(shell sysctl -n hw.logicalcpu 2>/dev/null || echo 8)
else
CLANG_TIDY_CANDIDATES := clang-tidy clang-tidy-22 clang-tidy-21 clang-tidy-20 clang-tidy-19 clang-tidy-18 clang-tidy-17
TIDY_DEFAULT_EXTRA_ARGS :=
TIDY_JOBS ?= $(shell nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 8)
endif

CLANG_TIDY ?= $(shell sh -c 'for t in $(CLANG_TIDY_CANDIDATES); do [ -x "$$t" ] && { echo "$$t"; exit 0; }; command -v "$$t" >/dev/null 2>&1 && { command -v "$$t"; exit 0; }; done')
TIDY_EXTRA_ARGS ?= $(TIDY_DEFAULT_EXTRA_ARGS)
TIDY_WARNINGS_AS_ERRORS ?= *
TIDY_HEADER_FILTER ?= (.*/)?(main\.cpp|actor/.*|lib/.*)
TIDY_EXCLUDE_HEADER_FILTER ?= (.*/)?(build|build-debug)/_deps/.*|.*/(rapidjson|nlohmann)/.*
CMAKE_CONFIG_FLAGS ?=
CMAKE_FLAGS := -DVRP_WARNINGS_AS_ERRORS=$(WARNINGS_AS_ERRORS) -DVRP_FETCH_NLOHMANN_JSON=$(FETCH_NLOHMANN_JSON) -DVRP_ENABLE_LTO=$(LTO) -DVRP_NATIVE_OPTIMIZATIONS=$(NATIVE) -DVRP_ENABLE_PROFILING=$(PROFILE) $(CMAKE_CONFIG_FLAGS)

.PHONY: help all configure release debug clean dist-clean format format-check lint type-check check check-strict run benchmark serve-view

all: release

help:
	@echo "Targets:"
	@echo "  make all            Build the release executable (default)."
	@echo "  make configure      Configure the release build directory."
	@echo "  make release        Configure and build in release mode."
	@echo "  make debug          Configure and build in debug mode."
	@echo "  make clean          Clean CMake build artifacts in BUILD_DIR."
	@echo "  make dist-clean     Remove BUILD_DIR."
	@echo "  make format         Format C++ sources with clang-format."
	@echo "  make format-check   Check C++ formatting without modifying files."
	@echo "  make lint           Run fast clang-tidy checks over C++ translation units."
	@echo "  make type-check     Run cppcheck static analysis."
	@echo "  make check          Run format-check, lint, type-check, and release."
	@echo "  make run            Build and run the solver."
	@echo "  make benchmark      Build and time the solver with cmake -E time."
	@echo "  make serve-view     Serve vrp-init/ at http://localhost:8000."
	@echo ""
	@echo "Flags / variables:"
	@echo "  BUILD_DIR           Build directory (default: build)."
	@echo "  CMAKE               CMake executable (default: cmake)."
	@echo "  RUN_INPUT           Input JSON used by make run."
	@echo "  WARNINGS_AS_ERRORS  Treat compiler warnings as errors: ON/OFF (default: ON)."
	@echo "  FETCH_NLOHMANN_JSON Fetch nlohmann/json when not installed: ON/OFF (default: ON)."
	@echo "  LTO                 Enable link-time optimization: ON/OFF (default: OFF)."
	@echo "  NATIVE              Enable -march=native for local benchmarks: ON/OFF (default: OFF)."
	@echo "  PROFILE             Add profiler-friendly debug symbols/frame pointers: ON/OFF (default: OFF)."
	@echo "  CLANG_FORMAT        clang-format executable."
	@echo "  CLANG_TIDY          clang-tidy executable."
	@echo "  CPPCHECK            cppcheck executable."
	@echo "  TIDY_EXTRA_ARGS     Extra arguments passed to clang-tidy."
	@echo "  TIDY_JOBS           Parallel clang-tidy workers (default: CPU count)."
	@echo "  TIDY_WARNINGS_AS_ERRORS  Clang-tidy warnings-as-errors filter (default: *)."
	@echo "  TIDY_HEADER_FILTER  Clang-tidy header filter regex."
	@echo "  TIDY_EXCLUDE_HEADER_FILTER Clang-tidy exclude-header regex."
	@echo "  CMAKE_CONFIG_FLAGS  Extra flags passed to CMake configure."
	@echo ""
	@echo "Examples:"
	@echo "  make run RUN_INPUT=instances/VRP-Set-E/E-n51-k5.json"
	@echo "  make lint CLANG_TIDY=clang-tidy-19"
	@echo "  make check WARNINGS_AS_ERRORS=OFF"
	@echo "  make benchmark LTO=ON NATIVE=ON"
	@echo "  make configure FETCH_NLOHMANN_JSON=OFF CMAKE_CONFIG_FLAGS='-DCMAKE_PREFIX_PATH=/opt/deps'"

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release $(CMAKE_FLAGS)

$(BUILD_DIR)/compile_commands.json:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release $(CMAKE_FLAGS) -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

release: configure
	$(CMAKE) --build $(BUILD_DIR) --parallel

debug:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Debug $(CMAKE_FLAGS)
	$(CMAKE) --build $(BUILD_DIR) --parallel

clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		$(CMAKE) --build $(BUILD_DIR) --target clean; \
	else \
		echo "$(BUILD_DIR) does not exist; nothing to clean."; \
	fi

dist-clean:
	$(CMAKE) -E rm -rf $(BUILD_DIR)

format:
	$(CLANG_FORMAT) -i $(SOURCE_FILES)

format-check:
	$(CLANG_FORMAT) --dry-run --Werror $(SOURCE_FILES)

lint: $(BUILD_DIR)/compile_commands.json
	@if [ -z "$(CLANG_TIDY)" ]; then echo "clang-tidy not found (macOS: brew install llvm)"; exit 127; fi
	@printf '%s\n' $(LINT_SOURCES) | xargs -n 1 -P $(TIDY_JOBS) $(CLANG_TIDY) -p $(BUILD_DIR) --quiet --warnings-as-errors='$(TIDY_WARNINGS_AS_ERRORS)' --header-filter='$(TIDY_HEADER_FILTER)' --exclude-header-filter='$(TIDY_EXCLUDE_HEADER_FILTER)' $(TIDY_EXTRA_ARGS)

type-check:
	$(CPPCHECK) --std=c++23 --language=c++ --enable=warning,performance,portability --inline-suppr --suppress=missingIncludeSystem --error-exitcode=1 --quiet main.cpp actor lib

check: format-check lint type-check release

run: release
	./$(BUILD_DIR)/VRP $(RUN_INPUT)

benchmark: release
	$(CMAKE) -E time ./$(BUILD_DIR)/VRP $(RUN_INPUT)

serve-view:
	python3 -m http.server 8000 --directory vrp-init
