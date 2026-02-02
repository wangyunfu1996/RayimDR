#include "XRayCfgDialog.h"

#include "ElaUIHepler.h"
#include "VJXRAY/IXS120BP120P366.h"

XRayCfgDialog::XRayCfgDialog(QWidget* parent) : ElaDialog(parent)
{
    ui.setupUi(this);
    this->setWindowTitle("射线源设置");
    this->setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
    this->setWindowButtonFlag(ElaAppBarType::MinimizeButtonHint, false);
    this->setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
    this->resize(700, 200);
    ElaUIHepler::ChangeToNormalStyle(this);

    connect(ui.pushButton_confirm, &QPushButton::clicked, this, [this]() { this->accept(); });
    connect(ui.pushButton_cancel, &QPushButton::clicked, this, [this]() { this->reject(); });
    connect(ui.checkBox_param, &QCheckBox::checkStateChanged, this,
            [this](int state)
            {
                bool isChecked = (state == Qt::Checked);
                ui.lineEdit_param->setEnabled(isChecked);
            });
    connect(ui.pushButton_docmd, &QPushButton::clicked, this,
            [this]()
            {
                std::string cmd = ui.lineEdit_cmd->text().toStdString();
                std::string param = "";
                if (ui.checkBox_param->isChecked())
                {
                    param = ui.lineEdit_param->text().toStdString();
                }
                qDebug() << "Prepared to send command to X-ray source:" << QString::fromStdString(cmd)
                         << (param.empty() ? "" : (" with param: " + QString::fromStdString(param)));
                //IXS120BP120P366::Instance().sendCommand(cmd, param);
            });


    ui.lineEdit_cmd->setEnabled(true);
    ui.lineEdit_cmdResult->setReadOnly(true);
}

XRayCfgDialog::~XRayCfgDialog() {}
