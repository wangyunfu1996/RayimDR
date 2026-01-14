#include "ElaUIHepler.h"

#include "ElaText.h"
#include "ElaSpinBox.h"
#include "ElaDoubleSpinBox.h"
#include "ElaLineEdit.h"
#include "ElaComboBox.h"

#define ELASPINBOX_HEIGHT 35
#define ELASPINBOX_WIDTH 115

#define ELADOUBLESPINBOX_HEIGHT 35
#define ELADOUBLESPINBOX_WIDTH 115

#define ELACOMBOBOX_HEIGHT 35

#define ELALINEEDIT_HEIGHT 35
#define ELALINEEDIT_WIDTH 115

#define ELACOMBOBOX_HEIGHT 35

ElaUIHepler::ElaUIHepler(QObject* parent)
	: QObject(parent)
{}

ElaUIHepler::~ElaUIHepler()
{}

void ElaUIHepler::ChangeToNormalStyle(QWidget* widget)
{
	if (nullptr == widget)
	{
		return;
	}

	for (auto elaText : widget->findChildren<ElaText*>())
	{
		elaText->setWordWrap(false);
	}

	for (auto elaSpinBox : widget->findChildren<ElaSpinBox*>())
	{
		elaSpinBox->setButtonMode(ElaSpinBoxType::ButtonMode::PMSide);
		elaSpinBox->setMinimumWidth(ELASPINBOX_WIDTH);
	}

	for (auto elaDoubleSpinBox : widget->findChildren<ElaDoubleSpinBox*>())
	{
		elaDoubleSpinBox->setButtonMode(ElaSpinBoxType::ButtonMode::PMSide);
		elaDoubleSpinBox->setMinimumWidth(ELADOUBLESPINBOX_WIDTH);
	}

	for (auto elaLineEdit : widget->findChildren<ElaLineEdit*>())
	{
		elaLineEdit->setMinimumWidth(ELALINEEDIT_WIDTH);
		elaLineEdit->setAlignment(Qt::AlignmentFlag::AlignCenter);
		elaLineEdit->setEnabled(false);
	}

	for (auto elaComboBox : widget->findChildren<ElaComboBox*>())
	{
	}

}

