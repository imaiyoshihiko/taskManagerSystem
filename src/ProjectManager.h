#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include "Project.h"
#include "User.h"
#include <string>
#include <vector>

class ProjectManager
{
public:
    ProjectManager();
    explicit ProjectManager(const std::string& sharedRoot);

    void SetSharedRoot(const std::string& sharedRoot);
    const std::string& GetSharedRoot() const;

    bool EnsureSharedRoot();
    bool Load();
    bool CreateProject(const std::string& projectName, std::string& error);
    bool AddMember(const std::string& projectName, const std::string& memberName);
    bool RemoveMember(const std::string& projectName, const std::string& memberName);
    bool LoadProjectMembers(const std::string& projectName, std::vector<std::string>& outMembers) const;
    std::vector<std::string> GetAccessibleProjectNames(const User& user) const;
    std::vector<std::string> CollectAllMemberNames() const;
    Project* FindProject(const std::string& projectName);
    const Project* FindProject(const std::string& projectName) const;
    const std::vector<Project>& GetProjects() const;

private:
    std::string JoinPath(const std::string& a, const std::string& b) const;
    std::string Trim(const std::string& text) const;
    bool EnsureDirectory(const std::string& path) const;
    bool FileExists(const std::string& path) const;
    bool ReadAllText(const std::string& path, std::string& out) const;
    bool WriteAllText(const std::string& path, const std::string& text) const;
    bool ListDirectories(std::vector<std::string>& outDirectories) const;
    bool SaveProjectMembers(const std::string& projectName, const std::vector<std::string>& members) const;

private:
    std::string m_sharedRoot;
    std::vector<Project> m_projects;
};

#endif
