#include "storage.h"

#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <set>
#include <ctime>

namespace
{
    std::string Trim(const std::string& s)
    {
        size_t begin = 0;
        while (begin < s.size() && std::isspace(static_cast<unsigned char>(s[begin])))
        {
            ++begin;
        }

        size_t end = s.size();
        while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1])))
        {
            --end;
        }
        return s.substr(begin, end - begin);
    }

    std::vector<std::string> SplitCsvLine(const std::string& line)
    {
        std::vector<std::string> fields;
        std::string cur;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); ++i)
        {
            char c = line[i];
            if (c == '"')
            {
                if (inQuotes && i + 1 < line.size() && line[i + 1] == '"')
                {
                    cur.push_back('"');
                    ++i;
                }
                else
                {
                    inQuotes = !inQuotes;
                }
            }
            else if (c == ',' && !inQuotes)
            {
                fields.push_back(cur);
                cur.clear();
            }
            else
            {
                cur.push_back(c);
            }
        }
        fields.push_back(cur);
        return fields;
    }

    std::string EscapeCsv(const std::string& s)
    {
        bool needQuotes = false;
        std::string out;
        for (size_t i = 0; i < s.size(); ++i)
        {
            const char c = s[i];
            if (c == ',' || c == '"' || c == '\n' || c == '\r')
            {
                needQuotes = true;
            }
            if (c == '"')
            {
                out += "\"\"";
            }
            else
            {
                out.push_back(c);
            }
        }
        return needQuotes ? ("\"" + out + "\"") : out;
    }

    std::string JoinPath(const std::string& a, const std::string& b)
    {
        if (a.empty()) return b;
        if (a.back() == '\\' || a.back() == '/') return a + b;
        return a + "\\" + b;
    }

    bool IsDotName(const std::string& name)
    {
        return name == "." || name == "..";
    }

    int ParseTaskVersion(const std::string& filename)
    {
        if (filename.size() < 10) return -1;
        if (filename.rfind("tasks", 0) != 0) return -1;
        if (filename.substr(filename.size() - 4) != ".txt") return -1;
        std::string num = filename.substr(5, filename.size() - 9);
        if (num.empty()) return -1;
        for (size_t i = 0; i < num.size(); ++i)
        {
            if (!std::isdigit(static_cast<unsigned char>(num[i]))) return -1;
        }
        return std::atoi(num.c_str());
    }

    std::vector<TaskItem> ParseTasks(const std::string& text)
    {
        std::vector<TaskItem> tasks;
        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line))
        {
            line = Trim(line);
            if (line.empty()) continue;
            std::vector<std::string> f = SplitCsvLine(line);
            if (f.size() < 12) continue;

            TaskItem t;
            t.id = std::atoi(f[0].c_str());
            t.name = f[1];
            t.category = f[2];
            t.assignee = f[3];
            t.effort = f[4];
            t.startDate = f[5];
            t.endDate = f[6];
            t.progress = std::atoi(f[7].c_str());
            t.priority = f[8];
            t.status = f[9];
            t.creator = f[10];
            t.comment = f[11];
            tasks.push_back(t);
        }
        return tasks;
    }

    std::string SerializeTasks(const std::vector<TaskItem>& tasks)
    {
        std::ostringstream oss;
        for (size_t i = 0; i < tasks.size(); ++i)
        {
            const TaskItem& t = tasks[i];
            oss << t.id << ','
                << EscapeCsv(t.name) << ','
                << EscapeCsv(t.category) << ','
                << EscapeCsv(t.assignee) << ','
                << EscapeCsv(t.effort) << ','
                << EscapeCsv(t.startDate) << ','
                << EscapeCsv(t.endDate) << ','
                << t.progress << ','
                << EscapeCsv(t.priority) << ','
                << EscapeCsv(t.status) << ','
                << EscapeCsv(t.creator) << ','
                << EscapeCsv(t.comment);
            if (i + 1 < tasks.size()) oss << "\n";
        }
        return oss.str();
    }

    bool ListDirectories(const std::string& root, std::vector<std::string>& names)
    {
        WIN32_FIND_DATAA data;
        HANDLE h = FindFirstFileA((root + "\\*").c_str(), &data);
        if (h == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        do
        {
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !IsDotName(data.cFileName))
            {
                names.push_back(data.cFileName);
            }
        }
        while (FindNextFileA(h, &data));

        FindClose(h);
        std::sort(names.begin(), names.end());
        return true;
    }

    std::vector<int> ListTaskVersions(const std::string& projectDir)
    {
        std::vector<int> versions;
        WIN32_FIND_DATAA data;
        HANDLE h = FindFirstFileA((projectDir + "\\tasks*.txt").c_str(), &data);
        if (h == INVALID_HANDLE_VALUE)
        {
            return versions;
        }

        do
        {
            int v = ParseTaskVersion(data.cFileName);
            if (v >= 1) versions.push_back(v);
        }
        while (FindNextFileA(h, &data));
        FindClose(h);
        std::sort(versions.begin(), versions.end());
        return versions;
    }
}

namespace Storage
{
    bool FileExists(const std::string& path)
    {
        DWORD attr = GetFileAttributesA(path.c_str());
        return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool EnsureDirectory(const std::string& path)
    {
        DWORD attr = GetFileAttributesA(path.c_str());
        if (attr != INVALID_FILE_ATTRIBUTES)
        {
            return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
        }
        return CreateDirectoryA(path.c_str(), nullptr) != 0;
    }

    bool WriteAllText(const std::string& path, const std::string& text)
    {
        std::ofstream ofs(path.c_str(), std::ios::binary | std::ios::trunc);
        if (!ofs) return false;
        ofs << text;
        return ofs.good();
    }

    bool ReadAllText(const std::string& path, std::string& out)
    {
        std::ifstream ifs(path.c_str(), std::ios::binary);
        if (!ifs) return false;
        std::ostringstream ss;
        ss << ifs.rdbuf();
        out = ss.str();
        return true;
    }

    std::string ExeDirectory()
    {
        char buffer[MAX_PATH] = {0};
        GetModuleFileNameA(nullptr, buffer, MAX_PATH);
        std::string full = buffer;
        size_t pos = full.find_last_of("\\/");
        return (pos == std::string::npos) ? "." : full.substr(0, pos);
    }

    std::string SharedPathConfigPath()
    {
        return JoinPath(ExeDirectory(), "sharedFolderPath.txt");
    }

    std::string UserDataPath(const std::string& sharedRoot)
    {
        return JoinPath(sharedRoot, "userData.txt");
    }

    std::string ProjectDirectory(const std::string& sharedRoot, const std::string& projectName)
    {
        return JoinPath(sharedRoot, projectName);
    }

    std::string ProjectMemberDataPath(const std::string& sharedRoot, const std::string& projectName)
    {
        return JoinPath(ProjectDirectory(sharedRoot, projectName), "projectMemberData.txt");
    }

    bool SaveSharedFolderPath(const std::string& path)
    {
        return WriteAllText(SharedPathConfigPath(), path);
    }

    bool LoadSharedFolderPath(std::string& outPath)
    {
        std::string text;
        if (!ReadAllText(SharedPathConfigPath(), text)) return false;
        outPath = Trim(text);
        return !outPath.empty();
    }

    bool LoadUsers(const std::string& sharedRoot, std::vector<UserAccount>& users)
    {
        users.clear();
        std::string path = UserDataPath(sharedRoot);
        if (!FileExists(path)) return true;

        std::string text;
        if (!ReadAllText(path, text)) return false;

        std::istringstream iss(text);
        std::string line;
        while (std::getline(iss, line))
        {
            line = Trim(line);
            if (line.empty()) continue;
            std::vector<std::string> f = SplitCsvLine(line);
            if (f.size() < 3) continue;
            UserAccount u;
            u.username = f[0];
            u.password = f[1];
            u.role = f[2];
            users.push_back(u);
        }
        return true;
    }

    bool AppendUser(const std::string& sharedRoot, const UserAccount& user)
    {
        std::ofstream ofs(UserDataPath(sharedRoot).c_str(), std::ios::app | std::ios::binary);
        if (!ofs) return false;
        ofs << EscapeCsv(user.username) << ','
            << EscapeCsv(user.password) << ','
            << EscapeCsv(user.role) << "\n";
        return ofs.good();
    }

    bool UserExists(const std::vector<UserAccount>& users, const std::string& username)
    {
        for (size_t i = 0; i < users.size(); ++i)
        {
            if (users[i].username == username) return true;
        }
        return false;
    }

    bool CreateProject(const std::string& sharedRoot, const std::string& projectName, std::string& error)
    {
        std::string dir = ProjectDirectory(sharedRoot, projectName);
        if (!EnsureDirectory(dir))
        {
            error = "プロジェクトフォルダの作成に失敗しました。";
            return false;
        }

        std::string memberPath = ProjectMemberDataPath(sharedRoot, projectName);
        if (!FileExists(memberPath) && !WriteAllText(memberPath, "admin\n"))
        {
            error = "projectMemberData.txt の作成に失敗しました。";
            return false;
        }

        if (!FileExists(JoinPath(dir, "tasks1.txt")) && !WriteAllText(JoinPath(dir, "tasks1.txt"), ""))
        {
            error = "tasks1.txt の作成に失敗しました。";
            return false;
        }
        return true;
    }

    bool LoadProjects(const std::string& sharedRoot, std::vector<ProjectInfo>& projects)
    {
        projects.clear();
        std::vector<std::string> dirs;
        if (!ListDirectories(sharedRoot, dirs)) return false;

        for (size_t i = 0; i < dirs.size(); ++i)
        {
            ProjectInfo p;
            p.name = dirs[i];
            LoadProjectMembers(sharedRoot, p.name, p.members);
            projects.push_back(p);
        }
        return true;
    }

    bool LoadProjectMembers(const std::string& sharedRoot, const std::string& projectName, std::vector<std::string>& members)
    {
        members.clear();
        std::string path = ProjectMemberDataPath(sharedRoot, projectName);
        if (!FileExists(path)) return true;

        std::string text;
        if (!ReadAllText(path, text)) return false;

        std::istringstream iss(text);
        std::string line;
        std::set<std::string> uniq;
        while (std::getline(iss, line))
        {
            std::string name = Trim(line);
            if (!name.empty()) uniq.insert(name);
        }
        members.assign(uniq.begin(), uniq.end());
        return true;
    }

    bool SaveProjectMembers(const std::string& sharedRoot, const std::string& projectName, const std::vector<std::string>& members)
    {
        std::set<std::string> uniq;
        for (size_t i = 0; i < members.size(); ++i)
        {
            std::string v = Trim(members[i]);
            if (!v.empty()) uniq.insert(v);
        }

        std::ostringstream oss;
        for (std::set<std::string>::const_iterator it = uniq.begin(); it != uniq.end(); ++it)
        {
            oss << *it << "\n";
        }
        return WriteAllText(ProjectMemberDataPath(sharedRoot, projectName), oss.str());
    }

    std::vector<std::string> AccessibleProjects(const std::string& sharedRoot, const std::string& username, const std::string& role)
    {
        std::vector<std::string> result;
        std::vector<ProjectInfo> projects;
        if (!LoadProjects(sharedRoot, projects)) return result;

        for (size_t i = 0; i < projects.size(); ++i)
        {
            if (role == "admin")
            {
                result.push_back(projects[i].name);
                continue;
            }
            for (size_t j = 0; j < projects[i].members.size(); ++j)
            {
                if (projects[i].members[j] == username)
                {
                    result.push_back(projects[i].name);
                    break;
                }
            }
        }
        return result;
    }

    std::vector<std::string> CollectAllMemberNames(const std::string& sharedRoot)
    {
        std::set<std::string> names;
        std::vector<ProjectInfo> projects;
        if (!LoadProjects(sharedRoot, projects)) return std::vector<std::string>();
        for (size_t i = 0; i < projects.size(); ++i)
        {
            for (size_t j = 0; j < projects[i].members.size(); ++j)
            {
                names.insert(projects[i].members[j]);
            }
        }
        return std::vector<std::string>(names.begin(), names.end());
    }

    bool LoadLatestTasks(const std::string& sharedRoot, const std::string& projectName, TaskFileBundle& bundle)
    {
        bundle = TaskFileBundle();
        std::string dir = ProjectDirectory(sharedRoot, projectName);
        std::vector<int> versions = ListTaskVersions(dir);
        if (versions.empty())
        {
            bundle.path = JoinPath(dir, "tasks1.txt");
            bundle.version = 1;
            bundle.tasks.clear();
            if (!FileExists(bundle.path)) WriteAllText(bundle.path, "");
            return true;
        }

        int latest = versions.back();
        bundle.version = latest;
        bundle.path = JoinPath(dir, "tasks" + std::to_string(latest) + ".txt");
        std::string text;
        if (!ReadAllText(bundle.path, text)) return false;
        bundle.tasks = ParseTasks(text);
        return true;
    }

    bool SaveTasksAsNextVersion(const std::string& sharedRoot,
                                const std::string& projectName,
                                const std::vector<TaskItem>& tasks,
                                int expectedVersion,
                                int& savedVersion,
                                std::string& error)
    {
        TaskFileBundle latest;
        if (!LoadLatestTasks(sharedRoot, projectName, latest))
        {
            error = "最新タスクファイルの読み込みに失敗しました。";
            return false;
        }

        if (latest.version != expectedVersion)
        {
            error = "最新ではないファイルを編集しています。最新のものを読み込みなおしてください。";
            return false;
        }

        savedVersion = latest.version + 1;
        std::string dir = ProjectDirectory(sharedRoot, projectName);
        std::string path = JoinPath(dir, "tasks" + std::to_string(savedVersion) + ".txt");
        if (!WriteAllText(path, SerializeTasks(tasks)))
        {
            error = "新しいタスクファイルの保存に失敗しました。";
            return false;
        }

        std::vector<int> versions = ListTaskVersions(dir);
        while (versions.size() > 100)
        {
            std::string oldPath = JoinPath(dir, "tasks" + std::to_string(versions.front()) + ".txt");
            DeleteFileA(oldPath.c_str());
            versions.erase(versions.begin());
        }
        return true;
    }

    std::string NowString()
    {
        std::time_t t = std::time(nullptr);
        std::tm* tmv = std::localtime(&t);
        char buf[64] = {0};
        std::snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
                      tmv->tm_year + 1900,
                      tmv->tm_mon + 1,
                      tmv->tm_mday,
                      tmv->tm_hour,
                      tmv->tm_min,
                      tmv->tm_sec);
        return buf;
    }
}
