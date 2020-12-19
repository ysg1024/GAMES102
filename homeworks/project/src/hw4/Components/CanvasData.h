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
	int selected = -1;  //ѡ��ķ����߿��Ƶ�
	int tselected = -1; //ѡ������߿��Ƶ�
	std::vector<Ubpa::pointf2> tPoints;
	int continuity = 3; //Ĭ�ϸı�����ʱC1���� 2: G1 1: G0

	// ???3
	std::vector<Ubpa::pointf2> lpoints;

	// �����������߲���
	std::vector<Ubpa::vecf2> tangents;
	std::vector<float> delta;//���������
};

#include "details/CanvasData_AutoRefl.inl"
