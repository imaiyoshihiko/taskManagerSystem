#include "Project.h"
#include <algorithm>

Project::Project() : m_name(), m_members() {}
Project::Project(const std::string& name) : m_name(name), m_members() {}

const std::string& Project::GetName() const { return m_name; }
void Project::SetName(const std::string& name) { m_name = name; }

const std::vector<std::string>& Project::GetMembers() const { return m_members; }
std::vector<std::string>& Project::GetMembers() { return m_members; }

bool Project::HasMember(const std::string& username) const
{
    return std::find(m_members.begin(), m_members.end(), username) != m_members.end();
}

void Project::AddMember(const std::string& username)
{
    if (username.empty() || HasMember(username)) return;
    m_members.push_back(username);
}

void Project::RemoveMember(const std::string& username)
{
    std::vector<std::string>::iterator it = std::remove(m_members.begin(), m_members.end(), username);
    m_members.erase(it, m_members.end());
}
