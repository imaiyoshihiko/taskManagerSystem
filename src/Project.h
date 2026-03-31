#ifndef PROJECT_H
#define PROJECT_H

#include <string>
#include <vector>

class Project
{
public:
    Project();
    explicit Project(const std::string& name);

    const std::string& GetName() const;
    void SetName(const std::string& name);

    const std::vector<std::string>& GetMembers() const;
    std::vector<std::string>& GetMembers();
    bool HasMember(const std::string& username) const;
    void AddMember(const std::string& username);
    void RemoveMember(const std::string& username);

private:
    std::string m_name;
    std::vector<std::string> m_members;
};

#endif
