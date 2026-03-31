#ifndef TASK_H
#define TASK_H

#include <string>

class Task
{
public:
    Task();

    int GetId() const;
    void SetId(int id);

    const std::string& GetName() const;
    void SetName(const std::string& value);

    const std::string& GetCategory() const;
    void SetCategory(const std::string& value);

    const std::string& GetAssignee() const;
    void SetAssignee(const std::string& value);

    const std::string& GetEffort() const;
    void SetEffort(const std::string& value);

    const std::string& GetStartDate() const;
    void SetStartDate(const std::string& value);

    const std::string& GetEndDate() const;
    void SetEndDate(const std::string& value);

    int GetProgress() const;
    void SetProgress(int value);

    const std::string& GetPriority() const;
    void SetPriority(const std::string& value);

    const std::string& GetStatus() const;
    void SetStatus(const std::string& value);

    const std::string& GetCreator() const;
    void SetCreator(const std::string& value);

    const std::string& GetComment() const;
    void SetComment(const std::string& value);

private:
    int m_id;
    std::string m_name;
    std::string m_category;
    std::string m_assignee;
    std::string m_effort;
    std::string m_startDate;
    std::string m_endDate;
    int m_progress;
    std::string m_priority;
    std::string m_status;
    std::string m_creator;
    std::string m_comment;
};

#endif
