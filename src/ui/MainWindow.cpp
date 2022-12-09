#include "MainWindow.h"
#include "Editor.h"
#include "Preferences.h"
#include "core/Constants.h"
#include "core/Exception.h"
#include "outliner/Outliner.h"
#include "database/Database.h"
#include "hotkey/GlobalHotkey.h"
#include <QtWidgets>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(Const::App::Name);
    setWindowIcon(QIcon(":/images/icon.png"));

    m_splitter = new QSplitter;
    setCentralWidget(m_splitter);

    m_database = new Database(this);

    m_globalHotkey = new GlobalHotkey(this);
    connect(m_globalHotkey, &GlobalHotkey::activated, this, &MainWindow::onGlobalActivated);

    createActions();
    createTrayIcon();
    setupSplitter();

    connect(m_outliner, &Outliner::noteChanged, this, &MainWindow::onNoteChanged);
    connect(m_editor, &Editor::focusLost, this, &MainWindow::onEditorFocusLost);
    connect(m_editor, &Editor::leave, this, [=] {
       m_outliner->setFocus();
    });

    readSettings();
    updateMenuState();
}

void MainWindow::readSettings() {
    applyHotSettings();

    QSettings settings;

    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();

    if (geometry.isEmpty()) {
        const QRect availableGeometry = QGuiApplication::screens().constFirst()->availableGeometry();
        resize(availableGeometry.width() / 2, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2, (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }

    m_splitter->restoreState(settings.value("splitter").toByteArray());

    int size = settings.beginReadArray("RecentFiles");

    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        addRecentFile(settings.value("path").toString());
    }

    settings.endArray();

    loadFile(settings.value("filePath").toString());

    if (!settings.value("minimizeOnStartup", false).toBool()) {
        show();
    }
}

void MainWindow::writeSettings() {
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("splitter", m_splitter->saveState());
    settings.setValue("filePath", m_currentFile);

    settings.beginWriteArray("RecentFiles");

    for (int i = 0; i < m_recentFilesMenu->actions().size() - Const::Window::SystemRecentFilesActions; ++i) {
        settings.setArrayIndex(i);
        settings.setValue("path", m_recentFilesMenu->actions().at(i)->text());
    }

    settings.endArray();
}

void MainWindow::applyHotSettings() {
    QSettings settings;
    m_trayIcon->setVisible(!settings.value("hideTrayIcon").toBool());

    if (settings.value("GlobalHotkey/enabled").toBool()) {
        m_globalHotkey->setShortcut(settings.value("GlobalHotkey/hotkey", Const::DefaultSettings::GlobalHotkey).toString());
    } else {
        m_globalHotkey->unsetShortcut();
    }

    QString fontFamily = settings.value("Editor/fontFamily").toString();

    if (!fontFamily.isEmpty()) {
        QFont font;
        font.setFamily(fontFamily);

        QString fontSize = settings.value("Editor/fontSize").toString();

        if (!fontSize.isEmpty()) {
            font.setPointSize(fontSize.toInt());
        }

        m_editor->setFont(font);
    }
}

void MainWindow::setupSplitter() {
    m_outliner = new Outliner(m_database);
    m_editor = new Editor;

    m_splitter->addWidget(m_outliner);
    m_splitter->addWidget(m_editor);

    m_splitter->setHandleWidth(1);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setSizes(QList<int>() << 120 << 500);
}

void MainWindow::createActions() {
    QMenu* fileMenu = menuBar()->addMenu(tr("File"));
    fileMenu->addAction(tr("New..."), Qt::CTRL | Qt::Key_N, this, &MainWindow::onNew);
    fileMenu->addAction(tr("Open..."), Qt::CTRL | Qt::Key_O, this, &MainWindow::onOpen);

    m_recentFilesMenu = new QMenu(tr("Recent Files"), this);
    m_recentFilesMenu->addSeparator();
    m_recentFilesMenu->addAction(tr("Clear"), this, &MainWindow::onClearRecentFiles);
    fileMenu->addAction(m_recentFilesMenu->menuAction());

    m_exportAction = fileMenu->addAction(tr("Export All..."), Qt::CTRL | Qt::Key_E, this, &MainWindow::onExport);
    m_exportAction = fileMenu->addAction(tr("Create Backup..."), this, &MainWindow::onBackup);
    m_closeAction = fileMenu->addAction(tr("Close"), Qt::CTRL | Qt::Key_W, this, &MainWindow::onClose);

    fileMenu->addSeparator();
    fileMenu->addAction(tr("Preferences..."), this, &MainWindow::onPreferences);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Hide"), Qt::Key_Escape, this, &MainWindow::hide);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), Qt::CTRL | Qt::Key_Q, this, &MainWindow::onQuit);

    QMenu* helpMenu = menuBar()->addMenu(tr("Help"));
    helpMenu->addAction(tr("Open download page"), [] {
        QDesktopServices::openUrl(QUrl(Const::App::ReleasesUrl));
    });
    helpMenu->addAction(tr("About %1...").arg(Const::App::Name), this, &MainWindow::onAbout);
}

void MainWindow::createTrayIcon() {
    m_trayIconMenu = new QMenu(this);

    m_trayIconMenu->addAction(tr("Show"), this, &QMainWindow::showNormal);
    m_trayIconMenu->addAction(tr("Hide"), this, &QMainWindow::hide);
    m_trayIconMenu->addSeparator();
    m_trayIconMenu->addAction(tr("Exit"), this, &MainWindow::onQuit);

    m_trayIcon = new QSystemTrayIcon(this);
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::onTrayIconActivated);
    m_trayIcon->setContextMenu(m_trayIconMenu);
    m_trayIcon->setIcon(windowIcon());
    m_trayIcon->show();
}

void MainWindow::updateMenuState() {
    bool isFileOpen = !m_currentFile.isEmpty();

    m_exportAction->setEnabled(isFileOpen);
    m_closeAction->setEnabled(isFileOpen);
}

void MainWindow::loadFile(const QString& filePath) {
    if (filePath.isEmpty() || !QFile::exists(filePath)) return;

    try {
        m_database->open(filePath);
        m_outliner->build();
        setCurrentFile(filePath);
        addRecentFile(filePath);
    } catch (const Exception& e) {
        showErrorDialog(e.error());
    }
}

void MainWindow::setCurrentFile(const QString& filePath) {
    QString title = QApplication::applicationName();

    if (!filePath.isEmpty()) {
        QFileInfo fi(filePath);
        title = title + " - " + fi.fileName();
    }

    setWindowTitle(title);
    m_currentFile = filePath;
    updateMenuState();
}

void MainWindow::addRecentFile(const QString& filePath) {
    if (!QFile::exists(filePath)) return;
    const auto actions = m_recentFilesMenu->actions();

    for (QAction* action : actions) {
        if (action->text() == filePath) {
            m_recentFilesMenu->removeAction(action);
        }
    }

    auto fileAction = new QAction(filePath);
    connect(fileAction, &QAction::triggered, this, [=] {
        loadFile(filePath);
    });

    m_recentFilesMenu->insertAction(m_recentFilesMenu->actions().constFirst(), fileAction);

    if (m_recentFilesMenu->actions().size() > Const::Window::MaxRecentFiles + Const::Window::SystemRecentFilesActions) {
        m_recentFilesMenu->removeAction(m_recentFilesMenu->actions().at(m_recentFilesMenu->actions().size() - Const::Window::SystemRecentFilesActions - 1));
    }

    updateMenuState();
}

void MainWindow::showErrorDialog(const QString& message) {
    QMessageBox::critical(this, tr("Error"), message, QMessageBox::Ok);
}

QString MainWindow::dateFileName(const QString& name) {
    QDateTime dateTime = QDateTime::currentDateTime();
    QFileInfo fi(name);
    return fi.baseName() + "-" + dateTime.date().toString("yyyy-MM-dd") + "_" + dateTime.time().toString("HH-mm-ss") + "." + fi.completeSuffix();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    writeSettings();
    event->accept();
}

void MainWindow::onNew() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("New File"), "notes.db",
                                                    tr("All Files (*);;Database Files (*.db)"));

    if (fileName.isEmpty()) return;

    onClose();

    if (QFile::exists(fileName)) {
        if (!QFile::remove(fileName)) {
            showErrorDialog(tr("Error rewriting old file"));
        }
    }

    try {
        m_database->create(fileName);
        loadFile(fileName);
    } catch (const Exception& e) {
        showErrorDialog(e.error());
    }
}

void MainWindow::onOpen() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                    tr("All Files (*);;Database Files (*.db)"));
    if (!fileName.isEmpty()) {
        onClose();
        loadFile(fileName);
    }
}

void MainWindow::onExport() {
    QSettings settings;
    QString directory = settings.value("Backups/directory").toString();

    QFileInfo fi(m_currentFile);
    QString name = directory + "/" + dateFileName(fi.baseName() + ".zip");
    QString filePath = QFileDialog::getSaveFileName(this, tr("Export notes to ZIP archive"), name);

    if (!filePath.isEmpty()) {
        m_outliner->exportAllNotes(filePath);
    }
}

void MainWindow::onBackup() {
    QSettings settings;
    QString directory = settings.value("Backups/directory").toString();

    QFileInfo fi(m_currentFile);
    QString name = directory + "/" + dateFileName(fi.fileName());

    QString backupFile = QFileDialog::getSaveFileName(this, tr("Create Backup"), name);

    if (!backupFile.isEmpty()) {
        QFile::copy(m_currentFile, backupFile);
    }
}

void MainWindow::onClose() {
    m_database->close();
    onNoteChanged(0);
    m_outliner->clear();
    setCurrentFile();
}

void MainWindow::onClearRecentFiles() {
    for (int i = m_recentFilesMenu->actions().size() - Const::Window::SystemRecentFilesActions - 1; i >= 0; i--) {
        m_recentFilesMenu->removeAction(m_recentFilesMenu->actions().at(i));
    }

    updateMenuState();
}

void MainWindow::onPreferences() {
    Preferences preferences;

    if (preferences.exec() == QDialog::Accepted) {
        applyHotSettings();
    }
}

void MainWindow::onAbout() {
    using namespace Const::App;

    QMessageBox::about(this, tr("About %1").arg(Name),
        tr("<h3>%1 %2 %3</h3>\
           Note-taking for quick notes<br><br> \
           Based on Qt %4<br> \
           Build on %5 %6<br><br> \
           <a href=%7>%7</a><br><br>Copyright © %8, Vladimir Zarypov")
            .arg(Name, Version, Status, QT_VERSION_STR, BuildDate, BuildTime, URL, CopyrightYear));
}

void MainWindow::onQuit() {
    writeSettings();
    QCoreApplication::quit();
}

void MainWindow::onNoteChanged(Id id) {
    m_editor->setId(id);
    m_editor->setEnabled(id > 0);

    if (id) {
        QString note = m_database->value(id, "note").toString();
        m_editor->setPlainText(note);
        m_editor->setFocus();

        int line = m_database->value(id, "line").toInt();
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, line);
        m_editor->setTextCursor(cursor);
    } else {
        m_editor->clear();
    }
}

void MainWindow::onEditorFocusLost() {
    Id lastId = m_editor->id();

    if (!lastId) return;

    if (m_editor->document()->isModified()) {
        m_database->updateValue(lastId, "note", m_editor->document()->toPlainText());
    }

    m_database->updateValue(lastId, "line", m_editor->textCursor().blockNumber());
}

void MainWindow::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {
        setVisible(!isVisible());
    }
}

void MainWindow::onGlobalActivated() {
    show();
    raise();
    activateWindow();
}
