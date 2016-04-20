/*===-- indexstore/indexstore.h - Index Store C API ----------------- C -*-===*\
|*                                                                            *|
|*                     The LLVM Compiler Infrastructure                       *|
|*                                                                            *|
|* This file is distributed under the University of Illinois Open Source      *|
|* License. See LICENSE.TXT for details.                                      *|
|*                                                                            *|
|*===----------------------------------------------------------------------===*|
|*                                                                            *|
|* This header provides a C API for the index store.                          *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifndef LLVM_CLANG_C_INDEXSTORE_INDEXSTORE_H
#define LLVM_CLANG_C_INDEXSTORE_INDEXSTORE_H

#include <stdint.h>
#include <stddef.h>

/**
 * \brief The version constants for the Index Store C API.
 * INDEXSTORE_VERSION_MINOR should increase when there are API additions.
 * INDEXSTORE_VERSION_MAJOR is intended for "major" source/ABI breaking changes.
 */
#define INDEXSTORE_VERSION_MAJOR 0
#define INDEXSTORE_VERSION_MINOR 1

#define INDEXSTORE_VERSION_ENCODE(major, minor) ( \
      ((major) * 10000)                           \
    + ((minor) *     1))

#define INDEXSTORE_VERSION INDEXSTORE_VERSION_ENCODE( \
    INDEXSTORE_VERSION_MAJOR,                         \
    INDEXSTORE_VERSION_MINOR )

#define INDEXSTORE_VERSION_STRINGIZE_(major, minor)   \
    #major"."#minor
#define INDEXSTORE_VERSION_STRINGIZE(major, minor)    \
    INDEXSTORE_VERSION_STRINGIZE_(major, minor)

#define INDEXSTORE_VERSION_STRING INDEXSTORE_VERSION_STRINGIZE( \
    INDEXSTORE_VERSION_MAJOR,                                   \
    INDEXSTORE_VERSION_MINOR)

#ifdef  __cplusplus
# define INDEXSTORE_BEGIN_DECLS  extern "C" {
# define INDEXSTORE_END_DECLS    }
#else
# define INDEXSTORE_BEGIN_DECLS
# define INDEXSTORE_END_DECLS
#endif

#ifndef INDEXSTORE_PUBLIC
# if defined (_MSC_VER)
#  define INDEXSTORE_PUBLIC __declspec(dllimport)
# else
#  define INDEXSTORE_PUBLIC
# endif
#endif

#ifndef __has_feature
# define __has_feature(x) 0
#endif

#if __has_feature(blocks)
# define INDEXSTORE_HAS_BLOCKS 1
#else
# define INDEXSTORE_HAS_BLOCKS 0
#endif

INDEXSTORE_BEGIN_DECLS

typedef void *indexstore_error_t;

INDEXSTORE_PUBLIC const char *
indexstore_error_get_description(indexstore_error_t);

INDEXSTORE_PUBLIC void
indexstore_error_dispose(indexstore_error_t);

typedef struct {
  const char *data;
  size_t length;
} indexstore_string_ref_t;

INDEXSTORE_PUBLIC unsigned
indexstore_format_version(void);

typedef void *indexstore_t;

INDEXSTORE_PUBLIC indexstore_t
indexstore_store_create(const char *store_path, indexstore_error_t *error);

INDEXSTORE_PUBLIC void
indexstore_store_dispose(indexstore_t);

#if INDEXSTORE_HAS_BLOCKS
INDEXSTORE_PUBLIC bool
indexstore_store_units_apply(indexstore_t,
                             bool(^applier)(indexstore_string_ref_t unit_name));
#endif

typedef enum {
  INDEXSTORE_UNIT_EVENT_ADDED = 1,
  INDEXSTORE_UNIT_EVENT_REMOVED = 2,
  INDEXSTORE_UNIT_EVENT_MODIFIED = 3,
  INDEXSTORE_UNIT_EVENT_DIRECTORY_DELETED = 4,
} indexstore_unit_event_kind_t;

typedef struct {
  indexstore_unit_event_kind_t kind;
  indexstore_string_ref_t unit_name;
} indexstore_unit_event_t;

#if INDEXSTORE_HAS_BLOCKS
typedef void (^indexstore_unit_event_handler_t)(indexstore_unit_event_t *events,
                                                size_t count);

INDEXSTORE_PUBLIC bool
indexstore_store_set_unit_event_handler(indexstore_t,
                                       indexstore_unit_event_handler_t handler,
                                       indexstore_error_t *error);
#endif

INDEXSTORE_PUBLIC void
indexstore_store_discard_unit(indexstore_t, const char *unit_name);

INDEXSTORE_PUBLIC void
indexstore_store_purge_stale_records(indexstore_t,
                                     indexstore_string_ref_t *active_record_names,
                                     size_t active_record_count);

/// Determines the unit name from the \c output_path and writes it out in the
/// \c name_buf buffer. It doesn't write more than \c buf_length.
/// \returns the length of the name. If this is larger than \c buf_length, the
/// caller should call the function again with a buffer of the appropriate size.
INDEXSTORE_PUBLIC size_t
indexstore_store_get_unit_name_from_output_path(indexstore_t store,
                                                const char *output_path,
                                                char *name_buf,
                                                size_t buf_size);

/// \returns true if an error occurred, false otherwise.
INDEXSTORE_PUBLIC bool
indexstore_store_get_unit_modification_time(indexstore_t store,
                                            const char *unit_name,
                                            int64_t *seconds,
                                            int64_t *nanoseconds,
                                            indexstore_error_t *error);

typedef void *indexstore_symbol_t;

typedef enum {
  INDEXSTORE_SYMBOL_KIND_UNKNOWN = 0,
  INDEXSTORE_SYMBOL_KIND_MODULE = 1,
  INDEXSTORE_SYMBOL_KIND_NAMESPACE = 2,
  INDEXSTORE_SYMBOL_KIND_NAMESPACEALIAS = 3,
  INDEXSTORE_SYMBOL_KIND_MACRO = 4,
  INDEXSTORE_SYMBOL_KIND_ENUM = 5,
  INDEXSTORE_SYMBOL_KIND_STRUCT = 6,
  INDEXSTORE_SYMBOL_KIND_CLASS = 7,
  INDEXSTORE_SYMBOL_KIND_PROTOCOL = 8,
  INDEXSTORE_SYMBOL_KIND_EXTENSION = 9,
  INDEXSTORE_SYMBOL_KIND_UNION = 10,
  INDEXSTORE_SYMBOL_KIND_TYPEALIAS = 11,
  INDEXSTORE_SYMBOL_KIND_FUNCTION = 12,
  INDEXSTORE_SYMBOL_KIND_VARIABLE = 13,
  INDEXSTORE_SYMBOL_KIND_FIELD = 14,
  INDEXSTORE_SYMBOL_KIND_ENUMCONSTANT = 15,
  INDEXSTORE_SYMBOL_KIND_INSTANCEMETHOD = 16,
  INDEXSTORE_SYMBOL_KIND_CLASSMETHOD = 17,
  INDEXSTORE_SYMBOL_KIND_STATICMETHOD = 18,
  INDEXSTORE_SYMBOL_KIND_INSTANCEPROPERTY = 19,
  INDEXSTORE_SYMBOL_KIND_CLASSPROPERTY = 20,
  INDEXSTORE_SYMBOL_KIND_STATICPROPERTY = 21,
  INDEXSTORE_SYMBOL_KIND_CONSTRUCTOR = 22,
  INDEXSTORE_SYMBOL_KIND_DESTRUCTOR = 23,
  INDEXSTORE_SYMBOL_KIND_CONVERSIONFUNCTION = 24,
} indexstore_symbol_kind_t;

typedef enum {
  INDEXSTORE_SYMBOL_SUB_KIND_NONE = 0,
  INDEXSTORE_SYMBOL_SUB_KIND_GENERIC = 1,
  INDEXSTORE_SYMBOL_SUB_KIND_TEMPLATE_PARTIAL_SPECIALIZATION = 2,
  INDEXSTORE_SYMBOL_SUB_KIND_TEMPLATE_SPECIALIZATION = 3,
} indexstore_symbol_sub_kind_t;

typedef enum {
  INDEXSTORE_SYMBOL_LANG_C = 0,
  INDEXSTORE_SYMBOL_LANG_OBJC = 1,
  INDEXSTORE_SYMBOL_LANG_CXX = 2,
} indexstore_symbol_language_t;

typedef enum {
  INDEXSTORE_SYMBOL_ROLE_DECLARATION = 1 << 0,
  INDEXSTORE_SYMBOL_ROLE_DEFINITION  = 1 << 1,
  INDEXSTORE_SYMBOL_ROLE_REFERENCE   = 1 << 2,
  INDEXSTORE_SYMBOL_ROLE_READ        = 1 << 3,
  INDEXSTORE_SYMBOL_ROLE_WRITE       = 1 << 4,
  INDEXSTORE_SYMBOL_ROLE_CALL        = 1 << 5,
  INDEXSTORE_SYMBOL_ROLE_DYNAMIC     = 1 << 6,
  INDEXSTORE_SYMBOL_ROLE_ADDRESSOF   = 1 << 7,
  INDEXSTORE_SYMBOL_ROLE_IMPLICIT    = 1 << 8,

  // Relation roles.
  INDEXSTORE_SYMBOL_ROLE_REL_CHILDOF     = 1 << 9,
  INDEXSTORE_SYMBOL_ROLE_REL_BASEOF      = 1 << 10,
  INDEXSTORE_SYMBOL_ROLE_REL_OVERRIDEOF  = 1 << 11,
  INDEXSTORE_SYMBOL_ROLE_REL_RECEIVEDBY  = 1 << 12,
  INDEXSTORE_SYMBOL_ROLE_REL_CALLEDBY    = 1 << 13,
} indexstore_symbol_role_t;

INDEXSTORE_PUBLIC indexstore_symbol_language_t
indexstore_symbol_get_language(indexstore_symbol_t);

INDEXSTORE_PUBLIC indexstore_symbol_kind_t
indexstore_symbol_get_kind(indexstore_symbol_t);

INDEXSTORE_PUBLIC indexstore_symbol_sub_kind_t
indexstore_symbol_get_sub_kind(indexstore_symbol_t);

INDEXSTORE_PUBLIC uint64_t
indexstore_symbol_get_roles(indexstore_symbol_t);

INDEXSTORE_PUBLIC uint64_t
indexstore_symbol_get_related_roles(indexstore_symbol_t);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_symbol_get_name(indexstore_symbol_t);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_symbol_get_usr(indexstore_symbol_t);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_symbol_get_codegen_name(indexstore_symbol_t);

typedef void *indexstore_symbol_relation_t;

INDEXSTORE_PUBLIC uint64_t
indexstore_symbol_relation_get_roles(indexstore_symbol_relation_t);

INDEXSTORE_PUBLIC indexstore_symbol_t
indexstore_symbol_relation_get_symbol(indexstore_symbol_relation_t);

typedef void *indexstore_occurrence_t;

INDEXSTORE_PUBLIC indexstore_symbol_t
indexstore_occurrence_get_symbol(indexstore_occurrence_t);

#if INDEXSTORE_HAS_BLOCKS
INDEXSTORE_PUBLIC bool
indexstore_occurrence_relations_apply(indexstore_occurrence_t,
                      bool(^applier)(indexstore_symbol_relation_t symbol_rel));
#endif

INDEXSTORE_PUBLIC uint64_t
indexstore_occurrence_get_roles(indexstore_occurrence_t);

INDEXSTORE_PUBLIC void
indexstore_occurrence_get_line_col(indexstore_occurrence_t,
                              unsigned *line, unsigned *column);

typedef void *indexstore_record_reader_t;

INDEXSTORE_PUBLIC indexstore_record_reader_t
indexstore_record_reader_create(indexstore_t store, const char *record_name,
                                indexstore_error_t *error);

INDEXSTORE_PUBLIC void
indexstore_record_reader_dispose(indexstore_record_reader_t);

#if INDEXSTORE_HAS_BLOCKS
/// Goes through the symbol data and passes symbols to \c receiver, for the
/// symbol data that \c filter returns true on.
///
/// This allows allocating memory only for the record symbols that the caller is
/// interested in.
INDEXSTORE_PUBLIC bool
indexstore_record_reader_search_symbols(indexstore_record_reader_t,
    bool(^filter)(indexstore_symbol_t symbol, bool *stop),
    void(^receiver)(indexstore_symbol_t symbol));

/// \param nocache if true, avoids allocating memory for the symbols.
/// Useful when the caller does not intend to keep \c indexstore_record_reader_t
/// for more queries.
INDEXSTORE_PUBLIC bool
indexstore_record_reader_symbols_apply(indexstore_record_reader_t,
                                       bool nocache,
                                    bool(^applier)(indexstore_symbol_t symbol));

INDEXSTORE_PUBLIC bool
indexstore_record_reader_occurrences_apply(indexstore_record_reader_t,
                                 bool(^applier)(indexstore_occurrence_t occur));

/// \param symbols if non-zero \c symbols_count, indicates the list of symbols
/// that we want to get occurrences for. An empty array indicates that we want
/// occurrences for all symbols.
/// \param related_symbols Same as \c symbols but for related symbols.
INDEXSTORE_PUBLIC bool
indexstore_record_reader_occurrences_of_symbols_apply(indexstore_record_reader_t,
        indexstore_symbol_t *symbols, size_t symbols_count,
        indexstore_symbol_t *related_symbols, size_t related_symbols_count,
        bool(^applier)(indexstore_occurrence_t occur));
#endif


typedef void *indexstore_unit_reader_t;

INDEXSTORE_PUBLIC indexstore_unit_reader_t
indexstore_unit_reader_create(indexstore_t store, const char *unit_name,
                              indexstore_error_t *error);

INDEXSTORE_PUBLIC void
indexstore_unit_reader_dispose(indexstore_unit_reader_t);

INDEXSTORE_PUBLIC void
indexstore_unit_reader_get_modification_time(indexstore_unit_reader_t,
                                             int64_t *seconds,
                                             int64_t *nanoseconds);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_unit_reader_get_working_dir(indexstore_unit_reader_t);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_unit_reader_get_output_file(indexstore_unit_reader_t);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_unit_reader_get_target(indexstore_unit_reader_t);

INDEXSTORE_PUBLIC size_t
indexstore_unit_reader_dependencies_count(indexstore_unit_reader_t);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_unit_reader_get_dependency_filepath(indexstore_unit_reader_t, size_t index);

#if INDEXSTORE_HAS_BLOCKS
INDEXSTORE_PUBLIC bool
indexstore_unit_reader_dependencies_filepaths_apply(indexstore_unit_reader_t,
                                  bool(^applier)(indexstore_string_ref_t path));
#endif

typedef void *indexstore_unit_dependency_t;

typedef enum {
  INDEXSTORE_UNIT_DEPENDENCY_UNIT = 1,
  INDEXSTORE_UNIT_DEPENDENCY_RECORD = 2,
} indexstore_unit_dependency_kind_t;

INDEXSTORE_PUBLIC indexstore_unit_dependency_kind_t
indexstore_unit_dependency_get_kind(indexstore_unit_dependency_t);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_unit_dependency_get_filepath(indexstore_unit_dependency_t);

INDEXSTORE_PUBLIC indexstore_string_ref_t
indexstore_unit_dependency_get_name(indexstore_unit_dependency_t);

INDEXSTORE_PUBLIC int
indexstore_unit_dependency_get_index(indexstore_unit_dependency_t);

#if INDEXSTORE_HAS_BLOCKS
INDEXSTORE_PUBLIC bool
indexstore_unit_reader_dependencies_apply(indexstore_unit_reader_t,
                             bool(^applier)(indexstore_unit_dependency_t));
#endif

INDEXSTORE_END_DECLS

#endif
