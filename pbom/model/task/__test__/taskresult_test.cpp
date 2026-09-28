#include <gtest/gtest.h>
#include "model/task/task.h"

namespace pboman3::model::task {
    TEST(TaskResultTest, SuccessContainsCompletionCount) {
        const TaskResult result = TaskResult::success();

        EXPECT_EQ(TaskStatus::Success, result.status);
        EXPECT_EQ(1, result.completedItems);
        EXPECT_EQ(0, result.failedItems);
        EXPECT_TRUE(result.diagnostics.isEmpty());
    }

    TEST(TaskResultTest, FailureContainsDiagnosticAndFailureCount) {
        const TaskResult result = TaskResult::failure("failed");

        EXPECT_EQ(TaskStatus::Failure, result.status);
        EXPECT_EQ(0, result.completedItems);
        EXPECT_EQ(1, result.failedItems);
        ASSERT_EQ(1, result.diagnostics.size());
        EXPECT_EQ("failed", result.diagnostics.first());
    }

    TEST(TaskResultTest, CancellationHasNoCompletedOrFailedItems) {
        const TaskResult result = TaskResult::cancelled();

        EXPECT_EQ(TaskStatus::Cancelled, result.status);
        EXPECT_EQ(0, result.completedItems);
        EXPECT_EQ(0, result.failedItems);
    }

    TEST(TaskResultTest, AppendAggregatesCountsAndKeepsTheMostSevereStatus) {
        TaskResult result;
        result.append(TaskResult::success());
        result.append(TaskResult::failure("failed"));
        result.append(TaskResult::cancelled());

        EXPECT_EQ(TaskStatus::Cancelled, result.status);
        EXPECT_EQ(1, result.completedItems);
        EXPECT_EQ(1, result.failedItems);
        ASSERT_EQ(1, result.diagnostics.size());
        EXPECT_EQ("failed", result.diagnostics.first());
    }
}
