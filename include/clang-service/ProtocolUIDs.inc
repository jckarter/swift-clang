#ifndef REGISTER_UID
#error Missing definition of REGISTER_UID(Name, UIDStr).
#endif

/* Generic key UIDs. */
REGISTER_UID(KeyRequest, "key.request")
REGISTER_UID(KeyToken, "key.token")
REGISTER_UID(KeyName, "key.name")
REGISTER_UID(KeyLineOffset, "key.line")
REGISTER_UID(KeyColOffset, "key.column")
REGISTER_UID(KeyCmdArgs, "key.cmd.args")
REGISTER_UID(KeyFiles, "key.files")
REGISTER_UID(KeyBuffer, "key.buffer")
REGISTER_UID(KeySort, "key.sort")
REGISTER_UID(KeyParseOptions, "key.parse.options")

/* Miscellaneous key UIDs. */
REGISTER_UID(KeyVersionMajor, "key.version_major")
REGISTER_UID(KeyVersionMinor, "key.version_minor")
REGISTER_UID(KeyIndexOptions, "key.index.options")
REGISTER_UID(KeyIndexExcludeDeclsFromPCH, "key.index.exclude_decls_pch")
REGISTER_UID(KeyIndexDiagnostics, "key.index.diagnostics")
REGISTER_UID(KeyDiagnosticsEnabled, "key.diagnostics.enabled")
REGISTER_UID(KeyDiagnosticsOptions, "key.diagnostics.options")

/* Location-based key UIDs. */
REGISTER_UID(KeyLocationFileName, "key.location.file.name")
REGISTER_UID(KeyLocationFileUID, "key.location.file.uid")

REGISTER_UID(KeyLocationInSysHeader, "key.location.in_sys_header")
REGISTER_UID(KeyLocationFromMainFile, "key.location.from_main_file")

REGISTER_UID(KeyLocationExpansionFile, "key.location.expansion.file")
REGISTER_UID(KeyLocationExpansionLineOffset, "key.location.expansion.line")
REGISTER_UID(KeyLocationExpansionColOffset, "key.location.expansion.column")
REGISTER_UID(KeyLocationExpansionOffset, "key.location.expansion.offset")

REGISTER_UID(KeyLocationPresumedFilename, "key.location.presumed.filename")
REGISTER_UID(KeyLocationPresumedLineOffset, "key.location.presumed.line")
REGISTER_UID(KeyLocationPresumedColOffset, "key.location.presumed.column")

REGISTER_UID(KeyLocationInstantiationFile, "key.location.instantiation.line")
REGISTER_UID(KeyLocationInstantiationLineOffset, "key.location.instantiation.column")
REGISTER_UID(KeyLocationInstantiationColOffset, "key.location.instantiation.offset")
REGISTER_UID(KeyLocationInstantiationOffset, "key.location.instantiation.offset")

REGISTER_UID(KeyLocationSpellingFile, "key.location.spelling.file")
REGISTER_UID(KeyLocationSpellingLineOffset, "key.location.spelling.line")
REGISTER_UID(KeyLocationSpellingColOffset, "key.location.spelling.column")
REGISTER_UID(KeyLocationSpellingOffset, "key.location.spelling.offset")

REGISTER_UID(KeyLocationFileFile, "key.location.file.file")
REGISTER_UID(KeyLocationFileLineOffset, "key.location.file.line")
REGISTER_UID(KeyLocationFileColOffset, "key.location.file.column")
REGISTER_UID(KeyLocationFileOffset, "key.location.file.offset")

REGISTER_UID(KeyRangeStart, "key.range.start")
REGISTER_UID(KeyRangeEnd, "key.range.end")

/* Code completion key UIDs. */
REGISTER_UID(KeyCodeCompleteOptions, "key.codecomplete.options")
REGISTER_UID(KeyCodeCompleteResults, "key.codecomplete.results")
REGISTER_UID(KeyCodeCompleteContexts, "key.codecomplete.contexts")
REGISTER_UID(KeyCodeCompleteContainerKind, "key.codecomplete.container.kind")
REGISTER_UID(KeyCodeCompleteContainerIncomplete,
             "key.codecomplete.container.incomplete")
REGISTER_UID(KeyCodeCompleteContainerUSR, "key.codecomplete.container.usr")
REGISTER_UID(KeyCodeCompleteObjCSelector, "key.codecomplete.objc_selector")
REGISTER_UID(KeyCodeCompleteDiagnostics, "key.codecomplete.diagnostics")
REGISTER_UID(KeyCodeCompleteCompletionKind, "key.codecomplete.completion.kind")
REGISTER_UID(KeyCodeCompleteCompletionStr, "key.codecomplete.completion.str")
REGISTER_UID(KeyCodeCompleteCompletionStrKind,
             "key.codecomplete.completion.str.kind")
REGISTER_UID(KeyCodeCompleteCompletionStrText,
             "key.codecomplete.completion.str.text")
REGISTER_UID(KeyCodeCompleteCompletionStrChunks,
             "key.codecomplete.completion.str.chunks")
REGISTER_UID(KeyCodeCompleteCompletionStrAvailability,
             "key.codecomplete.completion.str.availability")
REGISTER_UID(KeyCodeCompleteCompletionStrPriority,
             "key.codecomplete.completion.str.priority")
REGISTER_UID(KeyCodeCompleteCompletionStrAnnotations,
             "key.codecomplete.completion.str.annotations")
REGISTER_UID(KeyCodeCompleteCompletionStrParent,
             "key.codecomplete.completion.str.parent")
REGISTER_UID(KeyCodeCompleteCompletionStrParentKind,
             "key.codecomplete.completion.str.parent.kind")
REGISTER_UID(KeyCodeCompleteCompletionStrBrief,
             "key.codecomplete.completion.str.brief")

/* Indexing key UIDs. */
REGISTER_UID(KeyIndexSourceOptions, "key.index.source.options")

/* Diagnostic key UIDs. */
REGISTER_UID(KeyDiagnosticSeverity, "key.diagnostic.severity")
REGISTER_UID(KeyDiagnosticString, "key.diagnostic.string")
REGISTER_UID(KeyDiagnosticLocation, "key.diagnostic.location")
REGISTER_UID(KeyDiagnosticFixits, "key.diagnostic.fixits")

/* The request UIDs. */
REGISTER_UID(RequestCodeComplete, "source.request.codecomplete")
REGISTER_UID(RequestCodeCompleteOpen, "source.request.codecomplete.open")
REGISTER_UID(RequestCodeCompleteClose, "source.request.codecomplete.close")
REGISTER_UID(RequestCodeCompleteUpdate, "source.request.codecomplete.update")

REGISTER_UID(RequestIndexSource, "key.index.source")
REGISTER_UID(RequestIndexSourceContinue, "key.index.source.continue")
REGISTER_UID(RequestIndexSourceClose, "key.index.source.close")

/* Miscellaneous request UIDs. */
REGISTER_UID(RequestVersion, "source.request.version")

#undef REGISTER_UID
