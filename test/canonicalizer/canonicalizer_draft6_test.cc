#include <sourcemeta/core/test.h>

#include <sourcemeta/blaze/compiler.h>
#include <sourcemeta/blaze/evaluator.h>

#include <sourcemeta/core/json.h>
#include <sourcemeta/core/jsonschema.h>

#include <filesystem> // std::filesystem::path
#include <memory>     // std::unique_ptr

#include "canonicalizer_test_utils.h"

namespace {
auto compiled_metaschema() -> const sourcemeta::blaze::Template & {
  static const sourcemeta::blaze::Template SCHEMA_TEMPLATE{
      sourcemeta::blaze::compile(
          sourcemeta::core::read_json(std::filesystem::path{SCHEMAS_PATH} /
                                      "canonical-draft6.json"),
          sourcemeta::core::schema_walker, sourcemeta::core::schema_resolver,
          sourcemeta::blaze::default_schema_compiler)};
  return SCHEMA_TEMPLATE;
}
} // namespace

// Every other Draft 6 canonicalization case lives in
// `test/canonicalizer/draft6` as a JSON fixture. These cannot: the
// first group canonicalizes to a boolean, which carries no dialect and
// so cannot be compared as a document, and the second is not a valid
// Draft 6 schema to begin with, which the fixture runner insists on.

TEST(unsatisfiable_empty_enum) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "enum": []
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON(false)JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(type_boolean_as_enum_2) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "boolean",
    "enum": [ 1, 2, 3 ]
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON(
    false
  )JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(type_null_as_enum_2) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "null",
    "enum": [ 1, 2, 3 ]
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON(
    false
  )JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(const_with_incompatible_type) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "string",
    "const": 42
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON(
    false
  )JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(const_null_with_string_type) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "string",
    "const": null
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON(
    false
  )JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(enum_duplicate_values_removed) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "enum": [ 1, 2, 1, 3, 2 ]
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "allOf": [
      {
        "$ref": "#/definitions/0"
      }
    ],
    "definitions": {
      "0": {
        "enum": [
          1,
          2,
          3
        ]
      }
    }
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(integer_enum_with_real_equivalent) {
  auto document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": "integer",
    "enum": [ 3, 3.0, "hello" ]
  })JSON");

  const auto expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "allOf": [
      {
        "$ref": "#/definitions/0"
      }
    ],
    "definitions": {
      "0": {
        "allOf": [
          {
            "$ref": "#/definitions/1"
          },
          {
            "$ref": "#/definitions/2"
          }
        ]
      },
      "1": {
        "type": "integer",
        "multipleOf": 1
      },
      "2": {
        "enum": [
          3
        ]
      }
    }
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}

TEST(type_array_drops_unknown_name) {
  sourcemeta::core::JSON document = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "type": [ "string", "foo" ]
  })JSON");

  const sourcemeta::core::JSON expected = sourcemeta::core::parse_json(R"JSON({
    "$schema": "http://json-schema.org/draft-06/schema#",
    "allOf": [
      {
        "$ref": "#/definitions/0"
      }
    ],
    "definitions": {
      "0": {
        "type": "string",
        "minLength": 0
      }
    }
  })JSON");

  CANONICALIZE_AND_VALIDATE(document, expected, compiled_metaschema());
}
