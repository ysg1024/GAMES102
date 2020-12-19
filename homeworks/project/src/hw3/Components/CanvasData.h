#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<Ubpa::pointf2> points;
	Ubpa::valf2 scrolling{ 0.f,0.f };
	bool opt_enable_grid{ true };
	bool opt_enable_context_menu{ true };
	bool adding_line{ false };

	bool ChaiKin{ false };//ChaiKin细分
	bool Interpolatory{ false };//4点插值细分
	bool closed{ false };//曲线是否闭合
	bool typepoint{ true };//显示型值点
	int times{ 1 };//细分次数
	float alpha{ 0 };//插值细分alpha值
};

#include "details/CanvasData_AutoRefl.inl"
