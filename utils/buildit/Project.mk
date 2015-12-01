BUILD_CLANG = $(shell find $(SRCROOT) -name build_clang)

clang: $(OBJROOT) $(SYMROOT) $(DSTROOT)
	cd $(OBJROOT) && \
		$(BUILD_CLANG) $(Clang_Use_Assertions) $(Clang_Use_Optimized)
