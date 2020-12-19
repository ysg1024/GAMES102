#include "CanvasSystem.h"

#include "../Components/CanvasData.h"

#include <_deps/imgui/imgui.h>
// ???3
#include "../Eigen/Dense"
#include "../Eigen/Sparse"
#include "../Eigen/SparseLU"
#include <algorithm>
#include <cmath>
using namespace Ubpa;

bool selecting(ImVec2 center, ImVec2 mousePos, float r)
{/* input: ���Ƚϵĵ����꣬ ��������꣬�ж����
	output: flag=true: ѡ�п��Ƶ�
	*/
	bool flag = true;
	if (center[0] - r > mousePos[0])
		flag = false;
	if (center[1] - r > mousePos[1])
		flag = false;
	if (center[0] + r < mousePos[0])
		flag = false;
	if (center[1] + r < mousePos[1])
		flag = false;
	return flag;
}

// �����������߲���
void CacuSpline(CanvasData* data);
typedef Eigen::Triplet<float> float_tri;
// ???2
float ratio = 15.0f;//���ƻ��Ƶ����߳���

//����������
std::vector<float>UniformP(std::vector<Ubpa::pointf2>& point);
std::vector<float> ChordalP(std::vector<Ubpa::pointf2>& point);
std::vector<float> CentripetalP(std::vector<Ubpa::pointf2>& point);
std::vector<float> FoleyP(std::vector<Ubpa::pointf2>& point);

//Hermite��ֵ
std::vector<Ubpa::pointf2>Hermite(CanvasData* data);

void CanvasSystem::OnUpdate(Ubpa::UECS::Schedule& schedule) {
	schedule.RegisterCommand([](Ubpa::UECS::World* w) {
		auto data = w->entityMngr.GetSingleton<CanvasData>();
		if (!data)
			return;

		if (ImGui::Begin("Canvas")) {
			ImGui::Checkbox("Enable grid", &data->opt_enable_grid);
			ImGui::Checkbox("Enable context menu", &data->opt_enable_context_menu);
			ImGui::Text("Mouse Left: drag to add lines,\nMouse Right: drag to scroll, click for context menu.");
			// ???1
			//ImGui::SliderInt("Patameterization methods", &data->method, 1, 4);
			ImGui::Checkbox("Uniform Parameterization", &data->opt_enable_uniform);
			ImGui::Checkbox("Chord Parameterization", &data->opt_enable_chord);
			ImGui::Checkbox("Centripetal Parameterization", &data->opt_enable_centripetal);
			ImGui::Checkbox("Edit Mode", &data->ChangeControlPoint);

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
			// ???2
			if (is_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				int selected = -1;
				ImVec2 mPos = mouse_pos_in_canvas;
				// ����Ƿ�ѡ���˿��Ƶ�
				for (size_t i = 0; i < data->points.size(); ++i)
				{
					if (selecting(data->points[i], mPos, 5.0f) == true)
					{
						selected = i;
						data->selected = i;
						data->tselected = -1;
						break;
					}
				}
				// ��û��ѡ�е�
				if (selected == -1)
				{
					//���Ǹ��Ŀ��Ƶ�ģʽѡ����һ���㲢�Ҹ�֡û��ѡ�е�
					if (data->ChangeControlPoint && data->selected != -1)
					{
						//��ѡ������ߵ����߿��Ƶ�
						if (selecting(data->tPoints[0], mPos, 5.0f))
						{
							data->tselected = 0;
						}
						//ѡ�����ұߵ����߿��Ƶ�
						else if (selecting(data->tPoints[1], mPos, 5.0f))
						{
							data->tselected = 1;
						}
						//֮ǰѡ���˷����߿��Ƶ�
						else if (data->tselected == -1)
						{
							//���ĸÿ��Ƶ�
							data->points[data->selected] = mPos;
							CacuSpline(data);
						}
						//֮ǰѡ�������߿��Ƶ�
						else
						{
							pointf2 pos(mPos[0], mPos[1]);
							float tx, ty;//��б��,��ʾ��λ������tx,ty)
							//����е�
							if (data->tselected == 0)
							{
								tx = data->points[data->selected][0] - pos[0];
								ty = data->points[data->selected][1] - pos[1];
								tx *= ratio;
								ty *= ratio;
							}
							//�Ҳ�
							else if (data->tselected == 1)
							{
								tx = pos[0] - data->points[data->selected][0];
								ty = pos[1] - data->points[data->selected][1];
								tx *= ratio;
								ty *= ratio;
							}
							if (data->selected == 0)
							{
								data->tangents[0][0] = tx;
								data->tangents[0][1] = ty;
							}
							else if (data->selected == data->points.size() - 1)
							{
								data->tangents[2 * data->selected - 1][0] = tx;
								data->tangents[2 * data->selected - 1][1] = ty;
							}
							else
							{
								switch (data->continuity)
								{
								case 3:
									//C1 
									if (data->selected != 0)
									{
										data->tangents[2 * data->selected - 1][0] = tx;
										data->tangents[2 * data->selected - 1][1] = ty;
									}
									if (data->selected != data->points.size() - 1)
									{
										data->tangents[2 * data->selected][0] = tx;
										data->tangents[2 * data->selected][1] = ty;
									}
									break;
								case 2:
									//G1

									if (data->tselected == 0)
									{
										//�����Ҳ����߳���
										float length = data->tangents[2 * data->selected].norm();
										//�������б��
										data->tangents[2 * data->selected - 1][0] = tx;
										data->tangents[2 * data->selected - 1][1] = ty;
										//��ȡ������߷���λ����
										vecf2 tv(tx, ty);
										tv = tv.normalize();
										tv *= length;
										//�����Ҳ�
										data->tangents[2 * data->selected][0] = tv[0];
										data->tangents[2 * data->selected][1] = tv[1];

									}
									else
									{
										//����������߳���
										float length = data->tangents[2 * data->selected - 1].norm();
										//�����Ҳ�б��
										data->tangents[2 * data->selected][0] = tx;
										data->tangents[2 * data->selected][1] = ty;
										//��ȡ�Ҳ����ߵ�λ����
										vecf2 tv(tx, ty);
										tv = tv.normalize();
										tv *= length;
										data->tangents[2 * data->selected - 1][0] = tv[0];
										data->tangents[2 * data->selected - 1][1] = tv[1];
									}
									break;
								case 1:
									//G0
									if (data->tselected == 0)
									{
										data->tangents[2 * data->selected - 1][0] = tx;
										data->tangents[2 * data->selected - 1][1] = ty;
									}
									else
									{
										data->tangents[2 * data->selected][0] = tx;
										data->tangents[2 * data->selected][1] = ty;
									}
									break;
								default:
									break;
								}
							}





							//TODO :G1 G0
						}
					}
					// ����ѡ�п��Ƶ�ģʽ
					else if (!data->ChangeControlPoint) //�����ƶ����Ƶ�ģʽ
					{
						if (data->selected != -1)  //֮ǰѡ���˿��Ƶ� ��ȡ��֮
						{
							data->selected = -1;
							data->tselected = -1;
						}

						else //δѡ���������µ�
						{
							data->points.push_back(mPos);

							/*switch (data->method)
							{
							case 1: data->delta = UniformP(data->points); break;
							case 2: data->delta = ChordalP(data->points); break;
							case 3: data->delta = CentripetalP(data->points); break;
							case 4: data->delta = FoleyP(data->points); break;
							}*/
							if (data->opt_enable_uniform == true) {
								data->delta = UniformP(data->points);
							}
							else if (data->opt_enable_chord == true) {
								data->delta = ChordalP(data->points);
							}
							else if (data->opt_enable_centripetal == true) {
								data->delta = CentripetalP(data->points);
							}
							CacuSpline(data);
						}
					}


				}

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
				// ???3
				if (data->adding_line)
					data->points.resize(data->points.size() - 2);
				data->adding_line = false;
				//if (ImGui::MenuItem("Remove one", NULL, false, data->points.size() > 0)) { data->points.resize(data->points.size() - 2); }
				if (ImGui::MenuItem("Remove all", NULL, false, data->points.size() >= 0)) {
					data->points.clear();
					data->lpoints.clear();
					data->delta.clear();
					data->selected = -1;
					data->tselected = -1;
					data->tPoints.clear();
				}
				if (ImGui::MenuItem("C1(default)", NULL, false))
				{
					data->continuity = 3;
				}
				if (ImGui::MenuItem("G1", NULL, false))
				{
					data->continuity = 2;
				}
				if (ImGui::MenuItem("G0", NULL, false))
				{
					data->continuity = 1;
				}
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
			// ???4
			for (int n = 0; n < data->points.size(); n += 1)
				draw_list->AddCircleFilled(ImVec2(origin.x + data->points[n][0], origin.y + data->points[n][1]), 3.0f, IM_COL32(255, 255, 255, 255), 0);

			if (data->selected != -1)
			{
				data->tPoints.clear();
				int s = data->selected;
				//��ͨ���ѡ���
				//draw_list->AddRect(ImVec2(origin.x + data->points[data->selected][0] - 5.0f, origin.y + data->points[data->selected][1] - 5.0f),
				//	ImVec2(origin.x + data->points[data->selected][0] + 5.0f, origin.y + data->points[data->selected][1] + 5.0f),
				//	IM_COL32(255, 255, 255, 255));
				//���������߿��Ƶ����
				if (s != 0)
				{
					pointf2 tp1(data->points[s][0] - data->tangents[2 * s - 1][0] / ratio,
						data->points[s][1] - data->tangents[2 * s - 1][1] / ratio);
					data->tPoints.push_back(tp1);
					draw_list->AddLine(ImVec2(origin.x + data->points[s][0], origin.y + data->points[s][1]),
						ImVec2(origin.x + tp1[0], origin.y + tp1[1]), IM_COL32(255, 255, 255, 255), 1.0f);

					draw_list->AddCircleFilled(ImVec2(origin.x + tp1[0], origin.y + tp1[1]), 3.0f, IM_COL32(255, 255, 255, 255), 0);
					//if (data->tselected == 0)
					//{
					//	draw_list->AddRect(ImVec2(origin.x + tp1[0] - 5.0f, origin.y + tp1[1] - 5.0f),
					//		ImVec2(origin.x + tp1[0] + 5.0f, origin.y + tp1[1] + 5.0f),
					//		IM_COL32(255, 255, 255, 255));
					//}

				}
				else
					data->tPoints.push_back(pointf2(0.0f, 0.0f));
				if (s != data->points.size() - 1)
				{
					pointf2 tp1(data->points[s][0] + data->tangents[2 * s][0] / ratio,
						data->points[s][1] + data->tangents[2 * s][1] / ratio);
					data->tPoints.push_back(tp1);
					draw_list->AddLine(ImVec2(origin.x + data->points[s][0], origin.y + data->points[s][1]),
						ImVec2(origin.x + tp1[0], origin.y + tp1[1]), IM_COL32(255, 255, 255, 255), 1.0f);

					draw_list->AddCircleFilled(ImVec2(origin.x + tp1[0], origin.y + tp1[1]), 3.0f, IM_COL32(255, 255, 255, 255), 0);
					//if (data->tselected == 1)
					//{
					//	draw_list->AddRect(ImVec2(origin.x + tp1[0] - 5.0f, origin.y + tp1[1] - 5.0f),
					//		ImVec2(origin.x + tp1[0] + 5.0f, origin.y + tp1[1] + 5.0f),
					//		IM_COL32(255, 255, 255, 255));
					//}
				}
				else
					data->tPoints.push_back(pointf2(0.0f, 0.0f));


			}
			/*if(data->points.size() > 0)
				draw_list->AddCircleFilled(ImVec2(origin.x + data->points[0][0], origin.y + data->points[0][1]), 5.0f, IM_COL32(255, 255, 255, 255), 0);*/

				// draw lines
			data->lpoints = Hermite(data);
			for (int n = 0; n < data->lpoints.size(); n += 2)
			{

				draw_list->AddLine(ImVec2(origin.x + data->lpoints[n][0], origin.y + data->lpoints[n][1]), ImVec2(origin.x + data->lpoints[n + 1][0], origin.y + data->lpoints[n + 1][1]), IM_COL32(255, 0, 0, 255), 1.0f);

			}


			draw_list->PopClipRect();
		}

		ImGui::End();
	});
}

void CacuSpline(CanvasData* data)
{
	std::vector<Ubpa::pointf2> ret;
	//start here
	data->tangents.clear();
	int d = data->points.size();
	if (d < 3)
		return;
	Eigen::SparseMatrix<float> A(d - 2, d - 2);//ϵ������
	Eigen::SparseLU<Eigen::SparseMatrix<float>, Eigen::COLAMDOrdering<int> >solver;
	//Eigen::MatrixXf A(d, dim + 1);
	Eigen::VectorXf b(d - 2, 1);
	Eigen::VectorXf by(d - 2, 1);
	//�洢��Ԫ����vector
	std::vector<float_tri> tv;
	float t = 0.0;
	for (int i = 0; i < d - 2; ++i)
	{
		float h_pre = data->delta[i];
		float h_now = data->delta[i + 1];
		float u = 2 * (h_pre + h_now);
		if (i != 0)
		{
			float_tri temp(i, i - 1, h_pre);
			tv.push_back(temp);
		}
		if (i != d - 3)
		{
			float_tri temp(i, i + 1, h_now);
			tv.push_back(temp);
		}
		float_tri temp(i, i, u);
		tv.push_back(temp);
		//handle x first
		float b_pre = 6 * (data->points[i + 1][0] - data->points[i][0]) / h_pre;
		float b_now = 6 * (data->points[i + 2][0] - data->points[i + 1][0]) / h_now;
		float v = b_now - b_pre;
		b(i) = v;
		//then handle y
		b_pre = 6 * (data->points[i + 1][1] - data->points[i][1]) / h_pre;
		b_now = 6 * (data->points[i + 2][1] - data->points[i + 1][1]) / h_now;
		v = b_now - b_pre;
		by(i) = v;

	}
	//�����ԽǷ���
	A.setFromTriplets(tv.begin(), tv.end());//������A��ֵ
	solver.analyzePattern(A);
	solver.factorize(A);
	Eigen::VectorXf M = solver.solve(b);
	std::vector<float> Mx, My;
	Mx.push_back(0);
	for (int i = 0; i < M.size(); ++i)
	{
		Mx.push_back(M[i]);
	}
	Mx.push_back(0);
	M = solver.solve(by);
	My.push_back(0);
	for (int i = 0; i < M.size(); ++i)
	{
		My.push_back(M[i]);
	}
	My.push_back(0);
	Ubpa::pointf2 pre;
	float accu = 0.0;
	bool first = 1;
	for (int i = 0; i < d - 1; ++i)
	{
		float st = accu;
		float ed = accu + data->delta[i];
		accu = ed;
		float x = data->points[i][0];
		float x1 = data->points[i + 1][0];
		float y = data->points[i][1];
		float y1 = data->points[i + 1][1];
		float h = data->delta[i];
		float f0_hat_x = -h / 3 * Mx[i] - h / 6 * Mx[i + 1] - x / h + x1 / h;
		float f1_hat_x = h / 6 * Mx[i] + h / 3 * Mx[i + 1] - x / h + x1 / h;
		float f0_hat_y = -h / 3 * My[i] - h / 6 * My[i + 1] - y / h + y1 / h;
		float f1_hat_y = h / 6 * My[i] + h / 3 * My[i + 1] - y / h + y1 / h;
		float hneg = -data->delta[i];
		vecf2 temp(f0_hat_x, f0_hat_y);
		data->tangents.push_back(temp);
		temp[0] = f1_hat_x;
		temp[1] = f1_hat_y;
		data->tangents.push_back(temp);

	}
}

std::vector<float>UniformP(std::vector<Ubpa::pointf2>& point)
{
	int d = point.size();
	float tdelta = 1.0 / (d - 1);
	std::vector<Ubpa::pointf2> ret;
	std::vector<float> delta;
	for (int i = 0; i < d - 1; ++i)
		delta.push_back(tdelta);
	return delta;
}

std::vector<float> ChordalP(std::vector<Ubpa::pointf2>& point)
{
	int d = point.size();
	std::vector<float> delta;
	float arc_length = 0.0;
	for (int i = 1; i < d; ++i)
	{
		arc_length += (point[i] - point[i - 1]).norm();
	}
	for (int i = 1; i < d; ++i)
	{
		float tl = (point[i] - point[i - 1]).norm();
		tl /= arc_length;
		delta.push_back(tl);
	}
	return delta;
}

std::vector<float> CentripetalP(std::vector<Ubpa::pointf2>& point)
{
	int d = point.size();
	std::vector<float> delta;
	float arc_length = 0.0;
	for (int i = 1; i < d; ++i)
	{
		arc_length += sqrt((point[i] - point[i - 1]).norm());

	}
	for (int i = 1; i < d; ++i)
	{
		float tl = sqrt((point[i] - point[i - 1]).norm());
		tl /= arc_length;
		delta.push_back(tl);
	}
	return delta;
}

std::vector<float> FoleyP(std::vector<Ubpa::pointf2>& point)
{
	int d = point.size();
	std::vector<float> delta;
	if (d < 3)
		return delta;
	std::vector<float> dis;
	std::vector<float> theta;
	float delta_sum = 0.0;
	theta.push_back(-1);
	for (int i = 1; i < d; ++i)
	{
		if (i != d - 1)
		{
			Ubpa::vecf2 a = point[i - 1] - point[i];
			Ubpa::vecf2 b = point[i + 1] - point[i];
			float ttheta = acos(a.dot(b) / (a.norm() * b.norm()));

			theta.push_back(std::min(PI<float> -ttheta, PI<float> / 2.0f));
		}
		dis.push_back((point[i] - point[i - 1]).norm());
	}
	float ttemp = dis[0] * (1 + 3 * theta[1] * dis[1] / (2 * (dis[0] + dis[1])));
	delta.push_back(ttemp);
	delta_sum += ttemp;
	for (int i = 1; i < d - 2; ++i)
	{
		ttemp = dis[i] * (1 + 3 * theta[i] * dis[i - 1] / (2 * (dis[i - 1] + dis[i]) + 3 * theta[i + 1] * dis[i + 1] / (2 * (dis[i] + dis[i + 1]))));
		delta.push_back(ttemp);
		delta_sum += ttemp;
	}
	ttemp = dis[d - 2] * (1 + 3 * theta[d - 2] * dis[d - 3] / (2 * (dis[d - 3] + dis[d - 2])));
	delta_sum += ttemp;
	delta.push_back(ttemp);

	for (int i = 0; i < delta.size(); ++i)
	{
		delta[i] /= delta_sum;
	}
	return delta;
}

std::vector<Ubpa::pointf2>Hermite(CanvasData* data)
{
	std::vector<Ubpa::pointf2> ret;
	if (data->points.size() < 3)
		return ret;
	int d = data->points.size();
	float accu = 0.0;
	Ubpa::pointf2 pre;
	bool first = true;
	for (int i = 0; i < d - 1; ++i)
	{
		float st = accu;
		float ed = accu + data->delta[i];
		accu = ed;
		float x = data->points[i][0];
		float x1 = data->points[i + 1][0];
		float y = data->points[i][1];
		float y1 = data->points[i + 1][1];
		float h = data->delta[i];
		float f0_hat_x = data->tangents[2 * i][0];
		float f1_hat_x = data->tangents[2 * i + 1][0];
		float f0_hat_y = data->tangents[2 * i][1];
		float f1_hat_y = data->tangents[2 * i + 1][1];
		float hneg = -data->delta[i];
		for (float t = st; t < ed; t += 0.01)
		{
			Ubpa::pointf2 tp;
			float h0 = (1 + 2 * (t - st) / h) * pow((t - ed) / hneg, 2);
			float h1 = (1 + 2 * (t - ed) / hneg) * pow((t - st) / h, 2);
			float h0_hat = (t - st) * pow((t - ed) / hneg, 2);
			float h1_hat = (t - ed) * pow((t - st) / h, 2);
			tp[0] = h0 * x + h1 * x1 + h0_hat * f0_hat_x + h1_hat * f1_hat_x;
			tp[1] = h0 * y + h1 * y1 + h0_hat * f0_hat_y + h1_hat * f1_hat_y;
			/*tp[0] = Mx[i] * pow((ed - t), 3) / (6 * h) + Mx[i + 1] * pow((t - st), 3) / (6 * h) +
				(x1 / h - Mx[i + 1] * h / 6.0) * (t - st) + (x / h - Mx[i] * h / 6.0) * (ed - t);
			tp[1] = My[i] * pow((ed - t), 3) / (6 * h) + My[i + 1] * pow((t - st), 3) / (6 * h) +
				(y1 / h - My[i + 1] * h / 6.0) * (t - st) + (y / h - My[i] * h / 6.0) * (ed - t);*/
			if (first)
			{
				first = false;
			}
			else
			{
				ret.push_back(pre);
				ret.push_back(tp);
			}
			pre = tp;

		}
	}
	return ret;
}