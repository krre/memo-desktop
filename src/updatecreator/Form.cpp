#include "Form.h"
#include <QtWidgets>

Form::Form(QWidget* parent) : QWidget(parent) {
    setMinimumWidth(100);

    auto layout = new QVBoxLayout;
    setLayout(layout);

    auto generalGroupBox = new QGroupBox(tr("General"));

    auto generalGridLayout = new QGridLayout;
    generalGroupBox->setLayout(generalGridLayout);

    generalGridLayout->addWidget(new QLabel(tr("File template:")), 0, 0);
    templateLineEdit = new QLineEdit;
    generalGridLayout->addWidget(templateLineEdit, 0, 1);

    layout->addWidget(generalGroupBox);

    auto updateGroupBox = new QGroupBox(tr("Update"));

    auto updateGridLayout = new QGridLayout;
    updateGroupBox->setLayout(updateGridLayout);

    updateGridLayout->addWidget(new QLabel(tr("Version:")), 0, 0);
    versionLineEdit = new QLineEdit;
    updateGridLayout->addWidget(versionLineEdit, 0, 1);

    updateGridLayout->addWidget(new QLabel(tr("Date:")), 1, 0);
    dateLineEdit = new QLineEdit;
    updateGridLayout->addWidget(dateLineEdit, 1, 1);

    updateGridLayout->addWidget(new QLabel(tr("OS:")), 2, 0);

    auto osLayout = new QHBoxLayout;
    windowsCheckBox = new QCheckBox("Windows");
    osLayout->addWidget(windowsCheckBox);

    linuxCheckBox = new QCheckBox("Linux");
    osLayout->addWidget(linuxCheckBox);

    macosCheckBox = new QCheckBox("MacOS");
    osLayout->addWidget(macosCheckBox);

    updateGridLayout->addLayout(osLayout, 2, 1, Qt::AlignLeft);

    updateGridLayout->addWidget(new QLabel("Channel:"), 3, 0);
    channelComboBox = new QComboBox;
    channelComboBox->addItem("release");
    channelComboBox->addItem("beta");
    updateGridLayout->addWidget(channelComboBox, 3, 1, Qt::AlignLeft);

    updateGridLayout->addWidget(new QLabel(tr("Size:")), 4, 0);
    sizeLabel = new QLabel;
    updateGridLayout->addWidget(sizeLabel, 4, 1);

    updateGridLayout->addWidget(new QLabel(tr("Description:")), 5, 0);
    descriptionTextEdit = new QPlainTextEdit;
    updateGridLayout->addWidget(descriptionTextEdit, 5, 1);

    updateGridLayout->setRowStretch(2, 0);

    layout->addWidget(updateGroupBox);
    layout->setStretch(1, 0);
}
