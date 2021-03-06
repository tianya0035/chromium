// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/history_ui.h"

#include "base/strings/utf_string_conversions.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

struct TestResult {
  std::string url;
  int64 hour_offset;  // Visit time in hours past the baseline time.
};

// Duplicates on the same day in the local timezone are removed, so set a
// baseline time in local time.
const base::Time baseline_time = base::Time::UnixEpoch().LocalMidnight();

// For each item in |results|, create a new Value representing the visit, and
// insert it into |list_value|.
void AddQueryResults(
    TestResult* test_results,
    int test_results_size,
    std::vector<BrowsingHistoryHandler::HistoryEntry>* results) {
  for (int i = 0; i < test_results_size; ++i) {
    BrowsingHistoryHandler::HistoryEntry entry;
    entry.time = baseline_time +
                 base::TimeDelta::FromHours(test_results[i].hour_offset);
    entry.url = GURL(test_results[i].url);
    entry.all_timestamps.insert(entry.time.ToInternalValue());
    results->push_back(entry);
  }
}

// Returns true if |result| matches the test data given by |correct_result|,
// otherwise returns false.
bool ResultEquals(
    const BrowsingHistoryHandler::HistoryEntry& result,
    const TestResult& correct_result) {
  base::Time correct_time =
      baseline_time + base::TimeDelta::FromHours(correct_result.hour_offset);

  return result.time == correct_time && result.url == GURL(correct_result.url);
}

}  // namespace

// Tests that the MergeDuplicateResults method correctly removes duplicate
// visits to the same URL on the same day.
TEST(HistoryUITest, MergeDuplicateResults) {
  {
    // Basic test that duplicates on the same day are removed.
    TestResult test_data[] = {
      { "http://google.com", 0 },
      { "http://google.de", 1 },
      { "http://google.com", 2 },
      { "http://google.com", 3 }  // Most recent.
    };
    std::vector<BrowsingHistoryHandler::HistoryEntry> results;
    AddQueryResults(test_data, arraysize(test_data), &results);
    BrowsingHistoryHandler::MergeDuplicateResults(&results);

    ASSERT_EQ(2U, results.size());
    EXPECT_TRUE(ResultEquals(results[0], test_data[3]));
    EXPECT_TRUE(ResultEquals(results[1], test_data[1]));
  }

  {
    // Test that a duplicate URL on the next day is not removed.
    TestResult test_data[] = {
      { "http://google.com", 0 },
      { "http://google.com", 23 },
      { "http://google.com", 24 },  // Most recent.
    };
    std::vector<BrowsingHistoryHandler::HistoryEntry> results;
    AddQueryResults(test_data, arraysize(test_data), &results);
    BrowsingHistoryHandler::MergeDuplicateResults(&results);

    ASSERT_EQ(2U, results.size());
    EXPECT_TRUE(ResultEquals(results[0], test_data[2]));
    EXPECT_TRUE(ResultEquals(results[1], test_data[1]));
  }

  {
    // Test multiple duplicates across multiple days.
    TestResult test_data[] = {
      // First day.
      { "http://google.de", 0 },
      { "http://google.com", 1 },
      { "http://google.de", 2 },
      { "http://google.com", 3 },

      // Second day.
      { "http://google.de", 24 },
      { "http://google.com", 25 },
      { "http://google.de", 26 },
      { "http://google.com", 27 },  // Most recent.
    };
    std::vector<BrowsingHistoryHandler::HistoryEntry> results;
    AddQueryResults(test_data, arraysize(test_data), &results);
    BrowsingHistoryHandler::MergeDuplicateResults(&results);

    ASSERT_EQ(4U, results.size());
    EXPECT_TRUE(ResultEquals(results[0], test_data[7]));
    EXPECT_TRUE(ResultEquals(results[1], test_data[6]));
    EXPECT_TRUE(ResultEquals(results[2], test_data[3]));
    EXPECT_TRUE(ResultEquals(results[3], test_data[2]));
  }

  {
    // Test that timestamps for duplicates are properly saved.
    TestResult test_data[] = {
      { "http://google.com", 0 },
      { "http://google.de", 1 },
      { "http://google.com", 2 },
      { "http://google.com", 3 }  // Most recent.
    };
    std::vector<BrowsingHistoryHandler::HistoryEntry> results;
    AddQueryResults(test_data, arraysize(test_data), &results);
    BrowsingHistoryHandler::MergeDuplicateResults(&results);

    ASSERT_EQ(2U, results.size());
    EXPECT_TRUE(ResultEquals(results[0], test_data[3]));
    EXPECT_TRUE(ResultEquals(results[1], test_data[1]));
    EXPECT_EQ(3u, results[0].all_timestamps.size());
    EXPECT_EQ(1u, results[1].all_timestamps.size());
  }
}
