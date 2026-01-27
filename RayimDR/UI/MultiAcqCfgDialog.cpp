#include "MultiAcqCfgDialog.h"

#include <qfileinfo.h>
#include <qdir.h>
#include <qfiledialog.h>
#include <qdebug.h>

#include "Components/XSignalsHelper.h"

#include "ElaUIHepler.h"

MultiAcqCfgDialog::MultiAcqCfgDialog(QWidget* parent) : ElaDialog(parent)
{
    ui.setupUi(this);
    this->setWindowTitle("设置多帧采集条件");
    this->setWindowButtonFlag(ElaAppBarType::StayTopButtonHint, false);
    this->setWindowButtonFlag(ElaAppBarType::MinimizeButtonHint, false);
    this->setWindowButtonFlag(ElaAppBarType::MaximizeButtonHint, false);
    this->resize(640, 200);

    ui.spinBox_n->setMinimum(nMin);
    ui.spinBox_n->setMaximum(nMax);

    ElaUIHepler::ChangeToNormalStyle(this);

    connect(ui.pushButton_confirm, &QPushButton::clicked, this, [this]() { this->accept(); });

    connect(ui.pushButton_cancel, &QPushButton::clicked, this, [this]() { this->reject(); });

    ui.lineEdit_savePath->setEnabled(false);
    ui.comboBox_saveType->setEnabled(false);
    ui.pushButton_select->setEnabled(false);
    connect(ui.checkBox_saveToFiles, &QCheckBox::toggled, this,
            [this](bool checked)
            {
                ui.comboBox_saveType->setEnabled(checked);
                ui.pushButton_select->setEnabled(checked);
            });

    connect(ui.pushButton_select, &QPushButton::clicked, this,
            [this]()
            {
                // 选择文件夹
                QString folderPath =
                    QFileDialog::getExistingDirectory(nullptr, "选择保存文件夹", ".", QFileDialog::ShowDirsOnly);

                if (!folderPath.isEmpty())
                {
                    ui.lineEdit_savePath->setText(folderPath);
                    qDebug() << "选择的文件夹路径: " << folderPath;
                }
                else
                {
                    qDebug() << "用户取消了选择";
                }
            });
}

MultiAcqCfgDialog::~MultiAcqCfgDialog() {}

bool MultiAcqCfgDialog::getCfg(int& n, bool& saveToFiles, QString& savePath, QString& saveType)
{
    if (ui.spinBox_n->value() < nMin || ui.spinBox_n->value() > nMax)
    {
        QString msg = QString("请正确设置采集帧数（%1 - %2）!").arg(nMin).arg(nMax);
        qDebug() << msg;
        emit xSignaHelper.signalShowErrorMessageBar(msg);
        return false;
    }

    if (ui.checkBox_saveToFiles->isChecked())
    {
        if (ui.lineEdit_savePath->text().isEmpty() || ui.lineEdit_savePath->text().isNull() ||
            !QDir(ui.lineEdit_savePath->text()).exists())
        {
            QString msg = QString("多帧采集选择的保存位置 %1 不存在!").arg(ui.lineEdit_savePath->text());
            qDebug() << msg;
            emit xSignaHelper.signalShowErrorMessageBar(msg);
            return false;
        }
    }

    n = ui.spinBox_n->value();
    saveToFiles = ui.checkBox_saveToFiles->isChecked();
    savePath = ui.lineEdit_savePath->text();
    saveType = ui.comboBox_saveType->currentText();
    return true;
}
