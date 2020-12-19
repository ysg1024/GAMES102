#include "CanvasSystem.h"

#include "../Components/CanvasData.h"

#include <_deps/imgui/imgui.h>

using namespace Ubpa;

void ChaiKin(std::vector<Ubpa::pointf2> &points, bool closed);
void Interpolatory(std::vector<Ubpa::pointf2>& points, float alpha);//默认曲线封闭

void CanvasSystem::OnUpdate(Ubpa::UECS::Schedule& schedule) {
	schedule.RegisterCommand([](Ubpa::UECS::World* w) {
		auto data = w->entityMngr.GetSingleton<CanvasData>();
		if (!data)
			return;

		if (ImGui::Begin("Canvas")) {
			ImGui::Checkbox("Enable grid", &data->opt_enable_grid);
			ImGui::Checkbox("Enable context menu", &data->opt_enable_context_menu);
			ImGui::Text("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");

			ImGui::Checkbox("ChaiKin Subdivison", &data->ChaiKin);
			ImGui::SameLine();
			ImGui::Checkbox("Interpolatory Subdivison", &data->Interpolatory);
			ImGui::Checkbox("Closed", &data->closed);
			ImGui::SameLine();
			ImGui::Checkbox("TypePoints", &data->typepoint);
			ImGui::SliderInt("Times", &data->times, 0, 10);
			ImGui::SliderFloat("Alpha", &data->alpha, 0, 1);

			// Typically you would use a BeginChild()/EndChild() pair to benefit from a clipping region + own scrolling.
			// Here we demonstrate that this can be replaced by simple offsetting + custom drawing + PushClipRect/PopClipRect() calls.
			// To use a child window instead we could use, e.g:
			//      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));      // Disable padding
			//      ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(50, 50, 50, 255));  // Set a background color
			//      ImGui::BeginChild("canvas", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoMove);
			//      ImGui::PopStyleColor();
			//      ImGui::PopStyleVar();
			//      [...]
			//      ImGui::EndChild();

			// Using InvisibleButton() as a convenience 1) it will advance the layout cursor and 2) allows us to use IsItemHovered()/IsItemActive()
			ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
			ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
			if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
			if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
			ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);

			// Draw border and background color
			ImGuiIO& io = ImGui::GetIO();
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			draw_list->AddRectFilled(canvas_p0, canvas_p1, IM_COL32(50, 50, 50, 255));
			draw_list->AddRect(canvas_p0, canvas_p1, IM_COL32(255, 255, 255, 255));

			// This will catch our interactions
			ImGui::InvisibleButton("canvas", canvas_sz, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
			const bool is_hovered = ImGui::IsItemHovered(); // Hovered
			const bool is_active = ImGui::IsItemActive();   // Held
			const ImVec2 origin(canvas_p0.x + data->scrolling[0], canvas_p0.y + data->scrolling[1]); // Lock scrolled origin
			const pointf2 mouse_pos_in_canvas(io.MousePos.x - origin.x, io.MousePos.y - origin.y);

			// Add first and second point
			/*if (is_hovered && !data->adding_line && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				data->points.push_back(mouse_pos_in_canvas);
				data->points.push_back(mouse_pos_in_canvas);
				data->adding_line = true;
			}
			if (data->adding_line)
			{
				data->points.back() = mouse_pos_in_canvas;
				if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
					data->adding_line = false;
			}*/
			if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
				data->points.push_back(mouse_pos_in_canvas);
			}

			// Pan (we use a zero mouse threshold when there's no context menu)
			// You may decide to make that threshold dynamic based on whether the mouse is hovering something etc.
			const float mouse_threshold_for_pan = data->opt_enable_context_menu ? -1.0f : 0.0f;
			if (is_active && ImGui::IsMouseDragging(ImGuiMouseButton_Right, mouse_threshold_for_pan))
			{
				data->scrolling[0] += io.MouseDelta.x;
				data->scrolling[1] += io.MouseDelta.y;
			}

			// Context menu (under default mouse threshold)
			ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);
			if (data->opt_enable_context_menu && ImGui::IsMouseReleased(ImGuiMouseButton_Right) && drag_delta.x == 0.0f && drag_delta.y == 0.0f)
				ImGui::OpenPopupContextItem("context");
			if (ImGui::BeginPopup("context"))
			{
				if (data->adding_line)
					data->points.resize(data->points.size() - 2);
				data->adding_line = false;
				if (ImGui::MenuItem("Remove one", NULL, false, data->points.size() > 0)) { data->points.resize(data->points.size() - 2); }
				if (ImGui::MenuItem("Remove all", NULL, false, data->points.size() > 0)) { data->points.clear(); }
				ImGui::EndPopup();
			}

			// Draw grid + all lines in the canvas
			draw_list->PushClipRect(canvas_p0, canvas_p1, true);
			if (data->opt_enable_grid)
			{
				const float GRID_STEP = 64.0f;
				for (float x = fmodf(data->scrolling[0], GRID_STEP); x < canvas_sz.x; x += GRID_STEP)
					draw_list->AddLine(ImVec2(canvas_p0.x + x, canvas_p0.y), ImVec2(canvas_p0.x + x, canvas_p1.y), IM_COL32(200, 200, 200, 40));
				for (float y = fmodf(data->scrolling[1], GRID_STEP); y < canvas_sz.y; y += GRID_STEP)
					draw_list->AddLine(ImVec2(canvas_p0.x, canvas_p0.y + y), ImVec2(canvas_p1.x, canvas_p0.y + y), IM_COL32(200, 200, 200, 40));
			}
			/*for (int n = 0; n < data->points.size(); n += 2)
				draw_list->AddLine(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), ImVec2(origin.x + data->points[n + 1][0], origin.y + data->points[n + 1][1]), IM_COL32(255, 255, 0, 255), 2.0f);
				*/
			//原曲线
			if (data->typepoint) {
				for (int n = 0; n < data->points.size(); n += 1)
					draw_list->AddCircleFilled(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), 3.0f, IM_COL32(255, 255, 255, 255), 0);
			}
			if (data->points.size() >= 2) {
				for (int n = 0; n < data->points.size()-1; n += 1)
					draw_list->AddLine(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), ImVec2(origin.x + data->points[n + 1][0], origin.y + data->points[n + 1][1]), IM_COL32(255, 255, 0, 255), 2.0f);
				if (data->closed) {
					draw_list->AddLine(ImVec2(origin.x + data->points[data->points.size() - 1][0], origin.y + data->points[data->points.size() - 1][1]), ImVec2(origin.x + data->points[0][0], origin.y + data->points[0][1]), IM_COL32(255, 255, 0, 255), 2.0f);
				}
			}
			//细分曲线
			if (data->ChaiKin) {
				std::vector<Ubpa::pointf2> temp = data->points;
				for (int i = 0;i < data->times;i++) {
					ChaiKin(temp,data->closed);
				}
				for (int n = 0; n < temp.size() - 1; n += 1)
					draw_list->AddLine(ImVec2(origin.x + temp[n][0], origin.y + temp[n][1]), ImVec2(origin.x + temp[n + 1][0], origin.y + temp[n + 1][1]), IM_COL32(255, 0, 0, 255), 2.0f);
				if (data->closed) {
					draw_list->AddLine(ImVec2(origin.x + temp[temp.size() - 1][0], origin.y + temp[temp.size() - 1][1]), ImVec2(origin.x + temp[0][0], origin.y + temp[0][1]), IM_COL32(255, 0, 0, 255), 2.0f);
				}
				if (data->typepoint) {
					for (int n = 0; n < temp.size(); n += 1)
						draw_list->AddCircleFilled(ImVec2(origin.x + temp[n][0], origin.y + temp[n][1]), 3.0f, IM_COL32(255, 255, 255, 255), 0);
				}
			}
			if (data->Interpolatory) {
				std::vector<Ubpa::pointf2> temp = data->points;
				for (int i = 0;i < data->times;i++) {
					Interpolatory(temp, data->alpha);
				}
				for (int n = 0; n < temp.size() - 1; n += 1)
					draw_list->AddLine(ImVec2(origin.x + temp[n][0], origin.y + temp[n][1]), ImVec2(origin.x + temp[n + 1][0], origin.y + temp[n + 1][1]), IM_COL32(255, 0, 0, 255), 2.0f);
				if (1) {
					draw_list->AddLine(ImVec2(origin.x + temp[temp.size() - 1][0], origin.y + temp[temp.size() - 1][1]), ImVec2(origin.x + temp[0][0], origin.y + temp[0][1]), IM_COL32(255, 0, 0, 255), 2.0f);
				}
				if (data->typepoint) {
					for (int n = 0; n < temp.size(); n += 1)
						draw_list->AddCircleFilled(ImVec2(origin.x + temp[n][0], origin.y + temp[n][1]), 3.0f, IM_COL32(255, 255, 255, 255), 0);
				}
			}
			draw_list->PopClipRect();
		}

		ImGui::End();
	});
}
/*
将输入的点集ChaiKin细分，closed参数判断曲线是否封闭
*/
void ChaiKin(std::vector<Ubpa::pointf2>& points, bool closed)
{
	std::vector<Ubpa::pointf2> newpoints;
	if (closed) {
		for (int i = 0;i < points.size();i++) {
			Ubpa::pointf2 temp;
			if (i == 0) {
				temp[0] = 0.25 * points[points.size() - 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[points.size() - 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
				temp[0] = 0.25 * points[i + 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[i + 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
			}
			else if (i == points.size() - 1) {
				temp[0] = 0.25 * points[i - 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[i - 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
				temp[0] = 0.25 * points[0][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[0][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
			}
			else {
				temp[0] = 0.25 * points[i - 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[i - 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
				temp[0] = 0.25 * points[i + 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[i + 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
			}
		}
	}
	else {
		for (int i = 0;i < points.size();i++) {
			Ubpa::pointf2 temp;
			if (i == 0) {
				temp[0] = 0.25 * points[i + 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[i + 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
			}
			else if (i == points.size() - 1) {
				temp[0] = 0.25 * points[i - 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[i - 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
			}
			else {
				temp[0] = 0.25 * points[i - 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[i - 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
				temp[0] = 0.25 * points[i + 1][0] + 0.75 * points[i][0];
				temp[1] = 0.25 * points[i + 1][1] + 0.75 * points[i][1];
				newpoints.push_back(temp);
			}
		}
	}
	points = newpoints;
}
 /*
 4点插值细分，默认曲线封闭,至少有四个点
 */
void Interpolatory(std::vector<Ubpa::pointf2>& points,float alpha)
{
	std::vector<Ubpa::pointf2> newpoints;
	for (int i = 0;i < points.size();i++) {
		Ubpa::pointf2 temp;
		if (i == 0) {
			newpoints.push_back(points[i]);
			temp[0] = (1 + alpha) / 2 * (points[i][0] + points[i + 1][0]) - alpha / 2 * (points[points.size() - 1][0] + points[i + 2][0]);
			temp[1] = (1 + alpha) / 2 * (points[i][1] + points[i + 1][1]) - alpha / 2 * (points[points.size() - 1][1] + points[i + 2][1]);
			newpoints.push_back(temp);
		}
		else if (i == points.size() - 2) {
			newpoints.push_back(points[i]);
			temp[0] = (1 + alpha) / 2 * (points[i][0] + points[i + 1][0]) - alpha / 2 * (points[i - 1][0] + points[0][0]);
			temp[1] = (1 + alpha) / 2 * (points[i][1] + points[i + 1][1]) - alpha / 2 * (points[i - 1][1] + points[0][1]);
			newpoints.push_back(temp);
		}
		else if (i == points.size() - 1) {
			newpoints.push_back(points[i]);
			temp[0] = (1 + alpha) / 2 * (points[i][0] + points[0][0]) - alpha / 2 * (points[i - 1][0] + points[1][0]);
			temp[1] = (1 + alpha) / 2 * (points[i][1] + points[0][1]) - alpha / 2 * (points[i - 1][1] + points[1][1]);
			newpoints.push_back(temp);
		}
		else {
			newpoints.push_back(points[i]);
			temp[0] = (1 + alpha) / 2 * (points[i][0] + points[i + 1][0]) - alpha / 2 * (points[i - 1][0] + points[i + 2][0]);
			temp[1] = (1 + alpha) / 2 * (points[i][1] + points[i + 1][1]) - alpha / 2 * (points[i - 1][1] + points[i + 2][1]);
			newpoints.push_back(temp);
		}
	}
	points = newpoints;
}
