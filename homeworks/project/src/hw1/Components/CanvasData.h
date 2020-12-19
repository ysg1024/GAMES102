#pragma once

#include <UGM/UGM.h>
#include "imgui/imgui.h"

struct CanvasData {
	std::vector<Ubpa::pointf2> points;
	Ubpa::valf2 scrolling{ 0.f,0.f };

	std::vector<ImVec2> Vandermonde;
	std::vector<ImVec2> Gauss;
	std::vector<ImVec2> Least_Squares;
	std::vector<ImVec2> Ridge_Regression;
	bool opt_enable_Vandermonde{ false };
	bool opt_enable_Gauss{ false };
	bool opt_enable_LS{ false };
	bool opt_enable_RR{ false };

	int LeastSquaresM = 4;
	float RidgeRegressionLamda = 0.1;

	std::vector<ImVec2> uniform;
	std::vector<ImVec2> chord;
	std::vector<ImVec2> centripetal;
	bool opt_enable_uniform{ false };
	bool opt_enable_chord{ false };
	bool opt_enable_centripetal{ false };

	bool opt_enable_grid{ true };
	bool opt_enable_context_menu{ true };
	bool adding_line{ false };
};

#include "details/CanvasData_AutoRefl.inl"
