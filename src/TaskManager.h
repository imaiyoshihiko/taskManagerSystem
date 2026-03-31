#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include "Task.h"
#include <string>
#include <vector>

class TaskManager
{
public:
    struct Snapshot
    {
        std::string path;
        int version;
        std::vector<Task> tasks;
        Snapshot() : path(), version(1), tasks() {}
    };

public:
    TaskManager();
    explicit TaskManager(const std::string& sharedRoot);

    void SetSharedRoot(const std::string& sharedRoot);
    const std::string& GetSharedRoot() const;

    bool LoadLatest(const std::string& projectName);
    bool SaveNextVersion(const std::string& projectName,
                         const std::vector<Task>& tasks,
                         int expectedVersion,
                         int& outSavedVersion,
                         std::string& outError);

    const std::vector<Task>& GetTasks() const;
    std::vector<Task>& GetTasks();
    int GetCurrentVersion() const;
    const Snapshot& GetSnapshot() const;

private:
    std::string JoinPath(const std::string& a, const std::string& b) const;
    bool FileExists(const std::string& path) const;
    bool ReadAllText(const std::string& path, std::string& out) const;
    bool WriteAllText(const std::string& path, const std::string& text) const;
    int ParseVersion(const std::string& filename) const;
    std::vector<int> ListVersions(const std::string& projectDir) const;
    std::string Trim(const std::string& text) const;
    std::string EscapeCsv(const std::string& value) const;
    std::vector<std::string> SplitCsv(const std::string& line) const;
    std::vector<Task> ParseTasks(const std::string& text) const;
    std::string SerializeTasks(const std::vector<Task>& tasks) const;
    int ClampInt(int value, int minValue, int maxValue) const;

private:
    std::string m_sharedRoot;
    Snapshot m_snapshot;
};

#endif
