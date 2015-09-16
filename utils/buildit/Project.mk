BUILD_CLANG = $(shell find $(SRC) -name build_clang)

clang: $(OBJROOT) $(SYMROOT) $(DSTROOT)
	cd $(OBJROOT) && \
		$(BUILD_CLANG) $(ENABLE_ASSERTIONS) $(LLVM_OPTIMIZED)
