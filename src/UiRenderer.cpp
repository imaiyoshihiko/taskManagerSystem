#include "UiRenderer.h"
#include "imgui.h"
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <fstream>

static const char* ROLE_ITEMS[] = {"admin", "creator", "assignee"};
static const char* PRIORITY_ITEMS[] = {"Low", "Medium", "High"};
static const char* STATUS_ITEMS[] = {"Todo", "Doing", "Done", "Blocked"};

UiRenderer::TaskEditBuffer::TaskEditBuffer()
    : assigneeIndex(0), progress(0), priorityIndex(1), statusIndex(0), createMode(false)
{
    std::memset(id, 0, sizeof(id));
    std::memset(name, 0, sizeof(name));
    std::memset(category, 0, sizeof(category));
    std::memset(effort, 0, sizeof(effort));
    std::memset(startDate, 0, sizeof(startDate));
    std::memset(endDate, 0, sizeof(endDate));
    std::memset(comment, 0, sizeof(comment));
    std::snprintf(startDate, sizeof(startDate), "%s", "2026-04-01");
    std::snprintf(endDate, sizeof(endDate), "%s", "2026-04-01");
}

UiRenderer::UiRenderer(UserManager& userManager,
                       ProjectManager& projectManager,
                       TaskManager& taskManager,
                       ScreenNavigator& navigator)
    : m_userManager(userManager), m_projectManager(projectManager), m_taskManager(taskManager),
      m_navigator(navigator), m_registerRoleIndex(1), m_registerUserIndex(0),
      m_memberProjectIndex(0), m_filterIndex(0), m_showGantt(false), m_taskEdit()
{
    std::memset(m_loginName, 0, sizeof(m_loginName));
    std::memset(m_loginPassword, 0, sizeof(m_loginPassword));
    std::memset(m_registerPassword1, 0, sizeof(m_registerPassword1));
    std::memset(m_registerPassword2, 0, sizeof(m_registerPassword2));
    std::memset(m_newProjectName, 0, sizeof(m_newProjectName));
    std::memset(m_memberNameInput, 0, sizeof(m_memberNameInput));
}

bool UiRenderer::InitializeFromCli()
{
    char modulePath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, modulePath, MAX_PATH);
    std::string exePath = modulePath;
    std::string exeDir = ".";
    size_t pos = exePath.find_last_of("\\/");
    if (pos != std::string::npos) exeDir = exePath.substr(0, pos);
    std::string configPath = exeDir + "\\sharedFolderPath.txt";

    std::string sharedRoot;
    {
        std::ifstream ifs(configPath.c_str(), std::ios::binary);
        if (ifs)
        {
            std::getline(ifs, sharedRoot, '\0');
        }
    }

    if (sharedRoot.empty())
    {
        if (GetConsoleWindow() == NULL)
        {
            AllocConsole();
            FILE* fp = NULL;
            freopen_s(&fp, "CONIN$", "r", stdin);
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
        }
        std::cout << "=== 初期設定 ===\n";
        std::cout << "共有フォルダのパスを入力してください: ";
        std::getline(std::cin, sharedRoot);
        if (sharedRoot.empty())
        {
            m_navigator.SetErrorMessage("共有フォルダパスが空です。");
            return false;
        }
        std::ofstream ofs(configPath.c_str(), std::ios::binary | std::ios::trunc);
        ofs << sharedRoot;
    }

    m_userManager.SetSharedRoot(sharedRoot);
    m_projectManager.SetSharedRoot(sharedRoot);
    m_taskManager.SetSharedRoot(sharedRoot);
    if (!m_projectManager.EnsureSharedRoot())
    {
        m_navigator.SetErrorMessage("共有フォルダの作成または確認に失敗しました。");
        return false;
    }

    m_userManager.Load();
    if (!m_userManager.HasAdmin())
    {
        if (GetConsoleWindow() == NULL)
        {
            AllocConsole();
            FILE* fp = NULL;
            freopen_s(&fp, "CONIN$", "r", stdin);
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
        }
        std::string pass1, pass2;
        std::cout << "admin ユーザの初期登録を行います。\n";
        std::cout << "パスワード: ";
        std::getline(std::cin, pass1);
        std::cout << "パスワード再入力: ";
        std::getline(std::cin, pass2);
        if (pass1.empty() || pass1 != pass2)
        {
            m_navigator.SetErrorMessage("admin のパスワード入力が不正です。");
            return false;
        }
        if (!m_userManager.SaveUser(User("admin", pass1, "admin")))
        {
            m_navigator.SetErrorMessage("admin ユーザ保存に失敗しました。");
            return false;
        }
        m_userManager.Load();
    }

    m_projectManager.Load();
    m_navigator.SetStatusMessage("初期設定が完了しました。");
    return true;
}

void UiRenderer::Render()
{
    ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(1200, 760), ImGuiCond_Once);

    switch (m_navigator.GetScreen())
    {
    case ScreenType::Login: RenderLoginScreen(); break;
    case ScreenType::Register: RenderRegisterScreen(); break;
    case ScreenType::Admin: RenderAdminScreen(); break;
    case ScreenType::TaskList: RenderTaskListScreen(); break;
    case ScreenType::TaskDetail: RenderTaskDetailScreen(); break;
    case ScreenType::TaskEdit: RenderTaskEditScreen(); break;
    case ScreenType::ProjectSwitch: RenderProjectSwitchScreen(); break;
    }
}

void UiRenderer::RenderBanner()
{
    if (!m_navigator.GetStatusMessage().empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(20, 120, 40, 255));
        ImGui::TextWrapped("%s", m_navigator.GetStatusMessage().c_str());
        ImGui::PopStyleColor();
    }
    if (!m_navigator.GetErrorMessage().empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 40, 40, 255));
        ImGui::TextWrapped("%s", m_navigator.GetErrorMessage().c_str());
        ImGui::PopStyleColor();
    }
}

void UiRenderer::ReloadAfterLogin()
{
    m_userManager.Load();
    m_projectManager.Load();
    std::vector<std::string> projects = m_projectManager.GetAccessibleProjectNames(m_navigator.GetLoggedInUser());
    m_navigator.SetAccessibleProjects(projects);
    if (!projects.empty())
    {
        m_navigator.SetCurrentProject(projects[0]);
        m_taskManager.LoadLatest(projects[0]);
    }
    else
    {
        m_navigator.SetCurrentProject("");
    }
    m_navigator.SetSelectedTaskIndex(-1);
}

int UiRenderer::ClampInt(int value, int minValue, int maxValue) const
{
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

void UiRenderer::ResetTaskEditBuffer(bool createMode)
{
    m_taskEdit = TaskEditBuffer();
    m_taskEdit.createMode = createMode;
    std::vector<std::string> members;
    m_projectManager.LoadProjectMembers(m_navigator.GetCurrentProject(), members);

    if (createMode)
    {
        int maxId = 0;
        const std::vector<Task>& tasks = m_taskManager.GetTasks();
        for (size_t i = 0; i < tasks.size(); ++i)
        {
            if (tasks[i].GetId() > maxId) maxId = tasks[i].GetId();
        }
        std::snprintf(m_taskEdit.id, sizeof(m_taskEdit.id), "%d", maxId + 1);
        return;
    }

    int index = m_navigator.GetSelectedTaskIndex();
    const std::vector<Task>& tasks = m_taskManager.GetTasks();
    if (index < 0 || index >= static_cast<int>(tasks.size())) return;
    const Task& task = tasks[index];
    std::snprintf(m_taskEdit.id, sizeof(m_taskEdit.id), "%d", task.GetId());
    std::snprintf(m_taskEdit.name, sizeof(m_taskEdit.name), "%s", task.GetName().c_str());
    std::snprintf(m_taskEdit.category, sizeof(m_taskEdit.category), "%s", task.GetCategory().c_str());
    std::snprintf(m_taskEdit.effort, sizeof(m_taskEdit.effort), "%s", task.GetEffort().c_str());
    std::snprintf(m_taskEdit.startDate, sizeof(m_taskEdit.startDate), "%s", task.GetStartDate().c_str());
    std::snprintf(m_taskEdit.endDate, sizeof(m_taskEdit.endDate), "%s", task.GetEndDate().c_str());
    std::snprintf(m_taskEdit.comment, sizeof(m_taskEdit.comment), "%s", task.GetComment().c_str());
    m_taskEdit.progress = task.GetProgress();

    for (int i = 0; i < 3; ++i) if (task.GetPriority() == PRIORITY_ITEMS[i]) m_taskEdit.priorityIndex = i;
    for (int i = 0; i < 4; ++i) if (task.GetStatus() == STATUS_ITEMS[i]) m_taskEdit.statusIndex = i;
    for (size_t i = 0; i < members.size(); ++i) if (members[i] == task.GetAssignee()) m_taskEdit.assigneeIndex = static_cast<int>(i);
}

void UiRenderer::ApplyTaskEdit(Task& task)
{
    std::vector<std::string> members;
    m_projectManager.LoadProjectMembers(m_navigator.GetCurrentProject(), members);
    task.SetId(std::atoi(m_taskEdit.id));
    task.SetName(m_taskEdit.name);
    task.SetCategory(m_taskEdit.category);
    if (!members.empty())
    {
        int idx = ClampInt(m_taskEdit.assigneeIndex, 0, static_cast<int>(members.size()) - 1);
        task.SetAssignee(members[idx]);
    }
    else
    {
        task.SetAssignee(m_navigator.GetLoggedInUser().GetUsername());
    }
    task.SetEffort(m_taskEdit.effort);
    task.SetStartDate(m_taskEdit.startDate);
    task.SetEndDate(m_taskEdit.endDate);
    task.SetProgress(m_taskEdit.progress);
    task.SetPriority(PRIORITY_ITEMS[m_taskEdit.priorityIndex]);
    task.SetStatus(STATUS_ITEMS[m_taskEdit.statusIndex]);
    if (m_taskEdit.createMode) task.SetCreator(m_navigator.GetLoggedInUser().GetUsername());
    task.SetComment(m_taskEdit.comment);
}

bool UiRenderer::SaveTaskList(const std::vector<Task>& tasks)
{
    int savedVersion = 0;
    std::string error;
    if (!m_taskManager.SaveNextVersion(m_navigator.GetCurrentProject(), tasks, m_taskManager.GetCurrentVersion(), savedVersion, error))
    {
        m_navigator.SetErrorMessage(error);
        return false;
    }
    m_navigator.SetStatusMessage("保存しました。tasks" + std::to_string(savedVersion) + ".txt");
    m_taskManager.LoadLatest(m_navigator.GetCurrentProject());
    return true;
}

bool UiRenderer::CurrentUserCanEditAllFields() const
{
    if (!m_navigator.IsLoggedIn()) return false;
    const User& user = m_navigator.GetLoggedInUser();
    if (user.IsAdmin()) return true;

    int index = m_navigator.GetSelectedTaskIndex();
    const std::vector<Task>& tasks = m_taskManager.GetTasks();
    if (index < 0 || index >= static_cast<int>(tasks.size())) return user.IsCreator();
    return user.IsCreator() || tasks[index].GetCreator() == user.GetUsername();
}

bool UiRenderer::CurrentUserCanDeleteSelected() const
{
    return CurrentUserCanEditAllFields();
}

void UiRenderer::RenderLoginScreen()
{
    ImGui::Begin("ログイン");
    RenderBanner();
    ImGui::InputText("ユーザ名", m_loginName, IM_ARRAYSIZE(m_loginName));
    ImGui::InputText("パスワード", m_loginPassword, IM_ARRAYSIZE(m_loginPassword), ImGuiInputTextFlags_Password);

    if (ImGui::Button("ログイン", ImVec2(160, 32)))
    {
        m_navigator.ClearBanner();
        m_userManager.Load();
        User user;
        if (m_userManager.ValidateLogin(m_loginName, m_loginPassword, user))
        {
            m_navigator.SetLoggedInUser(user);
            ReloadAfterLogin();
            m_navigator.SetScreen(user.IsAdmin() ? ScreenType::Admin : ScreenType::TaskList);
            m_navigator.SetStatusMessage("ログインしました。");
        }
        else
        {
            m_navigator.SetErrorMessage("ユーザ名またはパスワードが違います。");
        }
    }
    if (ImGui::Button("新規登録", ImVec2(160, 32)))
    {
        m_navigator.ClearBanner();
        m_navigator.SetScreen(ScreenType::Register);
    }
    ImGui::End();
}

void UiRenderer::RenderRegisterScreen()
{
    ImGui::Begin("新規登録");
    RenderBanner();
    m_projectManager.Load();
    m_userManager.Load();

    std::vector<std::string> members = m_projectManager.CollectAllMemberNames();
    if (members.empty()) members.push_back("(メンバなし)");
    std::vector<const char*> items;
    for (size_t i = 0; i < members.size(); ++i) items.push_back(members[i].c_str());

    ImGui::Combo("権限", &m_registerRoleIndex, ROLE_ITEMS, IM_ARRAYSIZE(ROLE_ITEMS));
    ImGui::BeginDisabled(m_registerRoleIndex == 0);
    ImGui::Combo("ユーザ名(メンバ名)", &m_registerUserIndex, items.data(), static_cast<int>(items.size()));
    ImGui::EndDisabled();
    ImGui::InputText("パスワード", m_registerPassword1, IM_ARRAYSIZE(m_registerPassword1), ImGuiInputTextFlags_Password);
    ImGui::InputText("パスワード再入力", m_registerPassword2, IM_ARRAYSIZE(m_registerPassword2), ImGuiInputTextFlags_Password);

    if (ImGui::Button("登録", ImVec2(160, 32)))
    {
        m_navigator.ClearBanner();
        std::string role = ROLE_ITEMS[m_registerRoleIndex];
        std::string username = (role == "admin") ? "admin" : members[ClampInt(m_registerUserIndex, 0, static_cast<int>(members.size()) - 1)];
        if (role == "admin")
        {
            m_navigator.SetErrorMessage("admin は CLI 初期設定でのみ登録できます。");
        }
        else if (std::strlen(m_registerPassword1) == 0 || std::strcmp(m_registerPassword1, m_registerPassword2) != 0)
        {
            m_navigator.SetErrorMessage("パスワードが一致しないか空です。");
        }
        else if (m_userManager.HasUser(username))
        {
            m_navigator.SetErrorMessage("同名ユーザは登録済みです。");
        }
        else if (!m_userManager.SaveUser(User(username, m_registerPassword1, role)))
        {
            m_navigator.SetErrorMessage("userData.txt への追記に失敗しました。");
        }
        else
        {
            std::memset(m_registerPassword1, 0, sizeof(m_registerPassword1));
            std::memset(m_registerPassword2, 0, sizeof(m_registerPassword2));
            m_userManager.Load();
            m_navigator.SetStatusMessage("新規登録が完了しました。ログインしてください。");
            m_navigator.SetScreen(ScreenType::Login);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("戻る", ImVec2(160, 32)))
    {
        m_navigator.ClearBanner();
        m_navigator.SetScreen(ScreenType::Login);
    }
    ImGui::End();
}

void UiRenderer::RenderAdminScreen()
{
    m_projectManager.Load();
    ImGui::Begin("管理者画面");
    RenderBanner();
    ImGui::Text("共有フォルダ: %s", m_projectManager.GetSharedRoot().c_str());
    ImGui::Separator();

    ImGui::InputText("新規プロジェクト名", m_newProjectName, IM_ARRAYSIZE(m_newProjectName));
    if (ImGui::Button("プロジェクト作成", ImVec2(180, 32)))
    {
        m_navigator.ClearBanner();
        std::string error;
        if (m_projectManager.CreateProject(m_newProjectName, error))
        {
            m_navigator.SetStatusMessage("プロジェクトを作成しました。");
        }
        else
        {
            m_navigator.SetErrorMessage(error);
        }
    }

    ImGui::Separator();
    const std::vector<Project>& projects = m_projectManager.GetProjects();
    if (!projects.empty())
    {
        std::vector<const char*> projectItems;
        for (size_t i = 0; i < projects.size(); ++i) projectItems.push_back(projects[i].GetName().c_str());
        m_memberProjectIndex = ClampInt(m_memberProjectIndex, 0, static_cast<int>(projects.size()) - 1);
        ImGui::Combo("メンバ編集対象", &m_memberProjectIndex, projectItems.data(), static_cast<int>(projectItems.size()));

        const Project& project = projects[m_memberProjectIndex];
        ImGui::Text("選択中プロジェクト: %s", project.GetName().c_str());
        ImGui::InputText("メンバ名", m_memberNameInput, IM_ARRAYSIZE(m_memberNameInput));
        if (ImGui::Button("メンバ追加", ImVec2(150, 30)))
        {
            m_navigator.ClearBanner();
            if (m_projectManager.AddMember(project.GetName(), m_memberNameInput))
                m_navigator.SetStatusMessage("メンバを追加しました。");
            else
                m_navigator.SetErrorMessage("メンバ追加に失敗しました。");
        }

        ImGui::Separator();
        ImGui::Text("メンバ一覧");
        const std::vector<std::string>& members = project.GetMembers();
        for (size_t i = 0; i < members.size(); ++i)
        {
            ImGui::BulletText("%s", members[i].c_str());
            ImGui::SameLine(300);
            std::string button = "削除##member" + std::to_string(i);
            if (ImGui::SmallButton(button.c_str()))
            {
                m_navigator.ClearBanner();
                if (m_projectManager.RemoveMember(project.GetName(), members[i]))
                    m_navigator.SetStatusMessage("メンバを削除しました。");
                else
                    m_navigator.SetErrorMessage("メンバ削除に失敗しました。");
                break;
            }
        }
    }

    ImGui::Separator();
    if (ImGui::Button("タスク一覧へ", ImVec2(180, 32)))
    {
        ReloadAfterLogin();
        if (!m_navigator.GetAccessibleProjects().empty())
        {
            m_navigator.SetScreen(ScreenType::TaskList);
        }
        else
        {
            m_navigator.SetErrorMessage("アクセス可能なプロジェクトがありません。");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("ログアウト", ImVec2(180, 32)))
    {
        m_navigator.Logout();
        m_navigator.SetScreen(ScreenType::Login);
        m_navigator.SetStatusMessage("ログアウトしました。");
    }
    ImGui::End();
}

bool UiRenderer::ParseDate(const std::string& text, SimpleDate& out) const
{
    return std::sscanf(text.c_str(), "%d-%d-%d", &out.year, &out.month, &out.day) == 3;
}

time_t UiRenderer::ToTimeT(const SimpleDate& value) const
{
    std::tm tmv = {};
    tmv.tm_year = value.year - 1900;
    tmv.tm_mon = value.month - 1;
    tmv.tm_mday = value.day;
    tmv.tm_isdst = -1;
    return std::mktime(&tmv);
}

int UiRenderer::DaysBetween(const SimpleDate& a, const SimpleDate& b) const
{
    return static_cast<int>(std::difftime(ToTimeT(b), ToTimeT(a)) / (60 * 60 * 24));
}

UiRenderer::SimpleDate UiRenderer::AddDays(const SimpleDate& value, int days) const
{
    time_t t = ToTimeT(value) + static_cast<time_t>(days) * 24 * 60 * 60;
    std::tm* tmv = std::localtime(&t);
    SimpleDate result;
    result.year = tmv->tm_year + 1900;
    result.month = tmv->tm_mon + 1;
    result.day = tmv->tm_mday;
    return result;
}

void UiRenderer::RenderTaskListScreen()
{
    ImGui::Begin("タスク一覧");
    RenderBanner();
    const User& user = m_navigator.GetLoggedInUser();
    ImGui::Text("ユーザ: %s / 権限: %s / プロジェクト: %s / version=%d",
                user.GetUsername().c_str(), user.GetRole().c_str(),
                m_navigator.GetCurrentProject().c_str(), m_taskManager.GetCurrentVersion());

    if (ImGui::Button("登録", ImVec2(90, 30)))
    {
        m_navigator.ClearBanner();
        m_navigator.SetSelectedTaskIndex(-1);
        ResetTaskEditBuffer(true);
        m_navigator.SetScreen(ScreenType::TaskEdit);
    }
    ImGui::SameLine();
    if (ImGui::Button("削除", ImVec2(90, 30)))
    {
        m_navigator.ClearBanner();
        int index = m_navigator.GetSelectedTaskIndex();
        std::vector<Task> next = m_taskManager.GetTasks();
        if (index < 0 || index >= static_cast<int>(next.size()))
        {
            m_navigator.SetErrorMessage("削除対象のタスクを選択してください。");
        }
        else if (!CurrentUserCanDeleteSelected())
        {
            m_navigator.SetErrorMessage("このタスクを削除する権限がありません。");
        }
        else
        {
            next.erase(next.begin() + index);
            SaveTaskList(next);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("再読込", ImVec2(90, 30)))
    {
        m_navigator.ClearBanner();
        m_taskManager.LoadLatest(m_navigator.GetCurrentProject());
        m_navigator.SetSelectedTaskIndex(-1);
    }
    ImGui::SameLine();
    if (ImGui::Button("ガントチャート", ImVec2(130, 30)))
    {
        m_showGantt = !m_showGantt;
    }
    ImGui::SameLine(700);
    if (ImGui::Button("プロジェクト切替", ImVec2(150, 30)))
    {
        m_navigator.SetScreen(ScreenType::ProjectSwitch);
    }

    std::vector<std::string> members;
    m_projectManager.LoadProjectMembers(m_navigator.GetCurrentProject(), members);
    std::vector<const char*> filterItems;
    filterItems.push_back("(全員)");
    for (size_t i = 0; i < members.size(); ++i) filterItems.push_back(members[i].c_str());
    ImGui::Combo("担当者絞り込み", &m_filterIndex, filterItems.data(), static_cast<int>(filterItems.size()));

    if (ImGui::BeginTable("task_table", 10, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 320)))
    {
        ImGui::TableSetupColumn("ID");
        ImGui::TableSetupColumn("タスク名");
        ImGui::TableSetupColumn("分類");
        ImGui::TableSetupColumn("担当者");
        ImGui::TableSetupColumn("工数");
        ImGui::TableSetupColumn("開始");
        ImGui::TableSetupColumn("終了");
        ImGui::TableSetupColumn("進捗");
        ImGui::TableSetupColumn("優先度");
        ImGui::TableSetupColumn("状態");
        ImGui::TableHeadersRow();

        const std::vector<Task>& tasks = m_taskManager.GetTasks();
        for (size_t i = 0; i < tasks.size(); ++i)
        {
            const Task& task = tasks[i];
            if (m_filterIndex > 0 && task.GetAssignee() != members[m_filterIndex - 1]) continue;
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::Text("%d", task.GetId());
            ImGui::TableSetColumnIndex(1);
            std::string label = task.GetName() + "##task" + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), m_navigator.GetSelectedTaskIndex() == static_cast<int>(i), ImGuiSelectableFlags_SpanAllColumns))
            {
                m_navigator.SetSelectedTaskIndex(static_cast<int>(i));
                m_navigator.SetScreen(ScreenType::TaskDetail);
            }
            ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(task.GetCategory().c_str());
            ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(task.GetAssignee().c_str());
            ImGui::TableSetColumnIndex(4); ImGui::TextUnformatted(task.GetEffort().c_str());
            ImGui::TableSetColumnIndex(5); ImGui::TextUnformatted(task.GetStartDate().c_str());
            ImGui::TableSetColumnIndex(6); ImGui::TextUnformatted(task.GetEndDate().c_str());
            ImGui::TableSetColumnIndex(7); ImGui::Text("%d%%", task.GetProgress());
            ImGui::TableSetColumnIndex(8); ImGui::TextUnformatted(task.GetPriority().c_str());
            ImGui::TableSetColumnIndex(9); ImGui::TextUnformatted(task.GetStatus().c_str());
        }
        ImGui::EndTable();
    }

    if (m_showGantt)
    {
        ImGui::Separator();
        ImGui::Text("ガントチャート");
        ImGui::BeginChild("gantt_child", ImVec2(0, 260), true, ImGuiWindowFlags_HorizontalScrollbar);
        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 origin = ImGui::GetCursorScreenPos();

        const std::vector<Task>& tasks = m_taskManager.GetTasks();
        if (!tasks.empty())
        {
            SimpleDate minDate = {2026, 4, 1};
            SimpleDate maxDate = {2026, 4, 30};
            bool initialized = false;
            for (size_t i = 0; i < tasks.size(); ++i)
            {
                SimpleDate s, e;
                if (!ParseDate(tasks[i].GetStartDate(), s) || !ParseDate(tasks[i].GetEndDate(), e)) continue;
                if (!initialized || DaysBetween(s, minDate) < 0) minDate = s;
                if (!initialized || DaysBetween(maxDate, e) < 0) maxDate = e;
                initialized = true;
            }

            const float dayWidth = 24.0f;
            const float rowHeight = 28.0f;
            const float leftWidth = 220.0f;
            const float headerHeight = 52.0f;
            int totalDays = std::max(1, DaysBetween(minDate, maxDate) + 1);
            float width = leftWidth + totalDays * dayWidth;
            float height = headerHeight + rowHeight * tasks.size();

            draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(255, 255, 255, 255));
            draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(180, 180, 180, 255));

            for (int d = 0; d < totalDays; ++d)
            {
                float x = origin.x + leftWidth + d * dayWidth;
                SimpleDate cur = AddDays(minDate, d);
                draw->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + height), IM_COL32(220, 220, 220, 255));
                char buf[16] = {0};
                std::snprintf(buf, sizeof(buf), "%d/%d", cur.month, cur.day);
                draw->AddText(ImVec2(x + 2, origin.y + 4), IM_COL32(0, 0, 0, 255), buf);
            }

            for (size_t i = 0; i < tasks.size(); ++i)
            {
                const Task& task = tasks[i];
                float y = origin.y + headerHeight + i * rowHeight;
                draw->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + width, y), IM_COL32(220, 220, 220, 255));
                draw->AddText(ImVec2(origin.x + 4, y + 5), IM_COL32(0, 0, 0, 255), task.GetName().c_str());

                SimpleDate s, e;
                if (!ParseDate(task.GetStartDate(), s) || !ParseDate(task.GetEndDate(), e)) continue;
                int startOffset = DaysBetween(minDate, s);
                int endOffset = DaysBetween(minDate, e);
                float x1 = origin.x + leftWidth + startOffset * dayWidth;
                float x2 = origin.x + leftWidth + (endOffset + 1) * dayWidth;
                draw->AddRectFilled(ImVec2(x1, y + 4), ImVec2(x2, y + rowHeight - 4), IM_COL32(120, 170, 255, 255), 3.0f);
                float progressWidth = (x2 - x1) * (ClampInt(task.GetProgress(), 0, 100) / 100.0f);
                draw->AddRectFilled(ImVec2(x1, y + 4), ImVec2(x1 + progressWidth, y + rowHeight - 4), IM_COL32(80, 190, 110, 255), 3.0f);
                draw->AddRect(ImVec2(x1, y + 4), ImVec2(x2, y + rowHeight - 4), IM_COL32(60, 60, 60, 255), 3.0f);
            }
            ImGui::Dummy(ImVec2(width, height));
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void UiRenderer::RenderTaskDetailScreen()
{
    ImGui::Begin("タスク詳細");
    RenderBanner();
    int index = m_navigator.GetSelectedTaskIndex();
    const std::vector<Task>& tasks = m_taskManager.GetTasks();
    if (index < 0 || index >= static_cast<int>(tasks.size()))
    {
        ImGui::Text("タスクが選択されていません。");
    }
    else
    {
        const Task& task = tasks[index];
        ImGui::Text("ID: %d", task.GetId());
        ImGui::Text("タスク名: %s", task.GetName().c_str());
        ImGui::Text("分類: %s", task.GetCategory().c_str());
        ImGui::Text("担当者: %s", task.GetAssignee().c_str());
        ImGui::Text("工数: %s", task.GetEffort().c_str());
        ImGui::Text("開始日: %s", task.GetStartDate().c_str());
        ImGui::Text("終了日: %s", task.GetEndDate().c_str());
        ImGui::Text("進捗: %d%%", task.GetProgress());
        ImGui::Text("優先度: %s", task.GetPriority().c_str());
        ImGui::Text("ステータス: %s", task.GetStatus().c_str());
        ImGui::Text("作成者: %s", task.GetCreator().c_str());
        ImGui::Separator();
        ImGui::TextWrapped("コメント: %s", task.GetComment().c_str());
    }

    if (ImGui::Button("編集", ImVec2(150, 32)))
    {
        m_navigator.ClearBanner();
        ResetTaskEditBuffer(false);
        m_navigator.SetScreen(ScreenType::TaskEdit);
    }
    ImGui::SameLine();
    if (ImGui::Button("戻る", ImVec2(150, 32)))
    {
        m_navigator.SetScreen(ScreenType::TaskList);
    }
    ImGui::End();
}

void UiRenderer::RenderTaskEditScreen()
{
    ImGui::Begin("タスク編集");
    RenderBanner();
    bool fullEdit = m_taskEdit.createMode || CurrentUserCanEditAllFields();
    std::vector<std::string> members;
    m_projectManager.LoadProjectMembers(m_navigator.GetCurrentProject(), members);
    if (members.empty()) members.push_back(m_navigator.GetLoggedInUser().GetUsername());
    std::vector<const char*> memberItems;
    for (size_t i = 0; i < members.size(); ++i) memberItems.push_back(members[i].c_str());

    ImGui::InputText("ID", m_taskEdit.id, IM_ARRAYSIZE(m_taskEdit.id), ImGuiInputTextFlags_ReadOnly);
    ImGui::BeginDisabled(!fullEdit);
    ImGui::InputText("タスク名", m_taskEdit.name, IM_ARRAYSIZE(m_taskEdit.name));
    ImGui::InputText("分類", m_taskEdit.category, IM_ARRAYSIZE(m_taskEdit.category));
    ImGui::Combo("担当者", &m_taskEdit.assigneeIndex, memberItems.data(), static_cast<int>(memberItems.size()));
    ImGui::InputText("工数", m_taskEdit.effort, IM_ARRAYSIZE(m_taskEdit.effort));
    ImGui::InputText("開始日", m_taskEdit.startDate, IM_ARRAYSIZE(m_taskEdit.startDate));
    ImGui::InputText("終了日", m_taskEdit.endDate, IM_ARRAYSIZE(m_taskEdit.endDate));
    ImGui::Combo("優先度", &m_taskEdit.priorityIndex, PRIORITY_ITEMS, IM_ARRAYSIZE(PRIORITY_ITEMS));
    ImGui::Combo("ステータス", &m_taskEdit.statusIndex, STATUS_ITEMS, IM_ARRAYSIZE(STATUS_ITEMS));
    ImGui::EndDisabled();
    ImGui::SliderInt("進捗", &m_taskEdit.progress, 0, 100);
    ImGui::InputTextMultiline("コメント", m_taskEdit.comment, IM_ARRAYSIZE(m_taskEdit.comment), ImVec2(-1, 120));

    if (ImGui::Button("保存", ImVec2(150, 32)))
    {
        m_navigator.ClearBanner();
        if (std::strlen(m_taskEdit.name) == 0)
        {
            m_navigator.SetErrorMessage("タスク名を入力してください。");
        }
        else
        {
            std::vector<Task> next = m_taskManager.GetTasks();
            if (m_taskEdit.createMode)
            {
                Task task;
                ApplyTaskEdit(task);
                next.push_back(task);
                if (SaveTaskList(next)) m_navigator.SetScreen(ScreenType::TaskList);
            }
            else
            {
                int index = m_navigator.GetSelectedTaskIndex();
                if (index >= 0 && index < static_cast<int>(next.size()))
                {
                    Task original = next[index];
                    ApplyTaskEdit(next[index]);
                    if (!fullEdit)
                    {
                        next[index].SetName(original.GetName());
                        next[index].SetCategory(original.GetCategory());
                        next[index].SetAssignee(original.GetAssignee());
                        next[index].SetEffort(original.GetEffort());
                        next[index].SetStartDate(original.GetStartDate());
                        next[index].SetEndDate(original.GetEndDate());
                        next[index].SetPriority(original.GetPriority());
                        next[index].SetStatus(original.GetStatus());
                        next[index].SetCreator(original.GetCreator());
                    }
                    if (SaveTaskList(next)) m_navigator.SetScreen(ScreenType::TaskList);
                }
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("キャンセル", ImVec2(150, 32)))
    {
        m_navigator.SetScreen(m_navigator.GetSelectedTaskIndex() >= 0 ? ScreenType::TaskDetail : ScreenType::TaskList);
    }
    ImGui::End();
}

void UiRenderer::RenderProjectSwitchScreen()
{
    ImGui::Begin("プロジェクト切り替え");
    RenderBanner();
    const std::vector<std::string>& projects = m_navigator.GetAccessibleProjects();
    if (projects.empty())
    {
        ImGui::Text("切り替え可能なプロジェクトがありません。");
    }
    else
    {
        for (size_t i = 0; i < projects.size(); ++i)
        {
            if (ImGui::Button(projects[i].c_str(), ImVec2(300, 32)))
            {
                m_navigator.SetCurrentProject(projects[i]);
                m_taskManager.LoadLatest(projects[i]);
                m_navigator.SetSelectedTaskIndex(-1);
                m_navigator.SetScreen(ScreenType::TaskList);
            }
        }
    }
    if (ImGui::Button("戻る", ImVec2(150, 32)))
    {
        m_navigator.SetScreen(ScreenType::TaskList);
    }
    ImGui::End();
}
