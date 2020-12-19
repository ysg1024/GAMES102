#pragma once

#include <UGM/UGM.h>

struct CanvasData {
	std::vector<Ubpa::pointf2> points;
	Ubpa::valf2 scrolling{ 0.f,0.f };
	bool opt_enable_grid{ true };
	bool opt_enable_context_menu{ true };
	bool adding_line{ false };

	bool ChaiKin{ false };//ChaiKinϸ��
	bool Interpolatory{ false };//4���ֵϸ��
	bool closed{ false };//�����Ƿ�պ�
	bool typepoint{ true };//��ʾ��ֵ��
	int times{ 1 };//ϸ�ִ���
	float alpha{ 0 };//��ֵϸ��alphaֵ
};

#include "details/CanvasData_AutoRefl.inl"
