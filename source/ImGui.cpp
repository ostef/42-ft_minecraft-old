#define IMGUI_IMPLEMENTATION
#include "ImGui.hpp"

#include <backends/imgui_impl_glfw.cpp>
#include <backends/imgui_impl_opengl3.cpp>

namespace ImGuiExt
{
    bool IsItemDoubleClicked (ImGuiMouseButton mouse_button)
    {
        return ImGui::IsItemHovered () && ImGui::IsMouseDoubleClicked (mouse_button);
    }

    float HermiteCubicCalculate (float x0, float y0, float der0, float x1, float y1, float der1, float t)
    {
        float f8 = der0 * (x1 - x0) - (y1 - y0);
        float f9 = -der1 * (x1 - x0) + (y1 - y0);

        return ImLerp (y0, y1, t) + t * (1 - t) * ImLerp (f8, f9, t);
    }

    void AddHermiteCubic (ImDrawList *draw_list,
        const ImVec2 &offset, const ImVec2 &scale,
        float x0, float y0, float der0, float x1, float y1, float der1,
        ImU32 color, float thickness, int num_segments)
    {
        if ((color & IM_COL32_A_MASK) == 0)
            return;

        ImVec2 p0;
        p0.x = offset.x + x0 * scale.x;
        p0.y = offset.y + y0 * scale.y;

        draw_list->PathLineTo (p0);
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step += 1)
        {
            ImVec2 p;
            p.x = offset.x + ImLerp (x0, x1, t_step * i_step) * scale.x;
            p.y = offset.y + HermiteCubicCalculate (x0, y0, der0, x1, y1, der1, t_step * i_step) * scale.y;
            draw_list->_Path.push_back (p);
        }
        draw_list->PathStroke (color, 0, thickness);
    }

    void AddDashedLine (ImDrawList *draw_list, const ImVec2 &a, const ImVec2 &b, float line_len, float spacing, ImU32 color, float thickness)
    {
        float len = ImSqrt (ImLengthSqr (b - a));
        ImVec2 dir = (b - a) / len;

        float i = 0;
        while (i < len)
        {
            draw_list->AddLine (a + dir * i, a + dir * (i + line_len), color, thickness);
            i += line_len + spacing;
        }
    }

    void AddDottedLine (ImDrawList *draw_list, const ImVec2 &a, const ImVec2 &b, float spacing, ImU32 color, float thickness)
    {
        float len = ImSqrt (ImLengthSqr (b - a));
        ImVec2 dir = (b - a) / len;
        float radius = thickness / 2;

        float i = radius;
        while (i < len)
        {
            draw_list->AddCircleFilled (a + dir * i, radius, color);
            i += radius * 2 + spacing;
        }
    }

    static const float BezierSplineEditor_GrabRadius = 8;

    bool BezierSplinePoint (ImGuiID id, const ImRect &bounds, ImVec2 *point)
    {
        ImVec2 point_center = bounds.Min + ImVec2{point->x, 1 - point->y} * (bounds.Max - bounds.Min);

        ImRect box = ImRect{
            point_center - ImVec2{BezierSplineEditor_GrabRadius, BezierSplineEditor_GrabRadius},
            point_center + ImVec2{BezierSplineEditor_GrabRadius, BezierSplineEditor_GrabRadius}
        };
        bool hovered = false;
        bool held = false;

        ImGui::ItemAdd (box, id);
        ImGui::ButtonBehavior (box, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);

        if (hovered || held)
            ImGui::SetTooltip ("(%4.3f %4.3f)", point->x, point->y);

        if (held)
        {
            ImGui::SetHoveredID (id);

            point_center = ImGui::GetIO ().MousePos;

            point->x = (point_center.x - bounds.Min.x) / (bounds.Max.x - bounds.Min.x);
            point->y = 1 - (point_center.y - bounds.Min.y) / (bounds.Max.y - bounds.Min.y);
            if (ImGui::IsKeyDown (ImGuiKey_LeftCtrl))
            {
                point->x = roundf (point->x * 10) / 10;
                point->y = roundf (point->y * 10) / 10;
            }

            return true;
        }

        return false;
    }

    void BezierSplineInsertPoint (const ImVec2 &p, int *control_point_count, ImVec2 *control_points)
    {
        int insert_index = -1;
        int curve_count = (*control_point_count - 1) / 3;
        for (int i = curve_count - 1; i >= 0; i -= 1)
        {
            if (p.x >= control_points[i * 3].x)
            {
                insert_index = i + 1;
                break;
            }
        }

        insert_index = ImClamp (insert_index, 1, curve_count);
        insert_index *= 3;

        for (int i = *control_point_count + 2; i >= insert_index + 2; i -= 1)
            control_points[i] = control_points[i - 3];
        *control_point_count += 3;

        control_points[insert_index - 1] = p - ImVec2{0.1, 0};
        control_points[insert_index] = p;
        control_points[insert_index + 1] = p + ImVec2{0.1, 0};
    }

    void BezierSplineRemovePoint (int index, int *control_point_count, ImVec2 *control_points)
    {
        for (int i = index - 1; i < *control_point_count - 3; i += 1)
            control_points[i] = control_points[i + 3];
        *control_point_count -= 3;
    }

    bool BezierSplineEditor (const char *str_id, const ImVec2 &size, int max_control_points, int *control_point_count, ImVec2 *control_points)
    {
        assert (max_control_points >= 4);

        auto style = ImGui::GetStyle ();
        bool modified = false;

        // Ensure there is at least two points
        if (*control_point_count < 4)
        {
            control_points[0] = {0, 0};
            control_points[1] = {0, 0.7};
            control_points[2] = {0.3, 1};
            control_points[3] = {1, 1};
            *control_point_count = 4;

            modified = true;
        }

        ImGui::PushID (str_id);

        if (ImGui::Button ("Mirror X"))
        {
            for (int i = 0; i < *control_point_count / 2; i += 1)
            {
                auto p = control_points[i];
                p.x = 1 - p.x;

                control_points[i] = control_points[*control_point_count - i - 1];
                control_points[i].x = 1 - control_points[i].x;
                control_points[*control_point_count - i - 1] = p;
            }

            if (*control_point_count % 2 != 0)
                control_points[*control_point_count / 2].x = 1 - control_points[*control_point_count / 2].x;

            modified = true;
        }

        ImGui::SameLine ();

        if (ImGui::Button ("Mirror Y"))
        {
            for (int i = 0; i < *control_point_count; i += 1)
            {
                control_points[i].y = 1 - control_points[i].y;
            }
            modified = true;
        }

        if (!ImGui::BeginChild (str_id, size + style.WindowPadding * 2, true))
        {
            ImGui::EndChild ();
            ImGui::PopID ();
            return false;
        }

        auto draw_list = ImGui::GetWindowDrawList ();
        auto window = ImGui::GetCurrentWindow ();
        if (window->SkipItems)
            return false;

        ImRect bounds;
        bounds.Min = ImGui::GetCursorScreenPos ();
        bounds.Max = bounds.Min + size;

        // Draw Grid
        static const int GridSize = 10;
        int grid_cell_width  = (int)(size.x / GridSize);
        int grid_cell_height = (int)(size.y / GridSize);
        if (grid_cell_width > 0 && grid_cell_height > 0)
        {
            for (int i = grid_cell_width; i <= size.x - grid_cell_width; i += grid_cell_width)
                draw_list->AddLine (
                    ImVec2{bounds.Min.x + i, bounds.Min.y},
                    ImVec2{bounds.Min.x + i, bounds.Max.y},
                    ImGui::GetColorU32 (ImGuiCol_TextDisabled)
                );
            for (int i = grid_cell_height; i <= size.y - grid_cell_height; i += grid_cell_height)
                draw_list->AddLine (
                    ImVec2{bounds.Min.x, bounds.Min.y + i},
                    ImVec2{bounds.Max.x, bounds.Min.y + i},
                    ImGui::GetColorU32 (ImGuiCol_TextDisabled)
                );
        }

        int curve_count = ImGuiExt_BezierSpline_CurveCountFromPointCount (*control_point_count);

        // Move points
        int point_hovered = -1;
        int point_modified = -1;
        for (int i = 0; i < *control_point_count; i += 1)
        {
            bool first = i == 0;
            bool last = i == *control_point_count - 1;

            ImVec2 relative_prev_tang, relative_next_tang;
            if (i % 3 == 0)
            {
                if (!first)
                    relative_prev_tang = control_points[i - 1] - control_points[i];
                if (!last)
                    relative_next_tang = control_points[i + 1] - control_points[i];
            }

            if (BezierSplinePoint (window->GetID (i), bounds, &control_points[i]))
            {
                point_modified = i;
                modified = true;
            }

            if (i % 3 == 0 && !first && !last && ImGui::IsItemClicked (ImGuiMouseButton_Middle))
            {
                BezierSplineRemovePoint (i, control_point_count, control_points);
                modified = true;

                break;
            }

            if (ImGui::IsItemHovered ())
                point_hovered = i;

            if (point_modified == i)
            {
                if (i % 3 == 0)
                {
                    if (first)
                        control_points[i].x = 0;
                    else
                        control_points[i].x = ImMax (control_points[i].x, control_points[i - 3].x);

                    if (last)
                        control_points[i].x = 1;
                    else
                        control_points[i].x = ImMin (control_points[i].x, control_points[i + 3].x);

                    control_points[i].y = ImSaturate (control_points[i].y);

                    if (!first)
                        control_points[i - 1] = control_points[i] + relative_prev_tang;
                    if (!last)
                        control_points[i + 1] = control_points[i] + relative_next_tang;

                }
                else if (i % 3 == 1)
                {
                    if (!first && ImGui::IsKeyDown (ImGuiKey_LeftShift))
                    {
                        ImVec2 rel_tang = control_points[i] - control_points[i - 1];
                        control_points[i - 2] = control_points[i - 1] - rel_tang;
                    }
                }
                else if (i % 3 == 2)
                {
                    if (!last && ImGui::IsKeyDown (ImGuiKey_LeftShift))
                    {
                        ImVec2 rel_tang = control_points[i] - control_points[i + 1];
                        control_points[i + 2] = control_points[i + 1] - rel_tang;
                    }
                }

                break;
            }
        }

        // Insert new points
        if (point_hovered == -1 && ImGui::IsWindowHovered () && ImGui::IsMouseDoubleClicked (0))
        {
            if (*control_point_count + 3 <= max_control_points)
            {
                ImVec2 p = (ImGui::GetIO ().MousePos - bounds.Min);
                p.x = ImSaturate (p.x / size.x);
                p.y = 1 - ImSaturate (p.y / size.y);

                BezierSplineInsertPoint (p, control_point_count, control_points);
                modified = true;
            }
        }

        curve_count = ImGuiExt_BezierSpline_CurveCountFromPointCount (*control_point_count);

        // Draw lines
        for (int i = 0; i < curve_count; i += 1)
        {
            ImVec2 p0 = control_points[i * 3];
            p0 = bounds.Min + ImVec2{p0.x * size.x, (1 - p0.y) * size.y};
            ImVec2 t0 = control_points[i * 3 + 1];
            t0 = bounds.Min + ImVec2{t0.x * size.x, (1 - t0.y) * size.y};
            ImVec2 t1 = control_points[i * 3 + 2];
            t1 = bounds.Min + ImVec2{t1.x * size.x, (1 - t1.y) * size.y};
            ImVec2 p1 = control_points[i * 3 + 3];
            p1 = bounds.Min + ImVec2{p1.x * size.x, (1 - p1.y) * size.y};

            draw_list->AddLine (p0, t0, ImGui::GetColorU32 (ImGuiCol_Text));
            draw_list->AddLine (p1, t1, ImGui::GetColorU32 (ImGuiCol_Text));
            draw_list->AddBezierCubic (p0, t0, t1, p1, ImGui::GetColorU32 (ImGuiCol_Text), 2);
        }

        // Draw points
        for (int i = 0; i < *control_point_count; i += 1)
        {
            ImVec2 p = bounds.Min
                + ImVec2{control_points[i].x * size.x, (1 - control_points[i].y) * size.y};

            ImU32 color;
            ImU32 border_color;
            if (point_modified == i)
                color = ImGui::GetColorU32 (ImGuiCol_ButtonActive);
            else if (point_hovered == i)
                color = ImGui::GetColorU32 (ImGuiCol_ButtonHovered);
            else
                color = ImGui::GetColorU32 (ImGuiCol_Button);

            draw_list->AddCircleFilled (p, BezierSplineEditor_GrabRadius, color);
            if (style.FrameBorderSize > 0)
                draw_list->AddCircle (p, BezierSplineEditor_GrabRadius, ImGui::GetColorU32 (ImGuiCol_Border), 0, style.FrameBorderSize);
        }

        ImGui::EndChild ();

        ImGui::PopID ();

        return modified;
    }
}
