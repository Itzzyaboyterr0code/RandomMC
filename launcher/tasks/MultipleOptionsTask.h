#pragma once

#include "ConcurrentTask.h"

/* This task type will attempt to do run each of it's subtasks in sequence,
 * until one of them succeeds. When that happens, the remaining tasks will not run.
 * */
class MultipleOptionsTask : public ConcurrentTask {
    Q_OBJECT
   public:
    explicit MultipleOptionsTask(QObject* parent = nullptr, const QString& task_name = "");
    ~MultipleOptionsTask() override = default;

   private slots:
    void startNext() override;
    void updateState() override;
};
