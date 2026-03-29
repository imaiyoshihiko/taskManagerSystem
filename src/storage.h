#ifndef TASK_MANAGER_STORAGE_H
#define TASK_MANAGER_STORAGE_H

#include "models.h"
#include <string>
#include <vector>

struct TaskFileBundle
{
    std::string path;
    int version = 0;
    std::vector<TaskItem> tasks;
};

namespace Storage
{
    bool FileExists(const std::string& path);
    bool EnsureDirectory(const std::string& path);
    bool WriteAllText(const std::string& path, const std::string& text);
    bool ReadAllText(const std::string& path, std::string& out);
    std::string ExeDirectory();
    std::string SharedPathConfigPath();
    std::string UserDataPath(const std::string& sharedRoot);
    std::string ProjectDirectory(const std::string& sharedRoot, const std::string& projectName);
    std::string ProjectMemberDataPath(const std::string& sharedRoot, const std::string& projectName);

    bool SaveSharedFolderPath(const std::string& path);
    bool LoadSharedFolderPath(std::string& outPath);

    bool LoadUsers(const std::string& sharedRoot, std::vector<UserAccount>& users);
    bool AppendUser(const std::string& sharedRoot, const UserAccount& user);
    bool UserExists(const std::vector<UserAccount>& users, const std::string& username);

    bool CreateProject(const std::string& sharedRoot, const std::string& projectName, std::string& error);
    bool LoadProjects(const std::string& sharedRoot, std::vector<ProjectInfo>& projects);
    bool LoadProjectMembers(const std::string& sharedRoot, const std::string& projectName, std::vector<std::string>& members);
    bool SaveProjectMembers(const std::string& sharedRoot, const std::string& projectName, const std::vector<std::string>& members);

    std::vector<std::string> AccessibleProjects(const std::string& sharedRoot, const std::string& username, const std::string& role);
    std::vector<std::string> CollectAllMemberNames(const std::string& sharedRoot);

    bool LoadLatestTasks(const std::string& sharedRoot, const std::string& projectName, TaskFileBundle& bundle);
    bool SaveTasksAsNextVersion(const std::string& sharedRoot,
                                const std::string& projectName,
                                const std::vector<TaskItem>& tasks,
                                int expectedVersion,
                                int& savedVersion,
                                std::string& error);

    std::string NowString();
}

#endif
