#pragma once

#include <QWidget>
#include "ui_CommonConfigUI.h"

#include "Components/XGlobal.h"

class CommonConfigUI : public QWidget
{
	Q_OBJECT

public:
	CommonConfigUI(QWidget* parent = nullptr);
	~CommonConfigUI();

	bool checkInputValid();
	AcqCondition getAcqCondition();

private:
	void onThemeChanged(ElaThemeType::ThemeMode themeMode);
	void initUIConnect();
	void changeMode(const QString& modeText);
	void changeFrameRate(int nFrameRateComboboxIdx);
	std::string getModeFromUI();
	void setUIEnable(bool enable);
	void updateUIFromMode(std::string mode);

private:
	Ui::CommonConfigUIClass ui;

	AcqCondition acqCondition;
};

