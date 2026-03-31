#include "TaskManager.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

TaskManager::TaskManager() : m_sharedRoot(), m_snapshot() {}
TaskManager::TaskManager(const std::string& sharedRoot) : m_sharedRoot(sharedRoot), m_snapshot() {}

void TaskManager::SetSharedRoot(const std::string& sharedRoot) { m_sharedRoot = sharedRoot; }
const std::string& TaskManager::GetSharedRoot() const { return m_sharedRoot; }
const std::vector<Task>& TaskManager::GetTasks() const { return m_snapshot.tasks; }
std::vector<Task>& TaskManager::GetTasks() { return m_snapshot.tasks; }
int TaskManager::GetCurrentVersion() const { return m_snapshot.version; }
const TaskManager::Snapshot& TaskManager::GetSnapshot() const { return m_snapshot; }

std::string TaskManager::JoinPath(const std::string& a, const std::string& b) const
{
    if (a.empty()) return b;
    if (a[a.size() - 1] == '\\' || a[a.size() - 1] == '/') return a + b;
    return a + "\\" + b;
}

bool TaskManager::FileExists(const std::string& path) const
{
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool TaskManager::ReadAllText(const std::string& path, std::string& out) const
{
    std::ifstream ifs(path.c_str(), std::ios::binary);
    if (!ifs) return false;
    std::ostringstream ss;
    ss << ifs.rdbuf();
    out = ss.str();
    return true;
}

bool TaskManager::WriteAllText(const std::string& path, const std::string& text) const
{
    std::ofstream ofs(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs << text;
    return ofs.good();
}

int TaskManager::ParseVersion(const std::string& filename) const
{
    if (filename.size() < 10) return -1;
    if (filename.substr(0, 5) != "tasks") return -1;
    if (filename.substr(filename.size() - 4) != ".txt") return -1;
    std::string num = filename.substr(5, filename.size() - 9);
    if (num.empty()) return -1;
    for (size_t i = 0; i < num.size(); ++i)
    {
        if (!std::isdigit(static_cast<unsigned char>(num[i]))) return -1;
    }
    return std::atoi(num.c_str());
}

std::vector<int> TaskManager::ListVersions(const std::string& projectDir) const
{
    std::vector<int> versions;
    WIN32_FIND_DATAA data;
    HANDLE h = FindFirstFileA((projectDir + "\\tasks*.txt").c_str(), &data);
    if (h == INVALID_HANDLE_VALUE) return versions;

    do
    {
        int version = ParseVersion(data.cFileName);
        if (version >= 1) versions.push_back(version);
    } while (FindNextFileA(h, &data));
    FindClose(h);

    std::sort(versions.begin(), versions.end());
    return versions;
}

std::string TaskManager::Trim(const std::string& text) const
{
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(begin, end - begin);
}

std::string TaskManager::EscapeCsv(const std::string& value) const
{
    bool needQuotes = false;
    std::string out;
    for (size_t i = 0; i < value.size(); ++i)
    {
        char c = value[i];
        if (c == ',' || c == '"' || c == '\n' || c == '\r') needQuotes = true;
        if (c == '"') out += "\"\"";
        else out.push_back(c);
    }
    return needQuotes ? ("\"" + out + "\"") : out;
}

std::vector<std::string> TaskManager::SplitCsv(const std::string& line) const
{
    std::vector<std::string> fields;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.size(); ++i)
    {
        char c = line[i];
        if (c == '"')
        {
            if (inQuotes && i + 1 < line.size() && line[i + 1] == '"')
            {
                current.push_back('"');
                ++i;
            }
            else
            {
                inQuotes = !inQuotes;
            }
        }
        else if (c == ',' && !inQuotes)
        {
            fields.push_back(current);
            current.clear();
        }
        else
        {
            current.push_back(c);
        }
    }
    fields.push_back(current);
    return fields;
}

std::vector<Task> TaskManager::ParseTasks(const std::string& text) const
{
    std::vector<Task> tasks;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line))
    {
        line = Trim(line);
        if (line.empty()) continue;
        std::vector<std::string> fields = SplitCsv(line);
        if (fields.size() < 12) continue;
        Task task;
        task.SetId(std::atoi(fields[0].c_str()));
        task.SetName(fields[1]);
        task.SetCategory(fields[2]);
        task.SetAssignee(fields[3]);
        task.SetEffort(fields[4]);
        task.SetStartDate(fields[5]);
        task.SetEndDate(fields[6]);
        task.SetProgress(std::atoi(fields[7].c_str()));
        task.SetPriority(fields[8]);
        task.SetStatus(fields[9]);
        task.SetCreator(fields[10]);
        task.SetComment(fields[11]);
        tasks.push_back(task);
    }
    return tasks;
}

std::string TaskManager::SerializeTasks(const std::vector<Task>& tasks) const
{
    std::ostringstream oss;
    for (size_t i = 0; i < tasks.size(); ++i)
    {
        const Task& task = tasks[i];
        oss << task.GetId() << ','
            << EscapeCsv(task.GetName()) << ','
            << EscapeCsv(task.GetCategory()) << ','
            << EscapeCsv(task.GetAssignee()) << ','
            << EscapeCsv(task.GetEffort()) << ','
            << EscapeCsv(task.GetStartDate()) << ','
            << EscapeCsv(task.GetEndDate()) << ','
            << task.GetProgress() << ','
            << EscapeCsv(task.GetPriority()) << ','
            << EscapeCsv(task.GetStatus()) << ','
            << EscapeCsv(task.GetCreator()) << ','
            << EscapeCsv(task.GetComment());
        if (i + 1 < tasks.size()) oss << "\n";
    }
    return oss.str();
}

int TaskManager::ClampInt(int value, int minValue, int maxValue) const
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

bool TaskManager::LoadLatest(const std::string& projectName)
{
    std::string projectDir = JoinPath(m_sharedRoot, projectName);
    std::vector<int> versions = ListVersions(projectDir);
    if (versions.empty())
    {
        m_snapshot = Snapshot();
        m_snapshot.version = 1;
        m_snapshot.path = JoinPath(projectDir, "tasks1.txt");
        m_snapshot.tasks.clear();
        if (!FileExists(m_snapshot.path)) WriteAllText(m_snapshot.path, "");
        return true;
    }

    int latest = versions[versions.size() - 1];
    m_snapshot.version = latest;
    m_snapshot.path = JoinPath(projectDir, "tasks" + std::to_string(latest) + ".txt");
    std::string text;
    if (!ReadAllText(m_snapshot.path, text)) return false;
    m_snapshot.tasks = ParseTasks(text);
    return true;
}

bool TaskManager::SaveNextVersion(const std::string& projectName,
                                  const std::vector<Task>& tasks,
                                  int expectedVersion,
                                  int& outSavedVersion,
                                  std::string& outError)
{
    if (!LoadLatest(projectName))
    {
        outError = "最新タスクファイルの読み込みに失敗しました。";
        return false;
    }

    if (m_snapshot.version != expectedVersion)
    {
        outError = "最新ではないファイルを編集しています。最新のものを読み込みなおしてください。";
        return false;
    }

    outSavedVersion = m_snapshot.version + 1;
    std::string projectDir = JoinPath(m_sharedRoot, projectName);
    std::string path = JoinPath(projectDir, "tasks" + std::to_string(outSavedVersion) + ".txt");
    if (!WriteAllText(path, SerializeTasks(tasks)))
    {
        outError = "新しいタスクファイルの保存に失敗しました。";
        return false;
    }

    std::vector<int> versions = ListVersions(projectDir);
    while (versions.size() > 100)
    {
        std::string oldPath = JoinPath(projectDir, "tasks" + std::to_string(versions.front()) + ".txt");
        DeleteFileA(oldPath.c_str());
        versions.erase(versions.begin());
    }

    LoadLatest(projectName);
    return true;
}
