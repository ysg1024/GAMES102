#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<Ubpa::pointf2> points;
	Ubpa::valf2 scrolling{ 0.f,0.f };
	bool opt_enable_grid{ true };
	bool opt_enable_context_menu{ true };
	bool adding_line{ false };

	// ???1
	int method{ 4 };
	bool opt_enable_uniform{ false };
	bool opt_enable_chord{ false };
	bool opt_enable_centripetal{ false };
	bool ChangeControlPoint{ false };

	// ???2
	int selected = -1;  //选择的非切线控制点
	int tselected = -1; //选择的切线控制点
	std::vector<Ubpa::pointf2> tPoints;
	int continuity = 3; //默认改变切线时C1连续 2: G1 1: G0

	// ???3
	std::vector<Ubpa::pointf2> lpoints;

	// 计算样条曲线参数
	std::vector<Ubpa::vecf2> tangents;
	std::vector<float> delta;//参数化结果
};

#include "details/CanvasData_AutoRefl.inl"
