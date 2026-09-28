#pragma once

#include <QList>
#include <QObject>
#include <QStringList>
#include "util/util.h"

namespace pboman3::model::task {
    using namespace util;

    enum class TaskStatus {
        Success,
        Failure,
        Cancelled
    };

    struct TaskRename {
        QString source;
        QString destination;
    };

    struct TaskResult {
        TaskStatus status = TaskStatus::Success;
        qsizetype completedItems = 0;
        qsizetype failedItems = 0;
        QStringList diagnostics;
        QList<TaskRename> renames;

        [[nodiscard]] static TaskResult success() {
            return {TaskStatus::Success, 1, 0, {}, {}};
        }

        [[nodiscard]] static TaskResult failure(const QString& diagnostic) {
            return {TaskStatus::Failure, 0, 1, {diagnostic}, {}};
        }

        [[nodiscard]] static TaskResult cancelled() {
            return {TaskStatus::Cancelled, 0, 0, {}, {}};
        }

        void append(const TaskResult& other) {
            completedItems += other.completedItems;
            failedItems += other.failedItems;
            diagnostics.append(other.diagnostics);
            renames.append(other.renames);

            if (other.status == TaskStatus::Cancelled)
                status = TaskStatus::Cancelled;
            else if (status != TaskStatus::Cancelled && other.status == TaskStatus::Failure)
                status = TaskStatus::Failure;
        }
    };

    class Task : public QObject {
    Q_OBJECT

    public:
        [[nodiscard]] virtual TaskResult execute(const Cancel& cancel) = 0;

    signals:
        void taskThinking(const QString& text);

        void taskInitialized(const QString& text, qint32 minProgress, qint32 maxProgress);

        void taskProgress(qint32 progress);

        void taskMessage(const QString& message);
    };
}
