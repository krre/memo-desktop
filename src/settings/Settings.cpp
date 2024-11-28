#include "Settings.h"

void Settings::setApplicationLanguage(const QString& lang) {
    setValue("Application/language", lang);
}

QString Settings::applicationLanguage() const {
    return value("Application/language").toString();
}

void Settings::setApplicationFilePath(const QString& filePath) {
    setValue("Application/filePath", filePath);
}

QString Settings::applicationFilePath() const {
    return value("Application/filePath").toString();
}

void Settings::setApplicationMinimizeOnStartup(bool minimizeOnStartup) {
    setValue("Application/minimizeOnStartup", minimizeOnStartup);
}

bool Settings::applicationMinimizeOnStartup() const {
    return value("Application/minimizeOnStartup").toBool();
}

void Settings::setApplicationHideTrayIcon(bool hideTrayIcon) {
    setValue("Application/hideTrayIcon", hideTrayIcon);
}

bool Settings::applicationHideTrayIcon() const {
    return value("Application/hideTrayIcon").toBool();
}

void Settings::setMainWindowGeometry(const QByteArray& geometry) {
    setValue("MainWindow/geometry", geometry);
}

QByteArray Settings::mainWindowGeometry() const {
    return value("MainWindow/geometry").toByteArray();
}

void Settings::setMainWindowState(const QByteArray& state) {
    setValue("MainWindow/state", state);
}

QByteArray Settings::mainWindowState() const {
    return value("MainWindow/state").toByteArray();
}

void Settings::setMainWindowSplitter(const QByteArray& splitter) {
    setValue("MainWindow/splitter", splitter);
}

QByteArray Settings::mainWindowSplitter() const {
    return value("MainWindow/splitter").toByteArray();
}

void Settings::setBirthdaysGeometry(const QByteArray& geometry) {
    setValue("Birthdays/geometry", geometry);
}

QByteArray Settings::birthdaysGeometry() const {
    return value("Birthdays/geometry").toByteArray();
}

void Settings::setBackupsDirectory(const QString& directory) {
    setValue("Backups/directory", directory);
}

QString Settings::backupsDirectory() const {
    return value("Backups/directory").toString();
}

void Settings::setEditorFontFamily(const QString& fontFamily) {
    setValue("Editor/fontFamily", fontFamily);
}

QString Settings::editorFontFamily() const {
    return value("Editor/fontFamily").toString();
}

void Settings::setEditorFontSize(int fontSize) {
    setValue("Editor/fontSize", fontSize);
}

int Settings::editorFontSize() const {
    return value("Editor/fontSize").toInt();
}

void Settings::setGlobalHotkeyEnabled(bool enabled) {
    setValue("GlobalHotkey/enabled", enabled);
}

bool Settings::globalHotkeyEnabled() const {
    return value("GlobalHotkey/enabled").toBool();
}

void Settings::setGlobalHotkeyValue(const QString& value) {
    setValue("GlobalHotkey/value", value);
}

QString Settings::globalHotkeyValue() const {
    return value("GlobalHotkey/value").toString();
}
