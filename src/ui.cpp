#include "ui.h"
#include "app_state.h"
#include "storage.h"

#include "imgui.h"

#include <algorithm>
#include <ctime>
#include <cstdio>
#include <cstring>

namespace
{
    const char* kRoleItems[] = {"admin", "creator", "assignee"};
    const char* kPriorityItems[] = {"Low", "Medium", "High"};
    const char* kStatusItems[] = {"Todo", "Doing", "Done", "Blocked"};

    struct SimpleDate
    {
        int year = 2026;
        int month = 1;
        int day = 1;
    };

    bool ParseDate(const std::string& s, SimpleDate& out)
    {
        return std::sscanf(s.c_str(), "%d-%d-%d", &out.year, &out.month, &out.day) == 3;
    }

    time_t ToTimeT(const SimpleDate& d)
    {
        std::tm tmv = {};
        tmv.tm_year = d.year - 1900;
        tmv.tm_mon = d.month - 1;
        tmv.tm_mday = d.day;
        tmv.tm_isdst = -1;
        return std::mktime(&tmv);
    }

    int DaysBetween(const SimpleDate& a, const SimpleDate& b)
    {
        return static_cast<int>(std::difftime(ToTimeT(b), ToTimeT(a)) / (60 * 60 * 24));
    }

    SimpleDate AddDays(const SimpleDate& d, int days)
    {
        time_t t = ToTimeT(d) + static_cast<time_t>(days) * 24 * 60 * 60;
        std::tm* tmv = std::localtime(&t);
        SimpleDate r;
        r.year = tmv->tm_year + 1900;
        r.month = tmv->tm_mon + 1;
        r.day = tmv->tm_mday;
        return r;
    }

    std::vector<std::string> CurrentProjectMembers()
    {
        AppState& app = GetAppState();
        std::vector<std::string> members;
        Storage::LoadProjectMembers(app.sharedRoot, app.currentProject, members);
        return members;
    }

    void ShowBanner()
    {
        AppState& app = GetAppState();
        if (!app.message.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(20, 120, 40, 255));
            ImGui::TextWrapped("%s", app.message.c_str());
            ImGui::PopStyleColor();
        }
        if (!app.error.empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 40, 40, 255));
            ImGui::TextWrapped("%s", app.error.c_str());
            ImGui::PopStyleColor();
        }
    }

    void ClearBanner()
    {
        AppState& app = GetAppState();
        app.message.clear();
        app.error.clear();
    }

    void ApplyTaskFromEditor(TaskItem& t)
    {
        AppState& app = GetAppState();
        std::vector<std::string> members = CurrentProjectMembers();
        t.id = std::atoi(app.editId);
        t.name = app.editName;
        t.category = app.editCategory;
        t.assignee = members.empty() ? "" : members[std::clamp(app.editAssigneeIndex, 0, static_cast<int>(members.size()) - 1)];
        t.effort = app.editEffort;
        t.startDate = app.editStartDate;
        t.endDate = app.editEndDate;
        t.progress = app.editProgress;
        t.priority = kPriorityItems[app.editPriorityIndex];
        t.status = kStatusItems[app.editStatusIndex];
        if (app.createMode)
        {
            t.creator = app.currentUser.username;
        }
        t.comment = app.editComment;
    }

    bool SaveTaskList(std::vector<TaskItem>& tasks)
    {
        AppState& app = GetAppState();
        int savedVersion = 0;
        std::string error;
        if (!Storage::SaveTasksAsNextVersion(app.sharedRoot, app.currentProject, tasks, app.currentTaskFileVersion, savedVersion, error))
        {
            app.error = error;
            return false;
        }
        app.message = "保存しました。tasks" + std::to_string(savedVersion) + ".txt";
        ReloadCurrentProjectTasks();
        return true;
    }

    void RenderLoginScreen()
    {
        AppState& app = GetAppState();
        ImGui::Begin("ログイン");
        ShowBanner();
        ImGui::InputText("ユーザ名", app.loginName, IM_ARRAYSIZE(app.loginName));
        ImGui::InputText("パスワード", app.loginPassword, IM_ARRAYSIZE(app.loginPassword), ImGuiInputTextFlags_Password);

        if (ImGui::Button("ログイン", ImVec2(160, 32)))
        {
            ClearBanner();
            ReloadProjectsAndUsers();
            bool ok = false;
            for (size_t i = 0; i < app.users.size(); ++i)
            {
                if (app.users[i].username == app.loginName && app.users[i].password == app.loginPassword)
                {
                    app.currentUser = app.users[i];
                    app.loggedIn = true;
                    app.currentAccessibleProjects = Storage::AccessibleProjects(app.sharedRoot, app.currentUser.username, app.currentUser.role);
                    if (!app.currentAccessibleProjects.empty())
                    {
                        app.currentProject = app.currentAccessibleProjects.front();
                        ReloadCurrentProjectTasks();
                    }
                    app.screen = (app.currentUser.role == "admin") ? ScreenType::Admin : ScreenType::TaskList;
                    app.message = "ログインしました。";
                    ok = true;
                    break;
                }
            }
            if (!ok)
            {
                app.error = "ユーザ名またはパスワードが違います。";
            }
        }
        if (ImGui::Button("新規登録", ImVec2(160, 32)))
        {
            ClearBanner();
            app.screen = ScreenType::Register;
        }
        ImGui::End();
    }

    void RenderRegisterScreen()
    {
        AppState& app = GetAppState();
        ImGui::Begin("新規登録");
        ShowBanner();
        ReloadProjectsAndUsers();

        std::vector<std::string> memberNames = Storage::CollectAllMemberNames(app.sharedRoot);
        if (memberNames.empty()) memberNames.push_back("(メンバなし)");
        std::vector<const char*> items;
        for (size_t i = 0; i < memberNames.size(); ++i) items.push_back(memberNames[i].c_str());

        ImGui::Combo("権限", &app.registerRoleIndex, kRoleItems, IM_ARRAYSIZE(kRoleItems));
        ImGui::BeginDisabled(app.registerRoleIndex == 0);
        ImGui::Combo("ユーザ名(メンバ名)", &app.registerUserIndex, items.data(), static_cast<int>(items.size()));
        ImGui::EndDisabled();
        ImGui::InputText("パスワード", app.registerPassword1, IM_ARRAYSIZE(app.registerPassword1), ImGuiInputTextFlags_Password);
        ImGui::InputText("パスワード再入力", app.registerPassword2, IM_ARRAYSIZE(app.registerPassword2), ImGuiInputTextFlags_Password);

        if (ImGui::Button("登録", ImVec2(160, 32)))
        {
            ClearBanner();
            ReloadProjectsAndUsers();

            UserAccount user;
            user.role = kRoleItems[app.registerRoleIndex];
            user.username = (user.role == "admin") ? "admin" : memberNames[std::clamp(app.registerUserIndex, 0, static_cast<int>(memberNames.size()) - 1)];
            user.password = app.registerPassword1;

            if (user.role == "admin")
            {
                app.error = "admin は CLI 初期設定でのみ登録できます。";
            }
            else if (std::strcmp(app.registerPassword1, app.registerPassword2) != 0 || std::strlen(app.registerPassword1) == 0)
            {
                app.error = "パスワードが一致しないか空です。";
            }
            else if (Storage::UserExists(app.users, user.username))
            {
                app.error = "同名ユーザは登録済みです。";
            }
            else if (!Storage::AppendUser(app.sharedRoot, user))
            {
                app.error = "userData.txt への追記に失敗しました。";
            }
            else
            {
                app.message = "新規登録が完了しました。ログインしてください。";
                std::memset(app.registerPassword1, 0, sizeof(app.registerPassword1));
                std::memset(app.registerPassword2, 0, sizeof(app.registerPassword2));
                app.screen = ScreenType::Login;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("戻る", ImVec2(160, 32)))
        {
            ClearBanner();
            app.screen = ScreenType::Login;
        }
        ImGui::End();
    }

    void RenderAdminScreen()
    {
        AppState& app = GetAppState();
        ReloadProjectsAndUsers();
        ImGui::Begin("管理者画面");
        ShowBanner();
        ImGui::Text("共有フォルダ: %s", app.sharedRoot.c_str());
        ImGui::Separator();

        ImGui::InputText("新規プロジェクト名", app.newProjectName, IM_ARRAYSIZE(app.newProjectName));
        if (ImGui::Button("プロジェクト作成", ImVec2(180, 32)))
        {
            ClearBanner();
            std::string error;
            if (std::strlen(app.newProjectName) == 0)
            {
                app.error = "プロジェクト名を入力してください。";
            }
            else if (!Storage::CreateProject(app.sharedRoot, app.newProjectName, error))
            {
                app.error = error;
            }
            else
            {
                app.message = "プロジェクトを作成しました。";
                ReloadProjectsAndUsers();
                app.currentAccessibleProjects = Storage::AccessibleProjects(app.sharedRoot, app.currentUser.username, app.currentUser.role);
            }
        }

        ImGui::Separator();
        if (!app.projects.empty())
        {
            std::vector<const char*> projectItems;
            for (size_t i = 0; i < app.projects.size(); ++i) projectItems.push_back(app.projects[i].name.c_str());
            app.memberProjectIndex = std::clamp(app.memberProjectIndex, 0, static_cast<int>(app.projects.size()) - 1);
            ImGui::Combo("メンバ編集対象", &app.memberProjectIndex, projectItems.data(), static_cast<int>(projectItems.size()));

            ProjectInfo& p = app.projects[app.memberProjectIndex];
            ImGui::Text("選択中プロジェクト: %s", p.name.c_str());
            ImGui::InputText("メンバ名", app.memberNameInput, IM_ARRAYSIZE(app.memberNameInput));
            if (ImGui::Button("メンバ追加", ImVec2(150, 30)))
            {
                ClearBanner();
                std::vector<std::string> members = p.members;
                members.push_back(app.memberNameInput);
                if (Storage::SaveProjectMembers(app.sharedRoot, p.name, members))
                {
                    app.message = "メンバを追加しました。";
                    ReloadProjectsAndUsers();
                }
                else
                {
                    app.error = "projectMemberData.txt 更新に失敗しました。";
                }
            }
            ImGui::Separator();
            ImGui::Text("メンバ一覧");
            for (size_t i = 0; i < p.members.size(); ++i)
            {
                ImGui::BulletText("%s", p.members[i].c_str());
                ImGui::SameLine(300);
                std::string btn = "削除##member" + std::to_string(i);
                if (ImGui::SmallButton(btn.c_str()))
                {
                    ClearBanner();
                    std::vector<std::string> members = p.members;
                    members.erase(members.begin() + i);
                    if (Storage::SaveProjectMembers(app.sharedRoot, p.name, members))
                    {
                        app.message = "メンバを削除しました。";
                        ReloadProjectsAndUsers();
                    }
                    else
                    {
                        app.error = "メンバ削除に失敗しました。";
                    }
                    break;
                }
            }
        }

        ImGui::Separator();
        if (ImGui::Button("タスク一覧へ", ImVec2(180, 32)))
        {
            app.currentAccessibleProjects = Storage::AccessibleProjects(app.sharedRoot, app.currentUser.username, app.currentUser.role);
            if (!app.currentAccessibleProjects.empty())
            {
                app.currentProject = app.currentAccessibleProjects.front();
                ReloadCurrentProjectTasks();
                app.screen = ScreenType::TaskList;
            }
            else
            {
                app.error = "アクセス可能なプロジェクトがありません。";
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("ログアウト", ImVec2(180, 32)))
        {
            app.loggedIn = false;
            app.screen = ScreenType::Login;
            app.message = "ログアウトしました。";
        }
        ImGui::End();
    }

    void RenderTaskListScreen()
    {
        AppState& app = GetAppState();
        ImGui::Begin("タスク一覧");
        ShowBanner();
        ImGui::Text("ユーザ: %s / 権限: %s / プロジェクト: %s / version=%d",
                    app.currentUser.username.c_str(),
                    app.currentUser.role.c_str(),
                    app.currentProject.c_str(),
                    app.currentTaskFileVersion);

        if (ImGui::Button("登録", ImVec2(90, 30)))
        {
            ClearBanner();
            app.selectedTaskIndex = -1;
            ResetTaskEditorFromSelected(true);
            app.screen = ScreenType::TaskEdit;
        }
        ImGui::SameLine();
        if (ImGui::Button("削除", ImVec2(90, 30)))
        {
            ClearBanner();
            if (app.selectedTaskIndex < 0 || app.selectedTaskIndex >= static_cast<int>(app.currentTasks.size()))
            {
                app.error = "削除対象のタスクを選択してください。";
            }
            else if (!CurrentUserCanDeleteSelected())
            {
                app.error = "このタスクを削除する権限がありません。";
            }
            else
            {
                std::vector<TaskItem> next = app.currentTasks;
                next.erase(next.begin() + app.selectedTaskIndex);
                SaveTaskList(next);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("再読込", ImVec2(90, 30)))
        {
            ClearBanner();
            ReloadCurrentProjectTasks();
        }
        ImGui::SameLine();
        if (ImGui::Button("ガントチャート", ImVec2(130, 30)))
        {
            app.showGantt = !app.showGantt;
        }

        ImGui::SameLine(700);
        if (ImGui::Button("プロジェクト切替", ImVec2(150, 30)))
        {
            app.screen = ScreenType::ProjectSwitch;
        }

        std::vector<std::string> members = CurrentProjectMembers();
        std::vector<const char*> filterItems;
        filterItems.push_back("(全員)");
        for (size_t i = 0; i < members.size(); ++i) filterItems.push_back(members[i].c_str());
        static int filterIndex = 0;
        ImGui::Combo("担当者絞り込み", &filterIndex, filterItems.data(), static_cast<int>(filterItems.size()));
        app.filterAssignee = (filterIndex == 0) ? "" : members[filterIndex - 1];

        ImGui::Separator();
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

            for (size_t i = 0; i < app.currentTasks.size(); ++i)
            {
                const TaskItem& t = app.currentTasks[i];
                if (!app.filterAssignee.empty() && t.assignee != app.filterAssignee) continue;
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::Text("%d", t.id);
                ImGui::TableSetColumnIndex(1);
                std::string label = t.name + "##task" + std::to_string(i);
                if (ImGui::Selectable(label.c_str(), app.selectedTaskIndex == static_cast<int>(i), ImGuiSelectableFlags_SpanAllColumns))
                {
                    app.selectedTaskIndex = static_cast<int>(i);
                    app.screen = ScreenType::TaskDetail;
                }
                ImGui::TableSetColumnIndex(2); ImGui::TextUnformatted(t.category.c_str());
                ImGui::TableSetColumnIndex(3); ImGui::TextUnformatted(t.assignee.c_str());
                ImGui::TableSetColumnIndex(4); ImGui::TextUnformatted(t.effort.c_str());
                ImGui::TableSetColumnIndex(5); ImGui::TextUnformatted(t.startDate.c_str());
                ImGui::TableSetColumnIndex(6); ImGui::TextUnformatted(t.endDate.c_str());
                ImGui::TableSetColumnIndex(7); ImGui::Text("%d%%", t.progress);
                ImGui::TableSetColumnIndex(8); ImGui::TextUnformatted(t.priority.c_str());
                ImGui::TableSetColumnIndex(9); ImGui::TextUnformatted(t.status.c_str());
            }
            ImGui::EndTable();
        }

        if (app.showGantt)
        {
            ImGui::Separator();
            ImGui::Text("ガントチャート");
            ImGui::BeginChild("gantt_child", ImVec2(0, 260), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImDrawList* draw = ImGui::GetWindowDrawList();
            ImVec2 origin = ImGui::GetCursorScreenPos();

            if (!app.currentTasks.empty())
            {
                SimpleDate minD, maxD;
                bool inited = false;
                for (size_t i = 0; i < app.currentTasks.size(); ++i)
                {
                    SimpleDate s, e;
                    if (!ParseDate(app.currentTasks[i].startDate, s) || !ParseDate(app.currentTasks[i].endDate, e)) continue;
                    if (!inited || DaysBetween(s, minD) < 0) minD = s;
                    if (!inited || DaysBetween(maxD, e) < 0) maxD = e;
                    inited = true;
                }
                if (!inited)
                {
                    minD = {2026, 3, 1};
                    maxD = {2026, 4, 30};
                }

                const float dayWidth = 24.0f;
                const float rowHeight = 28.0f;
                const float leftWidth = 220.0f;
                const float headerHeight = 52.0f;
                int totalDays = std::max(1, DaysBetween(minD, maxD) + 1);
                float width = leftWidth + totalDays * dayWidth;
                float height = headerHeight + rowHeight * app.currentTasks.size();

                draw->AddRectFilled(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(255, 255, 255, 255));
                draw->AddRect(origin, ImVec2(origin.x + width, origin.y + height), IM_COL32(180, 180, 180, 255));

                for (int d = 0; d < totalDays; ++d)
                {
                    float x = origin.x + leftWidth + d * dayWidth;
                    SimpleDate cur = AddDays(minD, d);
                    draw->AddLine(ImVec2(x, origin.y), ImVec2(x, origin.y + height), IM_COL32(220, 220, 220, 255));
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "%d/%d", cur.month, cur.day);
                    draw->AddText(ImVec2(x + 2, origin.y + 4), IM_COL32(0, 0, 0, 255), buf);
                }

                for (size_t i = 0; i < app.currentTasks.size(); ++i)
                {
                    const TaskItem& t = app.currentTasks[i];
                    float y = origin.y + headerHeight + i * rowHeight;
                    draw->AddLine(ImVec2(origin.x, y), ImVec2(origin.x + width, y), IM_COL32(220, 220, 220, 255));
                    draw->AddText(ImVec2(origin.x + 4, y + 5), IM_COL32(0, 0, 0, 255), t.name.c_str());

                    SimpleDate s, e;
                    if (!ParseDate(t.startDate, s) || !ParseDate(t.endDate, e)) continue;
                    int startOffset = DaysBetween(minD, s);
                    int endOffset = DaysBetween(minD, e);
                    float x1 = origin.x + leftWidth + startOffset * dayWidth;
                    float x2 = origin.x + leftWidth + (endOffset + 1) * dayWidth;
                    draw->AddRectFilled(ImVec2(x1, y + 4), ImVec2(x2, y + rowHeight - 4), IM_COL32(120, 170, 255, 255), 3.0f);
                    float pw = (x2 - x1) * (std::clamp(t.progress, 0, 100) / 100.0f);
                    draw->AddRectFilled(ImVec2(x1, y + 4), ImVec2(x1 + pw, y + rowHeight - 4), IM_COL32(80, 190, 110, 255), 3.0f);
                    draw->AddRect(ImVec2(x1, y + 4), ImVec2(x2, y + rowHeight - 4), IM_COL32(60, 60, 60, 255), 3.0f);
                }
                ImGui::Dummy(ImVec2(width, height));
            }
            ImGui::EndChild();
        }

        ImGui::End();
    }

    void RenderTaskDetailScreen()
    {
        AppState& app = GetAppState();
        ImGui::Begin("タスク詳細");
        ShowBanner();
        if (app.selectedTaskIndex < 0 || app.selectedTaskIndex >= static_cast<int>(app.currentTasks.size()))
        {
            ImGui::Text("タスクが選択されていません。");
        }
        else
        {
            const TaskItem& t = app.currentTasks[app.selectedTaskIndex];
            ImGui::Text("ID: %d", t.id);
            ImGui::Text("タスク名: %s", t.name.c_str());
            ImGui::Text("分類: %s", t.category.c_str());
            ImGui::Text("担当者: %s", t.assignee.c_str());
            ImGui::Text("工数: %s", t.effort.c_str());
            ImGui::Text("開始日: %s", t.startDate.c_str());
            ImGui::Text("終了日: %s", t.endDate.c_str());
            ImGui::Text("進捗: %d%%", t.progress);
            ImGui::Text("優先度: %s", t.priority.c_str());
            ImGui::Text("ステータス: %s", t.status.c_str());
            ImGui::Text("作成者: %s", t.creator.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("コメント: %s", t.comment.c_str());
        }

        if (ImGui::Button("編集", ImVec2(150, 32)))
        {
            ClearBanner();
            ResetTaskEditorFromSelected(false);
            app.screen = ScreenType::TaskEdit;
        }
        ImGui::SameLine();
        if (ImGui::Button("戻る", ImVec2(150, 32)))
        {
            app.screen = ScreenType::TaskList;
        }
        ImGui::End();
    }

    void RenderTaskEditScreen()
    {
        AppState& app = GetAppState();
        ImGui::Begin("タスク編集");
        ShowBanner();
        const bool fullEdit = app.createMode || CurrentUserCanEditAllFields();
        std::vector<std::string> members = CurrentProjectMembers();
        if (members.empty()) members.push_back(app.currentUser.username);
        std::vector<const char*> memberItems;
        for (size_t i = 0; i < members.size(); ++i) memberItems.push_back(members[i].c_str());

        ImGui::InputText("ID", app.editId, IM_ARRAYSIZE(app.editId), ImGuiInputTextFlags_ReadOnly);
        ImGui::BeginDisabled(!fullEdit);
        ImGui::InputText("タスク名", app.editName, IM_ARRAYSIZE(app.editName));
        ImGui::InputText("分類", app.editCategory, IM_ARRAYSIZE(app.editCategory));
        ImGui::Combo("担当者", &app.editAssigneeIndex, memberItems.data(), static_cast<int>(memberItems.size()));
        ImGui::InputText("工数", app.editEffort, IM_ARRAYSIZE(app.editEffort));
        ImGui::InputText("開始日", app.editStartDate, IM_ARRAYSIZE(app.editStartDate));
        ImGui::InputText("終了日", app.editEndDate, IM_ARRAYSIZE(app.editEndDate));
        ImGui::Combo("優先度", &app.editPriorityIndex, kPriorityItems, IM_ARRAYSIZE(kPriorityItems));
        ImGui::Combo("ステータス", &app.editStatusIndex, kStatusItems, IM_ARRAYSIZE(kStatusItems));
        ImGui::EndDisabled();

        ImGui::SliderInt("進捗", &app.editProgress, 0, 100);
        ImGui::InputTextMultiline("コメント", app.editComment, IM_ARRAYSIZE(app.editComment), ImVec2(-1, 120));

        if (ImGui::Button("保存", ImVec2(150, 32)))
        {
            ClearBanner();
            if (std::strlen(app.editName) == 0)
            {
                app.error = "タスク名を入力してください。";
            }
            else
            {
                std::vector<TaskItem> next = app.currentTasks;
                if (app.createMode)
                {
                    TaskItem t;
                    ApplyTaskFromEditor(t);
                    next.push_back(t);
                    SaveTaskList(next);
                    app.screen = ScreenType::TaskList;
                }
                else if (app.selectedTaskIndex >= 0 && app.selectedTaskIndex < static_cast<int>(next.size()))
                {
                    TaskItem old = next[app.selectedTaskIndex];
                    ApplyTaskFromEditor(next[app.selectedTaskIndex]);
                    if (!fullEdit)
                    {
                        next[app.selectedTaskIndex].name = old.name;
                        next[app.selectedTaskIndex].category = old.category;
                        next[app.selectedTaskIndex].assignee = old.assignee;
                        next[app.selectedTaskIndex].effort = old.effort;
                        next[app.selectedTaskIndex].startDate = old.startDate;
                        next[app.selectedTaskIndex].endDate = old.endDate;
                        next[app.selectedTaskIndex].priority = old.priority;
                        next[app.selectedTaskIndex].status = old.status;
                        next[app.selectedTaskIndex].creator = old.creator;
                    }
                    SaveTaskList(next);
                    app.screen = ScreenType::TaskList;
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル", ImVec2(150, 32)))
        {
            app.screen = (app.selectedTaskIndex >= 0) ? ScreenType::TaskDetail : ScreenType::TaskList;
        }
        ImGui::End();
    }

    void RenderProjectSwitchScreen()
    {
        AppState& app = GetAppState();
        ImGui::Begin("プロジェクト切り替え");
        ShowBanner();
        if (app.currentAccessibleProjects.empty())
        {
            ImGui::Text("切り替え可能なプロジェクトがありません。");
        }
        else
        {
            for (size_t i = 0; i < app.currentAccessibleProjects.size(); ++i)
            {
                const std::string& p = app.currentAccessibleProjects[i];
                if (ImGui::Button(p.c_str(), ImVec2(300, 32)))
                {
                    app.currentProject = p;
                    ReloadCurrentProjectTasks();
                    app.screen = ScreenType::TaskList;
                }
            }
        }
        if (ImGui::Button("戻る", ImVec2(150, 32)))
        {
            app.screen = ScreenType::TaskList;
        }
        ImGui::End();
    }
}

// カレンダーの表示
void RenderCalendar(int year, int month)
{
    // 月の最初の日を取得
    std::tm time_in = {};
    time_in.tm_year = year - 1900;
    time_in.tm_mon = month - 1;
    time_in.tm_mday = 1;
    std::mktime(&time_in);

    int start_wday = time_in.tm_wday; // 曜日 (0=日曜)

    // 月の日数
    int days_in_month = 31;
    if (month == 2) days_in_month = 28; // 簡略（うるう年未対応）

    ImGui::Begin("Calendar");

    if (ImGui::BeginTable("CalendarTable", 7))
    {
        const char* days[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

        // ヘッダ
        for (int i = 0; i < 7; i++) {
            ImGui::TableNextColumn();
            ImGui::Text("%s", days[i]);
        }

        int day = 1;

        for (int row = 0; row < 6; row++)
        {
            ImGui::TableNextRow();
            for (int col = 0; col < 7; col++)
            {
                ImGui::TableNextColumn();

                if (row == 0 && col < start_wday) {
                    ImGui::Text(" ");
                }
                else if (day <= days_in_month) {
                    if (ImGui::Button(std::to_string(day).c_str()))
                    {
                        // 日付クリック処理
                    }
                    day++;
                }
            }
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

void RenderUI()
{
    AppState& app = GetAppState();
    ImGui::SetNextWindowPos(ImVec2(40, 40), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(1200, 760), ImGuiCond_Once);

	if (ImGui::BeginMainMenuBar()) {

		if (ImGui::BeginMenu("File")) {
			ImGui::MenuItem("About");
			RenderCalendar(2026,4);
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help")) {
			ImGui::MenuItem("About");
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

    switch (app.screen)
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


