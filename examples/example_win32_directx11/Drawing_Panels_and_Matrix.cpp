#include "Drawing_Panels_and_Matrix.hpp"

using namespace std;


void DrawMergedMatrix(ImDrawList* draw_list, const ImVec2& origin, const vector<vector<int>>& matrix,
 vector<ProcInfo>& processes, float totalPhysMB, int which_matrix, bool show_elemetns, ID3D11Device* g_pd3dDevice, ImFont* fontmed)
{
    int rows = matrix.size();
    if (rows == 0) return;
    int cols = matrix[0].size();

    ImVec2 grid_max = ImVec2(origin.x + (cols * 6), origin.y + (rows * 5));
    draw_list->AddRectFilled(origin, grid_max, GetColorById(13));
    vector<vector<bool>> visited(rows, vector<bool>(cols, false));
    for (int j = 0; j < rows; j++)
    {
        for (int i = 0; i < cols; i++)
        {
            if (visited[j][i]) continue;
            int color_idx = matrix[j][i];
            if (color_idx == 13 || color_idx == -1) {
                visited[j][i] = true;
                continue;
            }
            if (color_idx - 1 < 0 || color_idx - 1 >= (int)processes.size()) {
                visited[j][i] = true;
                continue;
            }
            int width = 1;
            int height = 1;
            while (i + width < cols && matrix[j][i + width] == color_idx) {
                width++;
            }
            while (j + height < rows) {
                bool full_row_matches = true;
                for (int k = 0; k < width; ++k) {
                    if (matrix[j + height][i + k] != color_idx) {
                        full_row_matches = false;
                        break;
                    }
                }
                if (!full_row_matches) {
                    break;
                }

                height++;
            }
            for (int dy = 0; dy < height; dy++) {
                for (int dx = 0; dx < width; dx++) {
                    visited[j + dy][i + dx] = true;
                }
            }

            ImVec2 square_min = ImVec2(origin.x + (i * 6) + 4.0f, origin.y + (j * 5) + 4.0f); //top-left
            ImVec2 square_max = ImVec2(origin.x + ((i + width) * 6), origin.y + ((j + height) * 5)); //down-right
            ImVec2 square_min_icons = ImVec2(origin.x + (i), origin.y + (j));
            float size_x_box = square_max.x - square_min.x;
            float size_y_box = square_max.y - square_min.y;
            ImU32 color = GetColorById(color_idx % 13);
            draw_list->AddRectFilled(square_min, square_max, color, 2);
            if (show_elemetns)
            {
                ImVec2 icon_place = ImVec2(square_min.x + 0.15 * size_x_box, square_min.y + 0.1 * size_y_box);
                int pick_size = min((int)size_x_box, (int)size_y_box);
                pick_size = max(pick_size / 3, 32); //divide by 3 looks the best
                pick_size = max(1, pick_size); // watch out for 0
                ImVec2 text_place = ImVec2(square_min.x + 0.15 * size_x_box, square_min.y + (0.18 * size_y_box + pick_size / 3) + 1);
                ImVec2 text_place_ram = ImVec2(square_min.x + 0.15 * size_x_box, square_min.y + (0.26 * size_y_box + pick_size / 3) + 5);
                ImVec2 text_place_cpu = ImVec2(square_min.x + 0.15 * size_x_box, square_min.y + (0.34 * size_y_box + pick_size / 3) + 6);
                proces_icon_test(matrix[j][i] - 1, processes, icon_place, pick_size / 2, pick_size / 2, g_pd3dDevice); //matrix[j][i]-1 due to indexing starting from 0


                int font_size = pick_size / 3; // looks the best 
                int how_much_to_cut = size_x_box / (font_size / 1.5); // dividing by 1.5 due to choosen font

                string app_name_string(processes[matrix[j][i] - 1].name.begin(), processes[matrix[j][i] - 1].name.end());
                app_name_string = app_name_string.substr(0, min((size_t)how_much_to_cut, app_name_string.length()));


                stringstream stream_RAM;
                stream_RAM << fixed << setprecision(2) << ((double)processes[matrix[j][i] - 1].mb / totalPhysMB) * 100.0 << " % RAM";
                string app_size_RAM_string = stream_RAM.str();
                app_size_RAM_string = app_size_RAM_string.substr(0, min((size_t)how_much_to_cut, app_size_RAM_string.length()));

                stringstream stream_CPU;
                stream_CPU << fixed << setprecision(2) << ((double)processes[matrix[j][i] - 1].cpu_usage) << " % CPU";
                string app_size_CPU_string = stream_CPU.str();
                app_size_CPU_string = app_size_CPU_string.substr(0, min((size_t)how_much_to_cut, app_size_CPU_string.length()));

                const char* app_name_charptr_CPU = app_size_CPU_string.c_str();
                const char* app_name_charptr_RAM = app_size_RAM_string.c_str();
                const char* app_name_charptr = app_name_string.c_str();


                draw_list->AddText(fontmed, font_size, text_place, GetColorById(14), app_name_charptr);
                if (size_y_box > 40)
                    draw_list->AddText(fontmed, font_size, text_place_ram, GetColorById(14), app_name_charptr_RAM);
                if (size_y_box > 80)
                    draw_list->AddText(fontmed, font_size, text_place_cpu, GetColorById(14), app_name_charptr_CPU);

                ImGui::SetCursorScreenPos(square_min);
                ImGui::PushID(i * 10000 + j + which_matrix);

                bool clicked = ImGui::InvisibleButton("##treemap_cell", ImVec2(size_x_box, size_y_box));
                bool hovered = ImGui::IsItemHovered();
                ImGui::PopID();

                if (clicked)
                {
                    selected_process = matrix[j][i] - 1;
                    cout << "ID" << i * 10000 + j << endl;
                }
                if (hovered)
                {
                    ImGui::SetTooltip("%s", WStringToString(processes[matrix[j][i] - 1].name).c_str());
                    draw_list->AddRect(square_min, square_max, GetColorById(14), 2, 0, 4.0f);
                }
            }

        }
    }
}

//Drawing of the side Panel to see processes info
void DrawSidePanel(vector<ProcInfo>& processes, float totalPhysMB, ImVec2 origin, ImDrawList* draw_list, ID3D11Device* g_pd3dDevice)
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.25f, 0.25f, 0.25f, 1.0f));
    float panel_width = 250.0f;
    float size_of_close_process_x = 120.0f;
    float size_of_close_process_y = 30.0f;
    ImVec2 widget_pos = ImVec2(origin.x + 610, origin.y + 12);
    ImGui::SetNextWindowPos(widget_pos);
    ImGui::SetNextWindowSize(ImVec2(180, 648));

    ImGui::BeginChild("##side_panel", ImVec2(250, 0), true);

    if (selected_process >= 0 && selected_process < (int)processes.size())
    {

        ProcInfo& p = processes[selected_process];
        int index = (int)selected_process;
        ImVec2 icon_pos = ImVec2(widget_pos.x + 5, widget_pos.y+10);
        proces_icon_test(index, processes, icon_pos, 35, 35, g_pd3dDevice);
        ImGui::Text("\n");
        ImGui::Text("\n");
        ImGui::Text("\n");
        ImGui::Text("Proces: %s", WStringToString(p.name).c_str());
        ImGui::Separator();
        ImGui::Text("RAM: %.2f %%", ((double)p.mb / totalPhysMB) * 100.0);
        ImGui::Text("\n");
        ImGui::Text("\n");
        ImGui::Text("\n");
        ImGui::Text("CPU: %.2f %%", p.cpu_usage);
        UpdateAndDrawUI_RAM_process(draw_list, widget_pos, p);
        UpdateAndDrawUI_CPU_process(draw_list, widget_pos, p);
        //Close Panel drawing
        ImGui::SetCursorPos(ImVec2(5, 610));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));       
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        if (ImGui::Button("Close process", ImVec2(size_of_close_process_x, size_of_close_process_y)))
        {
            kill_process(p.PID);
            cout << "Close finished" << endl;
        }
        ImGui::PopStyleVar(1);
        ImGui::PopStyleColor(4);

    }
    else
    {
        ImGui::TextDisabled("Click to see detatils");
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();
}
//Wstring to string conversion
    string WStringToString(const wstring& wstr) {
    using convert_type = codecvt_utf8<wchar_t>;
    wstring_convert<convert_type, wchar_t> converter;
    return converter.to_bytes(wstr);
}

bool kill_process(DWORD PID)
{
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, PID);
    if (hProcess == NULL) {
        cerr << "Failed to open process. Error code: " << GetLastError() << endl;
        return false;
    }
    //exit code 0 
    BOOL result = TerminateProcess(hProcess, 0);
    if (!result) {
        cerr << "Failed to terminate process. Error code: " << GetLastError() <<  endl;
    }
    else {
        cout << "Successfully terminated process ID: " << PID << endl;
    }
    CloseHandle(hProcess);
    return result != 0;
}
