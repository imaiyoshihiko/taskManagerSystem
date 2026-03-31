#include "ProjectManager.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>

ProjectManager::ProjectManager() : m_sharedRoot(), m_projects() {}
ProjectManager::ProjectManager(const std::string& sharedRoot) : m_sharedRoot(sharedRoot), m_projects() {}

void ProjectManager::SetSharedRoot(const std::string& sharedRoot) { m_sharedRoot = sharedRoot; }
const std::string& ProjectManager::GetSharedRoot() const { return m_sharedRoot; }
const std::vector<Project>& ProjectManager::GetProjects() const { return m_projects; }

std::string ProjectManager::JoinPath(const std::string& a, const std::string& b) const
{
    if (a.empty()) return b;
    if (a[a.size() - 1] == '\\' || a[a.size() - 1] == '/') return a + b;
    return a + "\\" + b;
}

std::string ProjectManager::Trim(const std::string& text) const
{
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(begin, end - begin);
}

bool ProjectManager::FileExists(const std::string& path) const
{
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool ProjectManager::EnsureDirectory(const std::string& path) const
{
    DWORD attr = GetFileAttributesA(path.c_str());
    if (attr != INVALID_FILE_ATTRIBUTES) return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
    return CreateDirectoryA(path.c_str(), NULL) != 0;
}

bool ProjectManager::ReadAllText(const std::string& path, std::string& out) const
{
    std::ifstream ifs(path.c_str(), std::ios::binary);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

bool ProjectManager::WriteAllText(const std::string& path, const std::string& text) const
{
    std::ofstream ofs(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs << text;
    return ofs.good();
}

bool ProjectManager::EnsureSharedRoot()
{
    return EnsureDirectory(m_sharedRoot);
}

bool ProjectManager::ListDirectories(std::vector<std::string>& outDirectories) const
{
    outDirectories.clear();
    WIN32_FIND_DATAA data;
    HANDLE h = FindFirstFileA((m_sharedRoot + "\\*").c_str(), &data);
    if (h == INVALID_HANDLE_VALUE) return false;

    do
    {
        std::string name = data.cFileName;
        if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && name != "." && name != "..")
        {
            outDirectories.push_back(name);
        }
    } while (FindNextFileA(h, &data));

    FindClose(h);
    std::sort(outDirectories.begin(), outDirectories.end());
    return true;
}

bool ProjectManager::LoadProjectMembers(const std::string& projectName, std::vector<std::string>& outMembers) const
{
    outMembers.clear();
    std::string path = JoinPath(JoinPath(m_sharedRoot, projectName), "projectMemberData.txt");
    if (!FileExists(path)) return true;

    std::string text;
    if (!ReadAllText(path, text)) return false;

    std::istringstream iss(text);
    std::string line;
    std::set<std::string> uniq;
    while (std::getline(iss, line))
    {
        line = Trim(line);
        if (!line.empty()) uniq.insert(line);
    }
    outMembers.assign(uniq.begin(), uniq.end());
    return true;
}

bool ProjectManager::SaveProjectMembers(const std::string& projectName, const std::vector<std::string>& members) const
{
    std::set<std::string> uniq;
    for (size_t i = 0; i < members.size(); ++i)
    {
        std::string name = Trim(members[i]);
        if (!name.empty()) uniq.insert(name);
    }
    std::ostringstream oss;
    for (std::set<std::string>::const_iterator it = uniq.begin(); it != uniq.end(); ++it)
    {
        oss << *it << "\n";
    }
    return WriteAllText(JoinPath(JoinPath(m_sharedRoot, projectName), "projectMemberData.txt"), oss.str());
}

bool ProjectManager::Load()
{
    m_projects.clear();
    std::vector<std::string> names;
    if (!ListDirectories(names)) return false;

    for (size_t i = 0; i < names.size(); ++i)
    {
        Project project(names[i]);
        std::vector<std::string> members;
        LoadProjectMembers(names[i], members);
        project.GetMembers() = members;
        m_projects.push_back(project);
    }
    return true;
}

bool ProjectManager::CreateProject(const std::string& projectName, std::string& error)
{
    if (projectName.empty())
    {
        error = "プロジェクト名を入力してください。";
        return false;
    }

    std::string projectDir = JoinPath(m_sharedRoot, projectName);
    if (!EnsureDirectory(projectDir))
    {
        error = "プロジェクトフォルダの作成に失敗しました。";
        return false;
    }

    std::string membersPath = JoinPath(projectDir, "projectMemberData.txt");
    if (!FileExists(membersPath) && !WriteAllText(membersPath, "admin\n"))
    {
        error = "projectMemberData.txt の作成に失敗しました。";
        return false;
    }

    std::string taskPath = JoinPath(projectDir, "tasks1.txt");
    if (!FileExists(taskPath) && !WriteAllText(taskPath, ""))
    {
        error = "tasks1.txt の作成に失敗しました。";
        return false;
    }

    Load();
    return true;
}

bool ProjectManager::AddMember(const std::string& projectName, const std::string& memberName)
{
    std::vector<std::string> members;
    if (!LoadProjectMembers(projectName, members)) return false;
    if (std::find(members.begin(), members.end(), memberName) == members.end()) members.push_back(memberName);
    bool ok = SaveProjectMembers(projectName, members);
    if (ok) Load();
    return ok;
}

bool ProjectManager::RemoveMember(const std::string& projectName, const std::string& memberName)
{
    std::vector<std::string> members;
    if (!LoadProjectMembers(projectName, members)) return false;
    members.erase(std::remove(members.begin(), members.end(), memberName), members.end());
    bool ok = SaveProjectMembers(projectName, members);
    if (ok) Load();
    return ok;
}

std::vector<std::string> ProjectManager::GetAccessibleProjectNames(const User& user) const
{
    std::vector<std::string> names;
    for (size_t i = 0; i < m_projects.size(); ++i)
    {
        if (user.IsAdmin() || m_projects[i].HasMember(user.GetUsername()))
        {
            names.push_back(m_projects[i].GetName());
        }
    }
    return names;
}

std::vector<std::string> ProjectManager::CollectAllMemberNames() const
{
    std::set<std::string> names;
    for (size_t i = 0; i < m_projects.size(); ++i)
    {
        const std::vector<std::string>& members = m_projects[i].GetMembers();
        for (size_t j = 0; j < members.size(); ++j)
        {
            names.insert(members[j]);
        }
    }
    return std::vector<std::string>(names.begin(), names.end());
}

Project* ProjectManager::FindProject(const std::string& projectName)
{
    for (size_t i = 0; i < m_projects.size(); ++i)
    {
        if (m_projects[i].GetName() == projectName) return &m_projects[i];
    }
    return NULL;
}

const Project* ProjectManager::FindProject(const std::string& projectName) const
{
    for (size_t i = 0; i < m_projects.size(); ++i)
    {
        if (m_projects[i].GetName() == projectName) return &m_projects[i];
    }
    return NULL;
}
