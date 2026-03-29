#ifndef TASK_MANAGER_MODELS_H
#define TASK_MANAGER_MODELS_H

#include <string>
#include <vector>

struct UserAccount
{
    std::string username;
    std::string password;
    std::string role; // admin / creator / assignee
};

struct TaskItem
{
    int id = 0;
    std::string name;
    std::string category;
    std::string assignee;
    std::string effort;
    std::string startDate;   // YYYY-MM-DD
    std::string endDate;     // YYYY-MM-DD
    int progress = 0;        // 0-100
    std::string priority;
    std::string status;
    std::string creator;
    std::string comment;
};

struct ProjectInfo
{
    std::string name;
    std::vector<std::string> members;
};

#endif
