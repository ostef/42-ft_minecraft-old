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
        float x0, float y0, float der0, float x1, float y1, float der1,
        ImU32 color, float thickness, int num_segments)
    {
        if ((color & IM_COL32_A_MASK) == 0)
            return;

        ImVec2 p0 = {x0, y0};

        draw_list->PathLineTo (p0);
        float t_step = 1.0f / (float)num_segments;
        for (int i_step = 1; i_step <= num_segments; i_step += 1)
        {
            ImVec2 p;
            p.x = ImLerp (x0, x1, t_step * i_step);
            p.y = HermiteCubicCalculate (x0, y0, der0, x1, y1, der1, t_step * i_step);
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

    bool BeginPanZoomView (const char *str_id, const ImVec2 &size, ImVec2 *offset, float *scale,
        bool border, PanZoomViewFlags flags, ImGuiWindowFlags window_flags, const PanZoomViewParams &extra)
    {
        if (!ImGui::BeginChild (str_id, size, border, window_flags))
            return false;

        bool y_down = (flags & PanZoomViewFlags_DownwardsYAxis) != 0;
        bool dragging = false;
        ImGuiMouseButton dragging_with_button = 0;
        if (ImGui::IsWindowHovered ())
        {
            if (flags & PanZoomViewFlags_PanOnLeftMouse)
            {
                dragging |= ImGui::IsMouseDragging (ImGuiMouseButton_Left);
                dragging_with_button = ImGuiMouseButton_Left;
            }

            if (!dragging && (flags & PanZoomViewFlags_PanOnMiddleMouse))
            {
                dragging |= ImGui::IsMouseDragging (ImGuiMouseButton_Middle);
                dragging_with_button = ImGuiMouseButton_Middle;
            }

            if (!dragging && (flags & PanZoomViewFlags_PanOnRightMouse))
            {
                dragging |= ImGui::IsMouseDragging (ImGuiMouseButton_Right);
                dragging_with_button = ImGuiMouseButton_Right;
            }

            if (flags & PanZoomViewFlags_PanOnSpace)
                dragging &= ImGui::IsKeyDown (ImGuiKey_Space);
        }

        ImRect bounds;
        bounds.Min = ImGui::GetWindowPos () + ImGui::GetStyle ().WindowPadding;
        bounds.Max = bounds.Min + size;

        ImVec2 mouse_pos = ImGui::GetMousePos () - bounds.Min;

        if (dragging)
        {
            ImVec2 delta = ImGui::GetMouseDragDelta (dragging_with_button);
            ImGui::ResetMouseDragDelta (dragging_with_button);

            offset->x -= delta.x / *scale;
            offset->y -= delta.y / *scale;

            *offset = ImClamp (*offset,
                {extra.XOffsetRange.x, extra.YOffsetRange.x},
                {extra.XOffsetRange.y, extra.YOffsetRange.y}
            );
        }

        ImVec2 mouse_before_zoom = mouse_pos;
        WindowToPanZoom (*offset, *scale, &mouse_before_zoom, NULL, y_down);

        if (ImGui::IsWindowHovered ())
        {
            auto wheel = ImGui::GetIO ().MouseWheel;
            if (wheel > 0)
                *scale *= 1.1f;
            if (wheel < 0)
                *scale *= 0.9f;
        }

        *scale = ImClamp (*scale, extra.ScaleRange.x, extra.ScaleRange.y);

        ImVec2 mouse_after_zoom = mouse_pos;
        WindowToPanZoom (*offset, *scale, &mouse_after_zoom, NULL, y_down);

        offset->x -= mouse_after_zoom.x - mouse_before_zoom.x;
        if (flags & PanZoomViewFlags_DownwardsYAxis)
            offset->y -= mouse_after_zoom.y - mouse_before_zoom.y;
        else
            offset->y += mouse_after_zoom.y - mouse_before_zoom.y;

        *offset = ImClamp (*offset,
            {extra.XOffsetRange.x, extra.YOffsetRange.x},
            {extra.XOffsetRange.y, extra.YOffsetRange.y}
        );

        auto draw_list = ImGui::GetWindowDrawList ();

        // Draw grid
        if (!(flags & PanZoomViewFlags_NoGrid))
        {
            static const int GridSize = 10;

            ImVec2 grid_cell_size = ImVec2{GridSize, GridSize};
            PanZoomToWindow (*offset, *scale, NULL, &grid_cell_size, y_down);

            if (grid_cell_size.x > 0 && grid_cell_size.y > 0)
            {
                for (float i = grid_cell_size.x; i <= size.x - grid_cell_size.x; i += grid_cell_size.x)
                {
                    ImVec2 pos = {i, 0.0f};
                    PanZoomToWindow (*offset, *scale, &pos, NULL, y_down);
                    pos = ImFloor (pos);

                    draw_list->AddLine (
                        ImVec2{bounds.Min.x + pos.x, bounds.Min.y},
                        ImVec2{bounds.Min.x + pos.x, bounds.Max.y},
                        ImGui::GetColorU32 (ImGuiCol_TextDisabled)
                    );
                }

                for (float i = grid_cell_size.y; i <= size.y - grid_cell_size.y; i += grid_cell_size.y)
                {
                    ImVec2 pos = {0.0f, i};
                    PanZoomToWindow (*offset, *scale, &pos, NULL, y_down);
                    pos = ImFloor (pos);

                    draw_list->AddLine (
                        ImVec2{bounds.Min.x, bounds.Min.y + pos.y},
                        ImVec2{bounds.Max.x, bounds.Min.y + pos.y},
                        ImGui::GetColorU32 (ImGuiCol_TextDisabled)
                    );
                }
            }
        }

        return true;
    }

    void EndPanZoomView ()
    {
        ImGui::EndChild ();
    }

    void PanZoomToWindow (const ImVec2 &offset, float scale, ImVec2 *inout_pos, ImVec2 *inout_size, bool y_down)
    {
        if (inout_pos)
        {
            inout_pos->x = (inout_pos->x - offset.x) * scale + ImGui::GetWindowWidth () * 0.5f;

            if (y_down)
                inout_pos->y = (inout_pos->y - offset.y) * scale + ImGui::GetWindowHeight () * 0.5f;
            else
                inout_pos->y = 1 - (inout_pos->y + offset.y) * scale + ImGui::GetWindowHeight () * 0.5f;
        }

        if (inout_size)
        {
            inout_size->x = inout_size->x * scale;
            inout_size->y = inout_size->y * scale;
        }
    }

    void WindowToPanZoom (const ImVec2 &offset, float scale, ImVec2 *inout_pos, ImVec2 *inout_size, bool y_down)
    {
        if (inout_pos)
        {
            inout_pos->x = (inout_pos->x - ImGui::GetWindowWidth () * 0.5f) / scale + offset.x;
            if (y_down)
                inout_pos->y = (inout_pos->y - ImGui::GetWindowHeight () * 0.5f) / scale + offset.y;
            else
                inout_pos->y = -(inout_pos->y - ImGui::GetWindowHeight () * 0.5f - 1) / scale - offset.y;
        }

        if (inout_size)
        {
            inout_size->x = inout_size->x / scale;
            inout_size->y = inout_size->y / scale;
        }
    }

    bool BeginHermiteSpline (const char *str_id, ImVec2 *offset, float *scale, const ImVec2 &size,
        bool border, HermiteSplineFlags flags, const HermiteSplineParams &extra)
    {
        auto storage = ImGui::GetStateStorage ();

        const char *wrapper_id;
        ImFormatStringToTempBuffer (&wrapper_id, NULL, "%s##Wrapper", str_id);
        ImGui::PushID (wrapper_id);

        bool opened;
        if (flags & HermiteSplineFlags_NoPanNoZoom)
            opened = ImGui::BeginChild (str_id, size, true);
        else
            opened = BeginPanZoomView (str_id, size, offset, scale,
                border, PanZoomViewFlags_NoGrid | PanZoomViewFlags_PanOnMiddleMouse, 0, extra.ViewParams);

        if (!opened)
            return false;

        ImRect bounds;
        bounds.Min = ImGui::GetWindowPos () + ImGui::GetStyle ().WindowPadding;
        bounds.Max = bounds.Min + size;

        auto draw_list = ImGui::GetWindowDrawList ();

        // Draw grid
        if (!(flags & HermiteSplineFlags_NoGrid))
        {
            ImVec2 min = ImVec2{
                extra.XRange.x,
                extra.YRange.x
            };
            PanZoomToWindow (*offset, *scale, &min, NULL, false);

            ImVec2 max = ImVec2{
                extra.XRange.y,
                extra.YRange.y
            };
            PanZoomToWindow (*offset, *scale, &max, NULL, false);

            ImVec2 min_by_grid = ImVec2{
                ImFloor (extra.XRange.x / extra.ViewParams.GridCellSize) * extra.ViewParams.GridCellSize,
                ImFloor (extra.YRange.y / extra.ViewParams.GridCellSize) * extra.ViewParams.GridCellSize
            };
            PanZoomToWindow (*offset, *scale, &min_by_grid, NULL, false);

            ImVec2 max_by_grid = ImVec2{
                ImFloor (extra.XRange.y / extra.ViewParams.GridCellSize) * extra.ViewParams.GridCellSize,
                ImFloor (extra.YRange.x / extra.ViewParams.GridCellSize) * extra.ViewParams.GridCellSize
            };
            PanZoomToWindow (*offset, *scale, &max_by_grid, NULL, false);

            float cell_size = extra.ViewParams.GridCellSize * *scale;

            if (cell_size > 0)
            {
                for (float i = min_by_grid.x; i <= max_by_grid.x + 0.001f; i += cell_size)
                {
                    draw_list->AddLine (
                        ImVec2{bounds.Min.x + ImFloor (i), bounds.Min.y + ImFloor (min.y)},
                        ImVec2{bounds.Min.x + ImFloor (i), bounds.Min.y + ImFloor (max.y)},
                        ImGui::GetColorU32 (ImGuiCol_TextDisabled)
                    );
                }

                for (float i = min_by_grid.y; i <= max_by_grid.y + 0.001f; i += cell_size)
                {
                    draw_list->AddLine (
                        ImVec2{bounds.Min.x + ImFloor (min.x), bounds.Min.y + ImFloor (i)},
                        ImVec2{bounds.Min.x + ImFloor (max.x), bounds.Min.y + ImFloor (i)},
                        ImGui::GetColorU32 (ImGuiCol_TextDisabled)
                    );
                }
            }

            ImVec2 origin = {};
            PanZoomToWindow (*offset, *scale, &origin, NULL, false);

            draw_list->AddLine (
                ImVec2{bounds.Min.x + ImFloor (origin.x), bounds.Min.y + ImFloor (min.y)},
                ImVec2{bounds.Min.x + ImFloor (origin.x), bounds.Min.y + ImFloor (max.y)},
                ImGui::GetColorU32 (ImGuiCol_TextDisabled),
                3
            );

            draw_list->AddLine (
                ImVec2{bounds.Min.x + ImFloor (min.x), bounds.Min.y + ImFloor (origin.y)},
                ImVec2{bounds.Min.x + ImFloor (max.x), bounds.Min.y + ImFloor (origin.y)},
                ImGui::GetColorU32 (ImGuiCol_TextDisabled),
                3
            );

            draw_list->AddRect (bounds.Min + min, bounds.Min + max, ImGui::GetColorU32 (ImGuiCol_TextDisabled), 0, 0, 3);
        }

        // Draw metrics
        if (!(flags & HermiteSplineFlags_NoMetricsOnXAxis))
        {
            float ref_width = ImGui::CalcTextSize ("-0.0").x * 1.5f;

            float metrics_incr = extra.ViewParams.GridCellSize;
            while (metrics_incr * *scale < ref_width)
                metrics_incr += extra.ViewParams.GridCellSize;

            float min_by_grid = ImFloor (extra.XRange.x / metrics_incr) * metrics_incr;
            float max_by_grid = ImFloor (extra.XRange.y / metrics_incr) * metrics_incr;

            for (float i = min_by_grid; i <= max_by_grid + 0.001f; i += metrics_incr)
            {
                const char *text = NULL;
                ImFormatStringToTempBuffer (&text, NULL, "%.1f", i);

                ImVec2 pos = {i, 0.0f};
                PanZoomToWindow (*offset, *scale, &pos, NULL, false);

                auto text_size = ImGui::CalcTextSize (text);
                pos.x += bounds.Min.x - text_size.x * 0.5;
                pos.y = bounds.Max.y - text_size.y - ImGui::GetStyle ().WindowPadding.y;

                draw_list->AddText (pos, ImGui::GetColorU32 (ImGuiCol_Text), text);
            }
        }

        if (!(flags & HermiteSplineFlags_NoMetricsOnYAxis))
        {
            float ref_height = ImGui::CalcTextSize ("-0.0").y * 1.5f;

            float metrics_incr = extra.ViewParams.GridCellSize;
            while (metrics_incr * *scale < ref_height)
                metrics_incr += extra.ViewParams.GridCellSize;

            float min_by_grid = ImFloor (extra.YRange.x / metrics_incr) * metrics_incr;
            float max_by_grid = ImFloor (extra.YRange.y / metrics_incr) * metrics_incr;

            for (float i = min_by_grid; i <= max_by_grid + 0.001f; i += metrics_incr)
            {
                const char *text = NULL;
                ImFormatStringToTempBuffer (&text, NULL, "%.1f", i);

                ImVec2 pos = {0.0f, i};
                PanZoomToWindow (*offset, *scale, &pos, NULL, false);

                auto text_size = ImGui::CalcTextSize (text);
                pos.x = bounds.Min.x;
                pos.y += bounds.Min.y - text_size.y * 0.5;

                draw_list->AddText (pos, ImGui::GetColorU32 (ImGuiCol_Text), text);
            }
        }

        return true;
    }

    void EndHermiteSpline ()
    {
        ImGui::EndChild ();
        ImGui::PopID ();
    }

    bool HermiteSplinePoint (ImGuiID id, const ImVec2 &offset, float scale, bool selected,
        HermiteSplinePointValues &point, const HermiteSplinePointValues &next, HermiteSplinePointFlags flags)
    {
        static const float GrabRadius = 8;

        auto window_pos = ImGui::GetWindowPos () + ImGui::GetStyle ().WindowPadding;

        auto original_loc = *point.location;
        auto original_val = *point.value;
        auto original_der = *point.derivative;

        auto point_center = ImVec2{*point.location, *point.value};
        PanZoomToWindow (offset, scale, &point_center, NULL, false);

        auto box = ImRect{
            window_pos + point_center - ImVec2{GrabRadius, GrabRadius},
            window_pos + point_center + ImVec2{GrabRadius, GrabRadius}
        };

        bool hovered, held;
        bool interact_with = !ImGui::IsAnyItemActive () || ImGui::GetActiveID () == id;

        ImGui::ItemAdd (box, id);
        selected |= ImGui::ButtonBehavior (box, id, &hovered, &held, ImGuiButtonFlags_NoNavFocus);
        interact_with &= selected;

        if (hovered || held)
            ImGui::SetTooltip ("(%4.3f %4.3f %4.3f)", *point.location, *point.value, *point.derivative);

        if (interact_with)
        {
            if (ImGui::IsKeyDown (ImGuiKey_Space))
            {
                auto new_point_center = ImGui::GetMousePos () - window_pos;
                WindowToPanZoom (offset, scale, &new_point_center, NULL, false);

                if (!(flags & HermiteSplinePointFlags_LockLocation))
                    *point.location = new_point_center.x;
                if (!(flags & HermiteSplinePointFlags_LockValue))
                    *point.value = new_point_center.y;

                ImGui::SetActiveID (id, ImGui::GetCurrentWindow ());
                ImGui::SetTooltip ("(%4.3f %4.3f %4.3f)", *point.location, *point.value, *point.derivative);
            }
            else if (ImGui::IsKeyDown (ImGuiKey_LeftShift))
            {
                auto mouse_pos = ImGui::GetMousePos () - window_pos;
                WindowToPanZoom (offset, scale, &mouse_pos, NULL, false);

                if (!(flags & HermiteSplinePointFlags_LockDerivative))
                {
                    float dx = mouse_pos.x - *point.location;
                    float dy = mouse_pos.y - *point.value;
                    if (ImAbs (dx) < 0.0001f)
                        *point.derivative = 0;
                    else
                        *point.derivative = -ImClamp (dy / dx, -100.0f, 100.0f);
                }

                ImGui::SetActiveID (id, ImGui::GetCurrentWindow ());
                ImGui::SetTooltip ("(%4.3f %4.3f %4.3f)", *point.location, *point.value, *point.derivative);
            }
        }

        auto draw_list = ImGui::GetWindowDrawList ();

        // Draw tangent
        if (selected)
        {
            static const float TangentLength = 2;

            ImVec2 p0 = ImVec2{original_loc, original_val};

            ImVec2 t0 = ImVec2{
                -1,
                original_der
            };

            ImVec2 t1 = ImVec2{
                1,
                -original_der
            };

            t0 *= TangentLength * 0.5f / ImSqrt (ImLengthSqr (t0));
            t0 += p0;

            t1 *= TangentLength * 0.5f / ImSqrt (ImLengthSqr (t1));
            t1 += p0;

            PanZoomToWindow (offset, scale, &p0, NULL, false);
            PanZoomToWindow (offset, scale, &t0, NULL, false);
            PanZoomToWindow (offset, scale, &t1, NULL, false);

            p0 += window_pos;
            t0 += window_pos;
            t1 += window_pos;

            AddDashedLine (draw_list, p0, t0, 4, 8, ImGui::GetColorU32 (ImGuiCol_Text, 0.7f), 2);
            AddDashedLine (draw_list, p0, t1, 4, 8, ImGui::GetColorU32 (ImGuiCol_Text, 0.7f), 2);
        }

        if (next.location && next.value && next.derivative)
        {
            auto p0 = ImVec2{original_loc, original_val};
            auto p1 = ImVec2{*next.location, *next.value};

            PanZoomToWindow (offset, scale, &p0, NULL, false);
            PanZoomToWindow (offset, scale, &p1, NULL, false);

            p0 += window_pos;
            p1 += window_pos;

            AddHermiteCubic (draw_list,
                p0.x, p0.y, original_der,
                p1.x, p1.y, *next.derivative,
                ImGui::GetColorU32 (ImGuiCol_Text),
                2
            );
        }

        ImU32 color;
        if (selected)
            color = ImGui::GetColorU32 (ImGuiCol_ButtonActive);
        else if (hovered)
            color = ImGui::GetColorU32 (ImGuiCol_ButtonHovered);
        else
            color = ImGui::GetColorU32 (ImGuiCol_Button);

        draw_list->AddCircleFilled (window_pos + point_center, GrabRadius, color);
        if (ImGui::GetStyle ().FrameBorderSize > 0)
            draw_list->AddCircle (window_pos + point_center, GrabRadius, ImGui::GetColorU32 (ImGuiCol_Border), 0, ImGui::GetStyle ().FrameBorderSize);

        return held;
    }
}
