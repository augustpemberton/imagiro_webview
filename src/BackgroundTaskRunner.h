//
// Created by August Pemberton on 17/05/2024.
//

#pragma once
#include <utility>

namespace imagiro {
    class BackgroundTaskRunner : juce::Thread {
    public:

        struct Task {
            std::function<choc::value::Value()> fn;
            int id = -1;
        };

        BackgroundTaskRunner(WebViewManager &webviewManager)
                : juce::Thread("Background Task Runner"),
                  wvm(webviewManager) {
            startThread();
        }

        ~BackgroundTaskRunner() override {
            stopThread(500);
        }

        void clearTasks() {
            clearTasksFlag = true;
        }

        void run() override {
            while (!threadShouldExit()) {
                while (tasks.try_dequeue(temp)) {
                    if (threadShouldExit()) return;

                    // if just cleared, ignore the rest of the task
                    if (clearTasksFlag) continue;

                    processTask(temp);
                }

                clearTasksFlag = false;
                juce::Thread::wait(-1);
            }
        }

        void processTask(const Task &task) {
            auto result = task.fn();
            wvm.evaluateJavascript("window.ui.onBackgroundTaskFinished(" +
                                   std::to_string(task.id) + ", " + choc::json::toString(result) + ")");
        }

        choc::value::Value queueTask(Task task) {
            task.id = tasksQueued++;
            tasks.enqueue(task);
            notify();
            return choc::value::Value(task.id);
        }

    private:
        WebViewManager &wvm;
        Task temp;
        moodycamel::ReaderWriterQueue<Task> tasks{512};

        int tasksQueued = 0;
        std::atomic<bool> clearTasksFlag{false};
    };
}