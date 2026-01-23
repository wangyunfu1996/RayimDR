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
	void Offset();	// 一键生成Offset
	void Gain();	// 一键生成Gain
	void Defect();	// 一键生成Defect

	void ShowTips(const QString& msg);
	void ModifyGainVoltageCurrent(int voltage, int current);
	void ModifyDefectVoltageCurrent(int voltage, int current);
	void ModifyMode(int modeIdx);

private:
	void onOffsetImageSelected(int nTotal, int nValid);
	void onGainAcqImageReceived(QImage image, int idx);
	void onGainImageSelected(int nTotal, int nValid);
	void onDefectAcqImageReceived(QImage image, int idx);
	void onDefectGroupChanged(int groupIdx, int nTotalGroup);
	void onDefectImageSelected(int nTotal, int nValid);

signals:
	void signalTipsChanged(const QString& msg);
	void signalGainVoltageCurrentChanged(int voltage, int current);
	void signalDefectVoltageCurrentChanged(int voltage, int current);
	void signalDefectGroupChanged(int groupIdx, int nTotalGroup);

private:
	Ui::CreateCorrectTemplateDlgClass ui;
	int nCurrentGray{ 0 };
	class QGraphicsPixmapItem* gainPixmapItem{ nullptr };
	class QGraphicsPixmapItem* defectPixmapItem{ nullptr };
};

