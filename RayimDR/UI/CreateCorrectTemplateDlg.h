#pragma once

#include "ElaDialog.h"
#include "ui_CreateCorrectTemplateDlg.h"

class CreateCorrectTemplateDlg : public ElaDialog
{
	Q_OBJECT

public:
	CreateCorrectTemplateDlg(QWidget* parent = nullptr);
	~CreateCorrectTemplateDlg();

private:
	void Abort();

	bool OffsetGeneration();

	void Gain();	// 一键生成Gain
	bool GainInit();
	bool GainStartAcq();
	bool GainSelectAll();
	bool GainGeneration();

	void Defect();	// 一键生成Defect
	bool DefectInit();
	bool DefectStartAcq();
	bool DefectSelectAll();
	bool DefectGeneration();

	void ShowTips(const QString& msg);
	void MidifyGainVoltageCurrent(int voltage, int current);

signals:
	void signalTipsChanged(const QString& msg);
	void signalGainVoltageCurrentChanged(int voltage, int current);

private:
	Ui::CreateCorrectTemplateDlgClass ui;
	int nCurrentGray{ 0 };
	class QGraphicsPixmapItem* gainPixmapItem{ nullptr };
};

