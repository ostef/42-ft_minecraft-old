// This file is the source file where all the ImGuiExt non-standalone
// functions, that depend on Minecraft things, are implemented

#include "Minecraft.hpp"

#include <imgui_internal.h>

namespace ImGuiExt
{

static const f32 NestedHermiteSplineEditor_GrabRadius = 8;

void AddHermiteCubic (ImDrawList *draw_list,
    const ImVec2 &offset, const ImVec2 &scale,
    f32 x0, f32 y0, f32 der0, f32 x1, f32 y1, f32 der1,
    ImU32 color, float thickness, int num_segments)
{
    if ((color & IM_COL32_A_MASK) == 0)
        return;

    ImVec2 p0;
    p0.x = offset.x + x0 * scale.x;
    p0.y = offset.y + y0 * scale.y;

    draw_list->PathLineTo (p0);
    f32 t_step = 1.0f / cast (f32) num_segments;
    for (int i_step = 1; i_step <= num_segments; i_step += 1)
    {
        ImVec2 p;
        p.x = offset.x + lerp (x0, x1, t_step * i_step) * scale.x;
        p.y = offset.y + hermite_cubic_calculate (x0, y0, der0, x1, y1, der1, lerp (x0, x1, t_step * i_step)) * scale.y;
        draw_list->_Path.push_back (p);
    }
    draw_list->PathStroke (color, 0, thickness);
}

bool NestedHermiteSplineMovePoint (int index, const ImRect &bounds, f32 *x, f32 *y, bool move_x, bool move_y)
{
    ImVec2 point_center = bounds.Min + ImVec2{*x, 1 - *y} * (bounds.Max - bounds.Min);

    ImRect box = ImRect{
        point_center - ImVec2{NestedHermiteSplineEditor_GrabRadius, NestedHermiteSplineEditor_GrabRadius},
        point_center + ImVec2{NestedHermiteSplineEditor_GrabRadius, NestedHermiteSplineEditor_GrabRadius}
    };

    bool hovered = false;
    bool held = false;

    ImGuiID id = ImGui::GetCurrentWindow ()->GetID (index);
    ImGui::ItemAdd (box, id);
    ImGui::ButtonBehavior (box, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);

    if (hovered || held)
        ImGui::SetTooltip ("(%4.3f %4.3f)", *x, *y);

    if (held)
    {
        ImGui::SetHoveredID (id);

        point_center = ImGui::GetIO ().MousePos;

        if (move_x)
            *x = (point_center.x - bounds.Min.x) / (bounds.Max.x - bounds.Min.x);
        if (move_y)
            *y = 1 - (point_center.y - bounds.Min.y) / (bounds.Max.y - bounds.Min.y);

        if (ImGui::IsKeyDown (ImGuiKey_LeftCtrl))
        {
            if (move_x)
                *x = roundf (*x * 10) / 10;
            if (move_y)
                *y = roundf (*y * 10) / 10;
        }

        *x = clamp (*x, 0.0f, 1.0f);
        *y = clamp (*y, 0.0f, 1.0f);

        return true;
    }

    return false;
}

bool NestedHermiteSplineEditor (const char *str_id, const ImVec2 &size, NestedHermiteSplineEditorData *data, const Slice<f32> &t_values,
    const char *zero_separated_t_value_names)
{
    auto style = ImGui::GetStyle ();

    ImGui::PushID (str_id);
    defer (ImGui::PopID ());

    if (data->spline_stack.count == 0)
        array_push (&data->spline_stack, data->root_spline);
    if (data->root_spline->knots.count == 0)
        data->selected_knot = -1;

    ImGui::BeginDisabled (data->index_in_stack == 0);
    if (ImGui::Button ("<"))
    {
        data->index_in_stack -= 1;
        data->selected_knot = -1;
    }
    ImGui::EndDisabled ();

    ImGui::SameLine ();

    ImGui::BeginDisabled (data->index_in_stack >= data->spline_stack.count - 1);
    if (ImGui::Button (">"))
    {
        data->index_in_stack += 1;
        data->selected_knot = -1;
    }
    ImGui::EndDisabled ();

    ImGui::SameLine ();

    auto spline = data->spline_stack[data->index_in_stack];

    ImGui::BeginDisabled (data->selected_knot == -1 || !spline->knots[data->selected_knot].is_nested_spline);
    if (ImGui::Button ("-"))
    {
        mem_free (spline->knots[data->selected_knot].spline, heap_allocator ());
        spline->knots[data->selected_knot].is_nested_spline = false;
    }
    ImGui::EndDisabled ();

    ImGui::SameLine ();

    ImGui::BeginDisabled (data->selected_knot == -1 || spline->knots[data->selected_knot].is_nested_spline);
    if (ImGui::Button ("+"))
    {
        spline->knots[data->selected_knot].is_nested_spline = true;
        spline->knots[data->selected_knot].spline = mem_alloc_typed (Nested_Hermite_Spline, 1, heap_allocator ());
    }
    ImGui::EndDisabled ();

    if (!zero_separated_t_value_names)
        ImGui::SliderInt ("T value", &spline->t_value_index, 0, t_values.count - 1);
    else
        ImGui::Combo ("T value", &spline->t_value_index, zero_separated_t_value_names);

    if (!ImGui::BeginChild ("spline-window", size + style.WindowPadding * 2, true))
    {
        ImGui::EndChild ();
        return false;
    }
    defer (ImGui::EndChild ());

    auto window = ImGui::GetCurrentWindow ();
    if (window->SkipItems)
        return false;

    ImRect bounds;
    bounds.Min = ImGui::GetCursorScreenPos ();
    bounds.Max = bounds.Min + size;

    ImGui::PushID (spline);

    bool modified = false;
    int modified_knot = -1;
    int hovered_knot  = -1;
    for_array (i, spline->knots)
    {
        auto *knot = &spline->knots[i];
        if (knot->is_nested_spline)
        {
            f32 y_value = hermite_cubic_calculate (knot->spline, t_values);
            if (NestedHermiteSplineMovePoint (i, bounds, &knot->x, &y_value, true, false))
            {
                modified_knot = i;
                data->selected_knot = i;
            }
        }
        else
        {
            if (NestedHermiteSplineMovePoint (i, bounds, &knot->x, &knot->y, true, true))
            {
                modified_knot = i;
                data->selected_knot = i;
            }
        }

        if (i != 0)
            knot->x = max (knot->x, spline->knots[i - 1].x);
        if (i != spline->knots.count - 1)
            knot->x = min (knot->x, spline->knots[i + 1].x);

        if (ImGui::IsItemHovered ())
            hovered_knot = i;

        if (ImGui::IsItemClicked (ImGuiMouseButton_Middle))
        {
            array_ordered_remove (&spline->knots, i);
            modified_knot = i;
            if (data->selected_knot > i)
                data->selected_knot -= 1;
            else if (data->selected_knot == i)
                data->selected_knot = -1;
        }
        else if (ImGui::IsItemClicked (ImGuiMouseButton_Right))
        {
            if (knot->is_nested_spline)
            {
                while (data->index_in_stack != data->spline_stack.count - 1)
                    array_pop (&data->spline_stack);

                array_push (&data->spline_stack, knot->spline);
                data->index_in_stack += 1;
                data->selected_knot = -1;
            }

            modified_knot = i;
        }

        if (modified_knot == i)
            break;
    }

    ImGui::PopID ();

    if (modified_knot > -1)
        modified = true;

    // Insert knots
    if (hovered_knot == -1 && ImGui::IsWindowHovered () && ImGui::IsMouseDoubleClicked (ImGuiMouseButton_Left))
    {
        ImVec2 p = (ImGui::GetIO ().MousePos - bounds.Min);
        p.x = ImSaturate (p.x / size.x);
        p.y = 1 - ImSaturate (p.y / size.y);

        s64 index = -1;
        for (s64 i = spline->knots.count - 1; i >= 0; i -= 1)
        {
            if (p.x >= spline->knots[i].x)
            {
                index = i + 1;
                break;
            }
        }

        index = clamp (index, 0i64, spline->knots.count);
        auto knot = array_ordered_insert (&spline->knots, index, {});
        knot->x = p.x;
        knot->y = p.y;

        data->selected_knot = index;
        modified = true;
    }

    auto draw_list = ImGui::GetWindowDrawList ();
    spline = data->spline_stack[data->index_in_stack];

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

    // Draw lines
    for_range (i, 0, spline->knots.count - 1)
    {
        auto k0 = spline->knots[i];
        auto k1 = spline->knots[i + 1];

        AddHermiteCubic (draw_list,
            ImVec2{bounds.Min.x, bounds.Min.y + size.y}, ImVec2{size.x, -size.y},
            k0.x, hermite_knot_value (k0, t_values), k0.derivative,
            k1.x, hermite_knot_value (k1, t_values), k1.derivative,
            ImGui::GetColorU32 (ImGuiCol_Text),
            2
        );
    }

    // Draw derivative
    if (data->selected_knot != -1)
    {
        auto knot = spline->knots[data->selected_knot];
        ImVec2 p0 = {knot.x, hermite_knot_value (knot, t_values)};
        ImVec2 t0 = {p0.x + 0.1f, p0.y + 0.1f * knot.derivative};
        ImVec2 t1 = {p0.x - 0.1f, p0.y - 0.1f * knot.derivative};

        p0 = bounds.Min + ImVec2{p0.x * size.x, (1 - p0.y) * size.y};
        t0 = bounds.Min + ImVec2{t0.x * size.x, (1 - t0.y) * size.y};
        t1 = bounds.Min + ImVec2{t1.x * size.x, (1 - t1.y) * size.y};

        draw_list->AddLine (p0, t0, ImGui::GetColorU32 (ImGuiCol_Text, 0.7f));
        draw_list->AddLine (p0, t1, ImGui::GetColorU32 (ImGuiCol_Text, 0.7f));
    }

    // Draw points
    for_array (i, spline->knots)
    {
        ImVec2 p0 = {spline->knots[i].x, hermite_knot_value (spline->knots[i], t_values)};
        p0 = bounds.Min + ImVec2{p0.x * size.x, (1 - p0.y) * size.y};

        ImU32 color;
        ImU32 border_color;
        if (modified_knot == i || data->selected_knot == i)
            color = ImGui::GetColorU32 (ImGuiCol_ButtonActive);
        else if (hovered_knot == i)
            color = ImGui::GetColorU32 (ImGuiCol_ButtonHovered);
        else
            color = ImGui::GetColorU32 (ImGuiCol_Button);

        if (spline->knots[i].is_nested_spline)
        {
            auto size = ImVec2{NestedHermiteSplineEditor_GrabRadius, NestedHermiteSplineEditor_GrabRadius};
            draw_list->AddRectFilled (p0 - size, p0 + size, color);
            if (style.FrameBorderSize > 0)
                draw_list->AddRect (p0 - size, p0 + size, ImGui::GetColorU32 (ImGuiCol_Border), 0, 0, style.FrameBorderSize);
        }
        else
        {
            draw_list->AddCircleFilled (p0, NestedHermiteSplineEditor_GrabRadius, color);
            if (style.FrameBorderSize > 0)
                draw_list->AddCircle (p0, NestedHermiteSplineEditor_GrabRadius, ImGui::GetColorU32 (ImGuiCol_Border), 0, style.FrameBorderSize);
        }
    }

    // Draw calculated point based on T value
    {
        ImVec2 p;
        p.x = t_values[spline->t_value_index];
        p.y = hermite_cubic_calculate (spline, t_values);
        p = bounds.Min + ImVec2{p.x * size.x, (1 - p.y) * size.y};
        draw_list->AddCircleFilled (p, NestedHermiteSplineEditor_GrabRadius, ImGui::ColorConvertFloat4ToU32 (ImVec4{0.6, 0.4, 0.3, 0.8}));
    }

    return modified;
}

}
