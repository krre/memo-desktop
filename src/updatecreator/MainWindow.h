#pragma once
#include <QMainWindow>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    virtual ~MainWindow() override = default;

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void readSettings();
    void writeSettings();
};