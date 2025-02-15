
/*
  Copyright(c) 2015 - 2022 Denis Blank <denis.blank at outlook dot com>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files(the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions :

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
**/

#include <string>

#include <continuable/detail/features.hpp>

#include <test-continuable.hpp>

using namespace cti;

TYPED_TEST(single_dimension_tests, are_completing_errors) {
  ASSERT_ASYNC_EXCEPTION_COMPLETION(
      this->supply_exception(supply_test_exception()));
}

TYPED_TEST(single_dimension_tests, are_yielding_error_result) {
  ASSERT_ASYNC_EXCEPTION_RESULT(this->supply_exception(supply_test_exception()),
                                get_test_exception_proto());
}

TYPED_TEST(single_dimension_tests, are_completed_after_error_handled) {
  auto handled = std::make_shared<bool>(false);
  auto continuation = this->supply_exception(supply_test_exception())
                          .fail([handled](cti::exception_t) {
                            ASSERT_FALSE(*handled);
                            *handled = true;
                          });

  ASSERT_ASYNC_COMPLETION(std::move(continuation));
  ASSERT_TRUE(*handled);
}

TYPED_TEST(single_dimension_tests, are_recoverable_after_error_handled) {
  auto recovered = std::make_shared<bool>(false);
  auto continuation = this->supply_exception(supply_test_exception())
                          .fail([](cti::exception_t){})
                          .then([recovered]{
                            ASSERT_FALSE(*recovered);
                            *recovered = true;
                          });

  ASSERT_ASYNC_COMPLETION(std::move(continuation));
  ASSERT_TRUE(*recovered);
}

TYPED_TEST(single_dimension_tests, fail_is_accepting_plain_continuables) {
  auto handled = std::make_shared<bool>(false);
  auto handler = this->supply().then([handled] {
    ASSERT_FALSE(*handled);
    *handled = true;
  });

  auto continuation =
      this->supply_exception(supply_test_exception()).fail(std::move(handler));

  ASSERT_ASYNC_COMPLETION(std::move(continuation));
  ASSERT_TRUE(*handled);
}

#if !defined(CONTINUABLE_WITH_NO_EXCEPTIONS)
// Enable this test only if we support exceptions
TYPED_TEST(single_dimension_tests, are_yielding_errors_from_handlers) {
  auto continuation = this->supply().then([] {
    // Throw an error from inside the handler
    throw test_exception();
  });

  ASSERT_ASYNC_EXCEPTION_RESULT(std::move(continuation),
                                get_test_exception_proto());
}
#endif

TYPED_TEST(single_dimension_tests, are_result_error_accepting) {
  auto handled = std::make_shared<bool>(false);
  auto continuation = this->supply(supply_test_exception())
                          .next(fu2::overload(
                              [handled] {
                                ASSERT_FALSE(*handled);
                                *handled = true;
                              },
                              [](cti::exception_arg_t, cti::exception_t) {
                                // ...
                                FAIL();
                              }));

  ASSERT_ASYNC_COMPLETION(std::move(continuation));
  ASSERT_TRUE(*handled);
}

TYPED_TEST(single_dimension_tests, are_flow_error_accepting) {
  auto handled = std::make_shared<bool>(false);
  auto continuation =
      this->supply_exception(supply_test_exception())
          .next(fu2::overload(
              [] {
                // ...
                FAIL();
              },
              [handled](cti::exception_arg_t, cti::exception_t) {
                ASSERT_FALSE(*handled);
                *handled = true;
              }));

  ASSERT_ASYNC_COMPLETION(std::move(continuation));
  ASSERT_TRUE(*handled);
}

TYPED_TEST(single_dimension_tests, are_exceptions_partial_applyable) {
  bool handled = false;
  ASSERT_ASYNC_COMPLETION(
      this->supply_exception(supply_test_exception()).fail([&]() -> void {
        EXPECT_FALSE(handled);
        handled = true;
      }));
  ASSERT_TRUE(handled);

  handled = false;
  ASSERT_ASYNC_INCOMPLETION(this->supply_exception(supply_test_exception())
                                .fail([&]() -> empty_result {
                                  EXPECT_FALSE(handled);
                                  handled = true;
                                  return stop();
                                }));
  ASSERT_TRUE(handled);

  handled = false;
  ASSERT_ASYNC_INCOMPLETION(
      this->supply_exception(supply_test_exception(),
                             detail::identity<int, int>{})
          .fail([&]() -> result<int, int> {
            EXPECT_FALSE(handled);
            handled = true;
            return stop();
          }));

  ASSERT_TRUE(handled);
}
