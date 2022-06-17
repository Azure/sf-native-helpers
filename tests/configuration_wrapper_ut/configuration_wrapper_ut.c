// Copyright (c) Microsoft. All rights reserved.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <wchar.h>

#include "fabricruntime.h"
#include "fabrictypes.h"

#include "macro_utils/macro_utils.h"

#include "real_gballoc_ll.h"
static void* my_gballoc_malloc(size_t size)
{
    return real_gballoc_ll_malloc(size);
}

static void my_gballoc_free(void* ptr)
{
    real_gballoc_ll_free(ptr);
}

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_charptr.h"
#include "umock_c/umocktypes_wcharptr.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/string_utils.h"
#include "c_util/rc_string.h"
#include "c_util/thandle.h"
#include "sf_c_util/configuration_reader.h"
#undef ENABLE_MOCKS

// Must include umock_c_prod so mocks are not expanded in real_rc_string
#include "umock_c/umock_c_prod.h"

#include "real_gballoc_hl.h"
#include "real_rc_string.h"
#include "real_string_utils.h"
#include "c_util_test_helpers/rc_string_test_decl.h"
#include "c_util_test_helpers/rc_string_test_type.h"

#include "configuration_wrapper_ut_helpers.h"
#include "test_configuration_wrapper.h"

#include "sf_c_util/configuration_wrapper.h"

CTEST_DECLARE_EQUALITY_ASSERTION_FUNCTIONS_FOR_TYPE(TEST_THANDLE_RC_STRING);
CTEST_DEFINE_EQUALITY_ASSERTION_FUNCTIONS_FOR_TYPE(TEST_THANDLE_RC_STRING, );

static TEST_MUTEX_HANDLE test_serialize_mutex;


RC_STRING_TEST_DECL(
    empty_string, ""
)

typedef THANDLE(RC_STRING) thandle_rc_string;

static const wchar_t* expected_config_package_name = L"default_config";
static const wchar_t* expected_section_name = L"MyConfigSectionName";

TEST_CONFIGURATION_WRAPPER_DEFINE_CONFIGURATION_READER_HOOKS(my_config, MY_CONFIG_TEST_PARAMS)

TEST_CONFIGURATION_WRAPPER_DEFINE_EXPECTED_CALL_HELPERS(my_config, expected_config_package_name, expected_section_name, MY_CONFIG_TEST_PARAMS)


MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));

    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_wcharptr_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_THANDLE_RC_STRING_register_types(), "umocktypes_THANDLE_RC_STRING_register_types");

    REGISTER_RC_STRING_GLOBAL_MOCK_HOOKS();
    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(malloc, my_gballoc_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc_2, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(realloc_flex, NULL);
    REGISTER_GLOBAL_MOCK_HOOK(free, my_gballoc_free);

    TEST_CONFIGURATION_WRAPPER_HOOK_CONFIGURATION_READER(my_config)

    rc_string_test_init_statics();
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    rc_string_test_cleanup_statics();

    TEST_MUTEX_DESTROY(test_serialize_mutex);

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    // Initialize so each default value is returned (individual tests can override what gets returned)
    TEST_CONFIGURATION_WRAPPER_RESET(MY_CONFIG_TEST_PARAMS);

    umock_c_reset_all_calls();
    umock_c_negative_tests_init();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();

    TEST_CONFIGURATION_WRAPPER_CLEANUP(MY_CONFIG_TEST_PARAMS);

    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_001: [ DECLARE_CONFIGURATION_WRAPPER shall generate a THANDLE declaration of type CONFIGURATION_WRAPPER(name). ]*/
static THANDLE(my_config_CONFIGURATION) my_config_CONFIGURATION_has_THANDLE = NULL;

// Tested implicitly in the cases below
/*Tests_SRS_CONFIGURATION_WRAPPER_42_004: [ DEFINE_CONFIGURATION_WRAPPER shall generate the CONFIGURATION_WRAPPER(name) struct. ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_005: [ DEFINE_CONFIGURATION_WRAPPER shall generate the implementation of CONFIGURATION_WRAPPER_CREATE(name). ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_006: [ DECLARE_CONFIGURATION_WRAPPER shall generate the implementation of the getter functions CONFIGURATION_WRAPPER_GETTER(name, param) for each of the configurations provided. ]*/

/*Tests_SRS_CONFIGURATION_WRAPPER_42_007: [ CONFIGURATION_WRAPPER shall expand to the name of the configuration module by appending the suffix _CONFIGURATION. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_macro_expands_to_name)
{
    // arrange

    // act
    const char* name = MU_TOSTRING(CONFIGURATION_WRAPPER(name));

    // assert
    ASSERT_ARE_EQUAL(char_ptr, "name_CONFIGURATION", name);
}

//
// CONFIGURATION_WRAPPER_CREATE
//

/*Tests_SRS_CONFIGURATION_WRAPPER_42_008: [ CONFIGURATION_WRAPPER_CREATE shall expand to the name of the create function for the configuration module by appending the suffix _configuration_create. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_macro_expands_to_name)
{
    // arrange

    // act
    const char* name = MU_TOSTRING(CONFIGURATION_WRAPPER_CREATE(name));

    // assert
    ASSERT_ARE_EQUAL(char_ptr, "name_configuration_create", name);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_009: [ If activation_context is NULL then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_NULL_activation_context_fails)
{
    // arrange

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_002: [ DECLARE_CONFIGURATION_WRAPPER shall generate a mockable create function CONFIGURATION_WRAPPER_CREATE(name) which takes an IFabricCodePackageActivationContext* and produces the THANDLE. ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_010: [ CONFIGURATION_WRAPPER_CREATE(name) shall allocate the THANDLE(CONFIGURATION_WRAPPER(name)) with MU_C2A(CONFIGURATION_WRAPPER(name), _dispose) as the dispose function. ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_011: [ CONFIGURATION_WRAPPER_CREATE(name) shall call AddRef and store the activation_context. ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_012: [ CONFIGURATION_WRAPPER_CREATE(name) shall store the sf_config_name and sf_parameters_section_name. ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_013: [ For each configuration value with name config_name: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_014: [ If the type is bool then: ]*/
        /*Tests_SRS_CONFIGURATION_WRAPPER_42_015: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_bool with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_016: [ If the type is uint32_t then: ]*/
        /*Tests_SRS_CONFIGURATION_WRAPPER_42_017: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_uint32_t with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_019: [ If the type is uint64_t then: ]*/
        /*Tests_SRS_CONFIGURATION_WRAPPER_42_020: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_uint64_t with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_022: [ If the type is char_ptr then: ]*/
        /*Tests_SRS_CONFIGURATION_WRAPPER_42_023: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_char_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_026: [ If the type is wchar_ptr then: ]*/
        /*Tests_SRS_CONFIGURATION_WRAPPER_42_027: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_wchar_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_030: [ If the type is thandle_rc_string then: ]*/
        /*Tests_SRS_CONFIGURATION_WRAPPER_42_031: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_thandle_rc_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_normal_values_for_all_types_succeeds)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&result, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_034: [ If there are any errors then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_fails_when_underlying_functions_fail)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            /// act
            THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

            /// assert
            ASSERT_IS_NULL(result, "On failed call %zu", i);
        }
    }
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_016: [ If the type is uint32_t then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_017: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_uint32_t with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_018: [ If the result is UINT32_MAX then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_fails_when_uint32_t_value_is_UINT32_MAX)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(parameter_3) = UINT32_MAX;
    TEST_CONFIGURATION_WRAPPER_EXPECT_READ_UP_TO(my_config)(TEST_CONFIGURATION_WRAPPER_INDEX_OF_CONFIG(my_config, parameter_3));

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_019: [ If the type is uint64_t then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_020: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_uint64_t with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_021: [ If the result is UINT64_MAX then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_fails_when_uint64_t_value_is_UINT64_MAX)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(parameter_1) = UINT64_MAX;
    TEST_CONFIGURATION_WRAPPER_EXPECT_READ_UP_TO(my_config)(TEST_CONFIGURATION_WRAPPER_INDEX_OF_CONFIG(my_config, parameter_1));

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_022: [ If the type is char_ptr then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_023: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_char_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_024: [ If the value is an empty string then CONFIGURATION_WRAPPER_CREATE(name) shall free the string and set it to NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_empty_optional_string_succeeds)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_optional) = "";
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&result, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_026: [ If the type is wchar_ptr then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_027: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_wchar_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_028: [ If the value is an empty string then CONFIGURATION_WRAPPER_CREATE(name) shall free the string and set it to NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_empty_optional_wide_string_succeeds)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(wide_string_option_optional) = L"";
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&result, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_030: [ If the type is thandle_rc_string then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_031: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_thandle_rc_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_032: [ If the value is an empty string then CONFIGURATION_WRAPPER_CREATE(name) shall free the string and set it to NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_empty_optional_thandle_rc_string_succeeds)
{
    // arrange
    THANDLE_ASSIGN(real_RC_STRING)(&TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_in_thandle_optional).x, g.empty_string);
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&result, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_022: [ If the type is char_ptr then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_023: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_char_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_024: [ If the value is an empty string then CONFIGURATION_WRAPPER_CREATE(name) shall free the string and set it to NULL. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_025: [ If the configuration value is CONFIG_REQUIRED and the value is NULL then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_empty_required_string_fails)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option) = "";
    TEST_CONFIGURATION_WRAPPER_EXPECT_READ_UP_TO(my_config)(TEST_CONFIGURATION_WRAPPER_INDEX_OF_CONFIG(my_config, string_option));

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_022: [ If the type is char_ptr then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_023: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_char_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_025: [ If the configuration value is CONFIG_REQUIRED and the value is NULL then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_NULL_required_string_fails)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option) = NULL;
    TEST_CONFIGURATION_WRAPPER_EXPECT_READ_UP_TO(my_config)(TEST_CONFIGURATION_WRAPPER_INDEX_OF_CONFIG(my_config, string_option));

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_026: [ If the type is wchar_ptr then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_027: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_wchar_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_028: [ If the value is an empty string then CONFIGURATION_WRAPPER_CREATE(name) shall free the string and set it to NULL. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_029: [ If the configuration value is CONFIG_REQUIRED and the value is NULL then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_empty_required_wide_string_fails)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(wide_string_option) = L"";
    TEST_CONFIGURATION_WRAPPER_EXPECT_READ_UP_TO(my_config)(TEST_CONFIGURATION_WRAPPER_INDEX_OF_CONFIG(my_config, wide_string_option));

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_026: [ If the type is wchar_ptr then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_027: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_wchar_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_029: [ If the configuration value is CONFIG_REQUIRED and the value is NULL then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_NULL_required_wide_string_fails)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(wide_string_option) = NULL;
    TEST_CONFIGURATION_WRAPPER_EXPECT_READ_UP_TO(my_config)(TEST_CONFIGURATION_WRAPPER_INDEX_OF_CONFIG(my_config, wide_string_option));

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_030: [ If the type is thandle_rc_string then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_031: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_thandle_rc_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_032: [ If the value is an empty string then CONFIGURATION_WRAPPER_CREATE(name) shall free the string and set it to NULL. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_033: [ If the configuration value is CONFIG_REQUIRED and the value is NULL then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_empty_required_thandle_rcstring_fails)
{
    // arrange
    THANDLE_ASSIGN(real_RC_STRING)(&TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_in_thandle).x, g.empty_string);
    TEST_CONFIGURATION_WRAPPER_EXPECT_READ_UP_TO(my_config)(TEST_CONFIGURATION_WRAPPER_INDEX_OF_CONFIG(my_config, string_option_in_thandle));

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_030: [ If the type is thandle_rc_string then: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_031: [ CONFIGURATION_WRAPPER_CREATE(name) shall call configuration_reader_get_thandle_rc_string with the activation_context, sf_config_name, sf_parameters_section_name, and CONFIGURATION_WRAPPER_PARAMETER_NAME_config_name. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_033: [ If the configuration value is CONFIG_REQUIRED and the value is NULL then CONFIGURATION_WRAPPER_CREATE(name) shall fail and return NULL. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_CREATE_with_NULL_required_thandle_rcstring_fails)
{
    // arrange
    THANDLE_ASSIGN(real_RC_STRING)(&TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_in_thandle).x, NULL);
    TEST_CONFIGURATION_WRAPPER_EXPECT_READ_UP_TO(my_config)(TEST_CONFIGURATION_WRAPPER_INDEX_OF_CONFIG(my_config, string_option_in_thandle));

    // act
    THANDLE(CONFIGURATION_WRAPPER(my_config)) result = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// Dispose
//

/*Tests_SRS_CONFIGURATION_WRAPPER_42_035: [ For each config value: ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_036: [ If the type is char_ptr then MU_C2A(CONFIGURATION_WRAPPER(name), _dispose) shall free the string. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_038: [ If the type is wchar_ptr then MU_C2A(CONFIGURATION_WRAPPER(name), _dispose) shall free the string. ]*/
    /*Tests_SRS_CONFIGURATION_WRAPPER_42_040: [ If the type is thandle_rc_string then MU_C2A(CONFIGURATION_WRAPPER(name), _dispose) shall assign the THANDLE to NULL. ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_042: [ MU_C2A(CONFIGURATION_WRAPPER(name), _dispose) shall Release the activation_context. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_Dispose_frees_all_strings)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    TEST_CONFIGURATION_WRAPPER_EXPECT_DESTROY(my_config)();

    // act
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_042: [ MU_C2A(CONFIGURATION_WRAPPER(name), _dispose) shall Release the activation_context. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_Dispose_works_when_optional_strings_are_null)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_optional) = "";
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(wide_string_option_optional) = L"";
    THANDLE_ASSIGN(real_RC_STRING)(&TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_in_thandle_optional).x, g.empty_string);

    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    TEST_CONFIGURATION_WRAPPER_EXPECT_DESTROY(my_config)();

    // act
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

//
// CONFIGURATION_WRAPPER_GETTER
//

/*Tests_SRS_CONFIGURATION_WRAPPER_42_043: [ CONFIGURATION_WRAPPER_GETTER shall expand to the name of the getter function for the configuration module and the given param by concatenating the name, the string _configuration_get, and the param. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_macro_expands_to_name_of_getter_function)
{
    // arrange

    // act
    const char* name = MU_TOSTRING(CONFIGURATION_WRAPPER_GETTER(name, parameter));

    // assert
    ASSERT_ARE_EQUAL(char_ptr, "name_configuration_get_parameter", name);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_044: [ If handle is NULL then CONFIGURATION_WRAPPER_GETTER(name, field_name) shall fail and return... ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_045: [ ...false if the type is bool ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_bool_with_NULL_handle_returns_false)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    bool result = CONFIGURATION_WRAPPER_GETTER(my_config, some_flag)(NULL);

    // assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_044: [ If handle is NULL then CONFIGURATION_WRAPPER_GETTER(name, field_name) shall fail and return... ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_046: [ ...UINT32_MAX if the type is uint32_t ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_uint32_t_with_NULL_handle_returns_UINT32_MAX)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    uint32_t result = CONFIGURATION_WRAPPER_GETTER(my_config, parameter_3)(NULL);

    // assert
    ASSERT_ARE_EQUAL(uint32_t, UINT32_MAX, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_044: [ If handle is NULL then CONFIGURATION_WRAPPER_GETTER(name, field_name) shall fail and return... ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_047: [ ...UINT64_MAX if the type is uint64_t ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_uint64_t_with_NULL_handle_returns_UINT64_MAX)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    uint64_t result = CONFIGURATION_WRAPPER_GETTER(my_config, parameter_1)(NULL);

    // assert
    ASSERT_ARE_EQUAL(uint64_t, UINT64_MAX, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_044: [ If handle is NULL then CONFIGURATION_WRAPPER_GETTER(name, field_name) shall fail and return... ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_048: [ ...NULL otherwise ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_char_ptr_with_NULL_handle_returns_NULL)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    const char* result = CONFIGURATION_WRAPPER_GETTER(my_config, string_option)(NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_044: [ If handle is NULL then CONFIGURATION_WRAPPER_GETTER(name, field_name) shall fail and return... ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_048: [ ...NULL otherwise ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_wchar_ptr_with_NULL_handle_returns_NULL)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    const wchar_t* result = CONFIGURATION_WRAPPER_GETTER(my_config, wide_string_option)(NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_044: [ If handle is NULL then CONFIGURATION_WRAPPER_GETTER(name, field_name) shall fail and return... ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_048: [ ...NULL otherwise ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_thandle_rc_string_with_NULL_handle_returns_NULL)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));

    // act
    THANDLE(RC_STRING) result = CONFIGURATION_WRAPPER_GETTER(my_config, string_option_in_thandle)(NULL);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_003: [ DECLARE_CONFIGURATION_WRAPPER shall generate mockable getter functions CONFIGURATION_WRAPPER_GETTER(name, param) for each of the configurations provided. ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_bool_with_true_value_returns_true)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(some_flag) = true;
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    bool result = CONFIGURATION_WRAPPER_GETTER(my_config, some_flag)(config);

    // assert
    ASSERT_IS_TRUE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_bool_with_false_value_returns_false)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(some_flag) = false;
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    bool result = CONFIGURATION_WRAPPER_GETTER(my_config, some_flag)(config);

    // assert
    ASSERT_IS_FALSE(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_uint32_t_returns_value)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(parameter_3) = 12345;
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    uint32_t result = CONFIGURATION_WRAPPER_GETTER(my_config, parameter_3)(config);

    // assert
    ASSERT_ARE_EQUAL(uint32_t, 12345, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_uint64_t_returns_value)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(parameter_2) = 0x12345678FFFFFFFF;
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    uint64_t result = CONFIGURATION_WRAPPER_GETTER(my_config, parameter_2)(config);

    // assert
    ASSERT_ARE_EQUAL(uint64_t, 0x12345678FFFFFFFF, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_const_char_returns_string)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option) = "parameter value";
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    const char* result = CONFIGURATION_WRAPPER_GETTER(my_config, string_option)(config);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, "parameter value", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_optional_const_char_empty_returns_NULL)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_optional) = "";
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    const char* result = CONFIGURATION_WRAPPER_GETTER(my_config, string_option_optional)(config);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_const_wchar_returns_string)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(wide_string_option) = L"parameter value";
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    const wchar_t* result = CONFIGURATION_WRAPPER_GETTER(my_config, wide_string_option)(config);

    // assert
    ASSERT_ARE_EQUAL(wchar_ptr, L"parameter value", result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_optional_const_wchar_empty_returns_NULL)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(wide_string_option_optional) = L"";
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    // act
    const wchar_t* result = CONFIGURATION_WRAPPER_GETTER(my_config, wide_string_option_optional)(config);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_049: [ If the type is thandle_rc_string then the returned value will be set using THANDLE_INITIALIZE and the caller will have a reference they must free. ]*/
/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_rc_string_returns_string)
{
    // arrange
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_in_thandle).x));

    // act
    THANDLE(RC_STRING) result = CONFIGURATION_WRAPPER_GETTER(my_config, string_option_in_thandle)(config);

    // assert
    ASSERT_ARE_EQUAL(TEST_THANDLE_RC_STRING, TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_in_thandle).x, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(RC_STRING)(&result, NULL);
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

/*Tests_SRS_CONFIGURATION_WRAPPER_42_050: [ CONFIGURATION_WRAPPER_GETTER(name, field_name) shall return the configuration value for field_name. ]*/
TEST_FUNCTION(CONFIGURATION_WRAPPER_GETTER_for_rc_string_with_NULL_returns_NULL)
{
    // arrange
    THANDLE_ASSIGN(real_RC_STRING)(&TEST_CONFIGURATION_WRAPPER_VALUE_TO_RETURN(string_option_in_thandle_optional).x, g.empty_string);
    TEST_CONFIGURATION_WRAPPER_EXPECT_ALL_READ(my_config)();
    THANDLE(CONFIGURATION_WRAPPER(my_config)) config = CONFIGURATION_WRAPPER_CREATE(my_config)(TEST_CONFIGURATION_WRAPPER_ACTIVATION_CONTEXT(my_config));
    ASSERT_IS_NOT_NULL(config);

    STRICT_EXPECTED_CALL(THANDLE_INITIALIZE(RC_STRING)(IGNORED_ARG, NULL));

    // act
    THANDLE(RC_STRING) result = CONFIGURATION_WRAPPER_GETTER(my_config, string_option_in_thandle_optional)(config);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(CONFIGURATION_WRAPPER(my_config))(&config, NULL);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
