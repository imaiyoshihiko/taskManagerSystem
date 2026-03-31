#include "Task.h"

Task::Task()
    : m_id(0), m_name(), m_category(), m_assignee(), m_effort(),
      m_startDate(), m_endDate(), m_progress(0), m_priority(),
      m_status(), m_creator(), m_comment()
{
}

int Task::GetId() const { return m_id; }
void Task::SetId(int id) { m_id = id; }
const std::string& Task::GetName() const { return m_name; }
void Task::SetName(const std::string& value) { m_name = value; }
const std::string& Task::GetCategory() const { return m_category; }
void Task::SetCategory(const std::string& value) { m_category = value; }
const std::string& Task::GetAssignee() const { return m_assignee; }
void Task::SetAssignee(const std::string& value) { m_assignee = value; }
const std::string& Task::GetEffort() const { return m_effort; }
void Task::SetEffort(const std::string& value) { m_effort = value; }
const std::string& Task::GetStartDate() const { return m_startDate; }
void Task::SetStartDate(const std::string& value) { m_startDate = value; }
const std::string& Task::GetEndDate() const { return m_endDate; }
void Task::SetEndDate(const std::string& value) { m_endDate = value; }
int Task::GetProgress() const { return m_progress; }
void Task::SetProgress(int value) { m_progress = value; }
const std::string& Task::GetPriority() const { return m_priority; }
void Task::SetPriority(const std::string& value) { m_priority = value; }
const std::string& Task::GetStatus() const { return m_status; }
void Task::SetStatus(const std::string& value) { m_status = value; }
const std::string& Task::GetCreator() const { return m_creator; }
void Task::SetCreator(const std::string& value) { m_creator = value; }
const std::string& Task::GetComment() const { return m_comment; }
void Task::SetComment(const std::string& value) { m_comment = value; }
