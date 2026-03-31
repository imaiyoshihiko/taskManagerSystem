#include "UserManager.h"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

UserManager::UserManager() : m_sharedRoot(), m_users() {}
UserManager::UserManager(const std::string& sharedRoot) : m_sharedRoot(sharedRoot), m_users() {}

void UserManager::SetSharedRoot(const std::string& sharedRoot) { m_sharedRoot = sharedRoot; }
const std::string& UserManager::GetSharedRoot() const { return m_sharedRoot; }
const std::vector<User>& UserManager::GetUsers() const { return m_users; }

std::string UserManager::BuildUserDataPath() const
{
    return m_sharedRoot + "\\userData.txt";
}

std::string UserManager::Trim(const std::string& text) const
{
    size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) ++begin;
    size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) --end;
    return text.substr(begin, end - begin);
}

std::string UserManager::EscapeCsv(const std::string& value) const
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

std::vector<std::string> UserManager::SplitCsv(const std::string& line) const
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

bool UserManager::Load()
{
    m_users.clear();
    std::ifstream ifs(BuildUserDataPath().c_str(), std::ios::binary);
    if (!ifs) return true;

    std::string line;
    while (std::getline(ifs, line))
    {
        line = Trim(line);
        if (line.empty()) continue;
        std::vector<std::string> fields = SplitCsv(line);
        if (fields.size() < 3) continue;
        m_users.push_back(User(fields[0], fields[1], fields[2]));
    }
    return true;
}

bool UserManager::SaveUser(const User& user)
{
    std::ofstream ofs(BuildUserDataPath().c_str(), std::ios::app | std::ios::binary);
    if (!ofs) return false;
    ofs << EscapeCsv(user.GetUsername()) << ','
        << EscapeCsv(user.GetPassword()) << ','
        << EscapeCsv(user.GetRole()) << "\n";
    ofs.flush();
    return ofs.good();
}

bool UserManager::HasUser(const std::string& username) const
{
    for (size_t i = 0; i < m_users.size(); ++i)
    {
        if (m_users[i].GetUsername() == username) return true;
    }
    return false;
}

bool UserManager::HasAdmin() const
{
    for (size_t i = 0; i < m_users.size(); ++i)
    {
        if (m_users[i].GetUsername() == "admin" && m_users[i].GetRole() == "admin") return true;
    }
    return false;
}

bool UserManager::ValidateLogin(const std::string& username, const std::string& password, User& outUser) const
{
    for (size_t i = 0; i < m_users.size(); ++i)
    {
        if (m_users[i].GetUsername() == username && m_users[i].GetPassword() == password)
        {
            outUser = m_users[i];
            return true;
        }
    }
    return false;
}
