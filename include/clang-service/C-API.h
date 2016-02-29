#ifndef LLVM_CLANG_CLANGSERVICE_C_API_H
#define LLVM_CLANG_CLANGSERVICE_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * \brief Create resource identifiers.
 */

typedef void *CXSUID;

CXSUID CXSUID_create(const char *uid);

const char *CXSUID_get_name(CXSUID cxs_uid);


/**
 * \brief Create, inspect, and free CXSValues.
 */

typedef struct {
  void *opaque1;
  unsigned opaque2[2];
} CXSValue;

enum CXSValue_Kind {
  CXSValue_Null,
  CXSValue_Bool,
  CXSValue_Int64,
  CXSValue_UID,
  CXSValue_String,
  CXSValue_Data,
  CXSValue_Array,
  CXSValue_Dict
};

CXSValue CXSValue_null_create(void);

CXSValue CXSValue_bool_create(bool value);

bool CXSValue_bool_get_value(CXSValue xbool);

CXSValue CXSValue_int64_create(int64_t value);

int64_t CXSValue_int64_get_value(CXSValue xint);

CXSValue CXSValue_uid_create(CXSUID cxs_uid);

CXSUID CXSValue_uid_get_value(CXSValue xuid);

CXSValue CXSValue_string_create(const char *string, bool no_copy);

size_t CXSValue_string_get_length(CXSValue xstring);

const char *CXSValue_string_get_string_ptr(CXSValue xstring);

CXSValue CXSValue_data_create(char *bytes, size_t length, bool no_copy);

size_t CXSValue_data_get_length(CXSValue xdata);

const char *CXSValue_data_get_bytes_ptr(CXSValue xdata);

CXSValue CXSValue_array_create();

/* The array takes ownership of the value. */
void CXSValue_array_append_value(CXSValue array, CXSValue *value);

/* The returned value is owned by the array, and should not be freed. */
CXSValue CXSValue_array_get_value(CXSValue array, size_t index);

size_t CXSValue_array_get_count(CXSValue array);

CXSValue CXSValue_dictionary_create();

/* The dictionary takes ownership of the value. */
void CXSValue_dictionary_set_value(CXSValue dictionary, CXSUID key,
                                   CXSValue *value);

/* The returned value is owned by the dictionary, and should not be freed. */
CXSValue CXSValue_dictionary_get_value(CXSValue dictionary, CXSUID key);

size_t CXSValue_dictionary_get_count(CXSValue dictionary);

/* Returns true to keep iterating. */
typedef bool (*CXSValue_dictionary_applier_t)(CXSUID key,
                                              CXSValue cxs_value);

void CXSValue_dictionary_apply(CXSValue dictionary,
                               CXSValue_dictionary_applier_t applier);

enum CXSValue_Kind CXSValue_get_kind(CXSValue cxs_value);

void CXSValue_free(CXSValue *cxs_value);


/**
 * \brief Create and free Clients, which are used to message the clang service.
 */

typedef void *CXSClient;

enum CXSClient_Kind {
  CXSClient_InProcess,
  CXSClient_XPC
};

CXSClient CXSClient_create(enum CXSClient_Kind client_kind);

/*
 * The client takes ownership of the request.
 *
 * The returned value must be freed.
 */
CXSValue CXSClient_request(CXSClient client, CXSValue *request);

typedef void (*CXSClient_handler_t)(CXSValue response);

/*
 * The client takes ownership of the request.
 *
 * The returned value must be freed.
 */
void CXSClient_request_async(CXSClient client, CXSValue *request,
                             CXSClient_handler_t handler);

void CXSClient_free(CXSClient client);

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // LLVM_CLANG_CLANGSERVICE_C_API_H
