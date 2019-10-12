#include "cyaml/cyaml.h"

typedef struct request {
  const char *fileType;
} request_t;

typedef struct requestInfo {
  request_t* requests;
  uint64_t requests_count;
}requestInfo_t;


static const cyaml_schema_field_t fileTypeSchema[] = {
  CYAML_FIELD_STRING_PTR(
      "FILE_TYPE", CYAML_FLAG_POINTER,
      request_t, fileType, 0, CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t requestTypeSchema = {
  CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
      request_t, fileTypeSchema),
};

static const cyaml_schema_field_t requestInfoFieldSchema[] = {
  CYAML_FIELD_SEQUENCE("REQUESTS", CYAML_FLAG_POINTER, requestInfo_t,
      requests, &requestTypeSchema, 0, CYAML_UNLIMITED),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t requestSchema = {
  CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER, requestInfo_t,
      requestInfoFieldSchema),
};

static const cyaml_config_t config = {
  .log_level = CYAML_LOG_WARNING,
  .log_fn = cyaml_log,
  .mem_fn = cyaml_mem
};

