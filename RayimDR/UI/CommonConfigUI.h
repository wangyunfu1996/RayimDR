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

private:
	Ui::CommonConfigUIClass ui;

	AcqCondition acqCondition;
};

