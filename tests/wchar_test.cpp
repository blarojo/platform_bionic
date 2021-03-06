/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <errno.h>
#include <limits.h>
#include <wchar.h>

TEST(wchar, sizeof_wchar_t) {
  EXPECT_EQ(4U, sizeof(wchar_t));
  EXPECT_EQ(4U, sizeof(wint_t));
}

TEST(wchar, mbrlen) {
  char bytes[] = { 'h', 'e', 'l', 'l', 'o', '\0' };
  EXPECT_EQ(0U, mbrlen(&bytes[0], 0, NULL));
  EXPECT_EQ(1U, mbrlen(&bytes[0], 1, NULL));

  EXPECT_EQ(1U, mbrlen(&bytes[4], 1, NULL));
  EXPECT_EQ(0U, mbrlen(&bytes[5], 1, NULL));
}

TEST(wchar, wctomb_wcrtomb) {
  // wctomb and wcrtomb behave differently when s == NULL.
  EXPECT_EQ(0, wctomb(NULL, L'h'));
  EXPECT_EQ(0, wctomb(NULL, L'\0'));
  EXPECT_EQ(1U, wcrtomb(NULL, L'\0', NULL));
  EXPECT_EQ(1U, wcrtomb(NULL, L'h', NULL));

  char bytes[MB_LEN_MAX];

  // wctomb and wcrtomb behave similarly for the null wide character.
  EXPECT_EQ(1, wctomb(bytes, L'\0'));
  EXPECT_EQ(1U, wcrtomb(bytes, L'\0', NULL));

  // ...and for regular characters.
  bytes[0] = 'x';
  EXPECT_EQ(1, wctomb(bytes, L'h'));
  EXPECT_EQ('h', bytes[0]);

  bytes[0] = 'x';
  EXPECT_EQ(1U, wcrtomb(bytes, L'h', NULL));
  EXPECT_EQ('h', bytes[0]);
}

TEST(wchar, wcstombs_wcrtombs) {
  const wchar_t chars[] = { L'h', L'e', L'l', L'l', L'o', 0 };
  const wchar_t bad_chars[] = { L'h', L'i', 666, 0 };
  const wchar_t* src;
  char bytes[BUFSIZ];

  // Given a NULL destination, these functions count valid characters.
  EXPECT_EQ(5U, wcstombs(NULL, chars, 0));
  EXPECT_EQ(5U, wcstombs(NULL, chars, 4));
  EXPECT_EQ(5U, wcstombs(NULL, chars, 256));
  src = chars;
  EXPECT_EQ(5U, wcsrtombs(NULL, &src, 0, NULL));
  EXPECT_EQ(&chars[0], src);
  src = chars;
  EXPECT_EQ(5U, wcsrtombs(NULL, &src, 4, NULL));
  EXPECT_EQ(&chars[0], src);
  src = chars;
  EXPECT_EQ(5U, wcsrtombs(NULL, &src, 256, NULL));
  EXPECT_EQ(&chars[0], src);

  // An unrepresentable char just returns an error from wcstombs...
  errno = 0;
  EXPECT_EQ(static_cast<size_t>(-1), wcstombs(NULL, bad_chars, 0));
  EXPECT_EQ(EILSEQ, errno);
  errno = 0;
  EXPECT_EQ(static_cast<size_t>(-1), wcstombs(NULL, bad_chars, 256));
  EXPECT_EQ(EILSEQ, errno);

  // And wcsrtombs doesn't tell us where it got stuck because we didn't ask it
  // to actually convert anything...
  errno = 0;
  src = bad_chars;
  EXPECT_EQ(static_cast<size_t>(-1), wcsrtombs(NULL, &src, 0, NULL));
  EXPECT_EQ(&bad_chars[0], src);
  EXPECT_EQ(EILSEQ, errno);
  errno = 0;
  src = bad_chars;
  EXPECT_EQ(static_cast<size_t>(-1), wcsrtombs(NULL, &src, 256, NULL));
  EXPECT_EQ(&bad_chars[0], src);
  EXPECT_EQ(EILSEQ, errno);

  // Okay, now let's test actually converting something...
  memset(bytes, 'x', sizeof(bytes));
  EXPECT_EQ(0U, wcstombs(bytes, chars, 0));
  memset(bytes, 'x', sizeof(bytes));
  EXPECT_EQ(4U, wcstombs(bytes, chars, 4));
  bytes[5] = 0;
  EXPECT_STREQ("hellx", bytes);
  memset(bytes, 'x', sizeof(bytes));
  EXPECT_EQ(5U, wcstombs(bytes, chars, 256));
  EXPECT_STREQ("hello", bytes);
  memset(bytes, 'x', sizeof(bytes));
  EXPECT_EQ(5U, wcstombs(bytes, chars, 6));
  EXPECT_STREQ("hello", bytes);
  errno = 0;
  memset(bytes, 'x', sizeof(bytes));
  EXPECT_EQ(static_cast<size_t>(-1), wcstombs(bytes, bad_chars, 256));
  EXPECT_EQ(EILSEQ, errno);
  bytes[3] = 0;
  EXPECT_STREQ("hix", bytes);

  // wcsrtombs is a bit more informative...
  memset(bytes, 'x', sizeof(bytes));
  src = chars;
  EXPECT_EQ(0U, wcsrtombs(bytes, &src, 0, NULL));
  EXPECT_EQ(&chars[0], src); // No input consumed.
  EXPECT_EQ(EILSEQ, errno);

  memset(bytes, 'x', sizeof(bytes));
  src = chars;
  EXPECT_EQ(4U, wcsrtombs(bytes, &src, 4, NULL));
  EXPECT_EQ(&chars[4], src); // Some input consumed.
  EXPECT_EQ(EILSEQ, errno);
  bytes[5] = 0;
  EXPECT_STREQ("hellx", bytes);

  memset(bytes, 'x', sizeof(bytes));
  src = chars;
  EXPECT_EQ(5U, wcsrtombs(bytes, &src, 256, NULL));
  EXPECT_EQ(NULL, src); // All input consumed!
  EXPECT_EQ(EILSEQ, errno);
  EXPECT_STREQ("hello", bytes);

  memset(bytes, 'x', sizeof(bytes));
  src = chars;
  EXPECT_EQ(5U, wcsrtombs(bytes, &src, 6, NULL));
  EXPECT_EQ(NULL, src); // All input consumed.
  EXPECT_EQ(EILSEQ, errno);
  EXPECT_STREQ("hello", bytes);

  memset(bytes, 'x', sizeof(bytes));
  src = bad_chars;
  EXPECT_EQ(static_cast<size_t>(-1), wcsrtombs(bytes, &src, 256, NULL));
  EXPECT_EQ(&bad_chars[2], src);
  EXPECT_EQ(EILSEQ, errno);
  bytes[3] = 0;
  EXPECT_STREQ("hix", bytes);
}

TEST(wchar, limits) {
  ASSERT_LT(WCHAR_MIN, WCHAR_MAX);
}
