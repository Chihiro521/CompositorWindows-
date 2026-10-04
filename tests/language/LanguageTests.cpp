#include "ui/LanguageManager.h"

#include <QAbstractButton>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLocale>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QTemporaryDir>
#include <QWidget>
#include <QTest>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <algorithm>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setOrganizationName("Compositor Windows Language Tests");
    app.setApplicationName("Compositor Language Tests");
    QTemporaryDir settingsRoot;
    if (!settingsRoot.isValid()) return 2;
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsRoot.path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, settingsRoot.path());
    const bool savedPreferenceMode = app.arguments().contains(QStringLiteral("--saved-preference"));
    QSettings startupSettings;
    startupSettings.remove(QStringLiteral("ui/language"));
    if (savedPreferenceMode) startupSettings.setValue(QStringLiteral("ui/language"), QStringLiteral("en"));
    startupSettings.sync();
    compositor::ui::LanguageManager::initialize(app);
    if (savedPreferenceMode) {
        require(compositor::ui::LanguageManager::language() == QStringLiteral("en"), "Saved English preference did not override the Windows display language");
    } else {
        wchar_t localeName[LOCALE_NAME_MAX_LENGTH]{};
        const bool windowsSimplifiedChinese = LCIDToLocaleName(GetUserDefaultUILanguage(), localeName, LOCALE_NAME_MAX_LENGTH, 0) &&
            (QString::fromWCharArray(localeName).compare(QStringLiteral("zh-CN"), Qt::CaseInsensitive) == 0 ||
             QString::fromWCharArray(localeName).compare(QStringLiteral("zh-SG"), Qt::CaseInsensitive) == 0);
        const auto systemLocale = QLocale::system();
        const auto uiLanguages = systemLocale.uiLanguages();
        const bool localeSimplifiedChinese = systemLocale.language() == QLocale::Chinese &&
            (systemLocale.script() == QLocale::SimplifiedChineseScript || systemLocale.territory() == QLocale::China || systemLocale.territory() == QLocale::Singapore);
        const bool expectedChinese = windowsSimplifiedChinese || localeSimplifiedChinese || std::any_of(uiLanguages.begin(), uiLanguages.end(), [](const QString& value) {
            const auto candidate = value.toLower();
            return candidate.startsWith(QStringLiteral("zh-hans")) || candidate.startsWith(QStringLiteral("zh-cn")) || candidate.startsWith(QStringLiteral("zh-sg"));
        });
        require(compositor::ui::LanguageManager::language() == (expectedChinese ? QStringLiteral("zh_CN") : QStringLiteral("en")), "First-run language did not follow the Windows Simplified Chinese display language");
    }
    compositor::ui::LanguageManager::setLanguage(QStringLiteral("en"), false);

    QWidget window;
    auto* label = new QLabel(QStringLiteral("File"), &window);
    auto* staticChoices = new QComboBox(&window);
    staticChoices->addItem(QStringLiteral("Normal"));
    auto* projectChoices = new QComboBox(&window);
    projectChoices->setProperty("dynamicChoices", true);
    projectChoices->addItem(QStringLiteral("Background"));
    auto* userInput = new QLineEdit(&window);
    userInput->setText(QStringLiteral("My layer name"));
    auto* action = new QAction(QStringLiteral("Close Project"), &window);
    window.show();
    app.processEvents();

    require(compositor::ui::LanguageManager::setLanguage(QStringLiteral("zh_CN")), "Chinese language switch failed");
    require(label->text() == QStringLiteral("文件"), "Label did not switch to Simplified Chinese");
    require(staticChoices->itemText(0) == QStringLiteral("正常"), "Static combo choice did not switch to Simplified Chinese");
    require(projectChoices->itemText(0) == QStringLiteral("Background"), "Project data in a dynamic combo was translated");
    require(userInput->text() == QStringLiteral("My layer name"), "User text changed during language switch");
    require(action->text() == QStringLiteral("关闭项目"), "Action text did not switch to Simplified Chinese");
    auto* lateLabel = new QLabel(QStringLiteral("Red"), &window);
    lateLabel->show();
    QTest::qWait(30);
    require(lateLabel->text() == QStringLiteral("红色"), "A newly shown widget did not use the active language");
    auto* message = new QMessageBox(QMessageBox::Warning, QStringLiteral("Unsaved Changes"), QStringLiteral("Save changes before closing this project?"), QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, &window);
    message->show();
    QTest::qWait(30);
    require(message->windowTitle() == QStringLiteral("未保存的更改") && message->text() == QStringLiteral("关闭此项目前要保存更改吗？"), "Message box content did not follow the active language");
    require(message->button(QMessageBox::Save)->text() == QStringLiteral("保存"), "Qt standard message box buttons did not use the Qt Simplified Chinese catalog");
    require(QSettings().value(QStringLiteral("ui/language")).toString() == QStringLiteral("zh_CN"), "Language choice was not remembered");
    require(compositor::ui::LanguageManager::translateText(QStringLiteral("Close My Project")) == QStringLiteral("关闭My Project"), "Parameterized tooltip did not translate");
    require(compositor::ui::LanguageManager::translateText(QStringLiteral("Undo Crop")) == QStringLiteral("撤销裁剪"), "Parameterized undo label did not translate its operation name");
    require(compositor::ui::LanguageManager::translateText(QStringLiteral("Click the canvas to sample")) == QStringLiteral("单击画布以采样"), "Palette instruction did not translate");
    require(compositor::ui::LanguageManager::translateText(QStringLiteral("Add Noise…")) == QStringLiteral("添加噪点…"), "Filter menu text did not translate");
    require(compositor::ui::LanguageManager::translateText(QStringLiteral("Reveal All")) == QStringLiteral("全部显示"), "Mask menu text did not translate");

    require(compositor::ui::LanguageManager::setLanguage(QStringLiteral("en")), "English language switch failed");
    QTest::qWait(30);
    require(label->text() == QStringLiteral("File"), "Label did not return to English");
    require(staticChoices->itemText(0) == QStringLiteral("Normal"), "Static combo choice did not return to English");
    require(projectChoices->itemText(0) == QStringLiteral("Background"), "Project combo text changed after returning to English");
    require(userInput->text() == QStringLiteral("My layer name"), "User text changed when returning to English");
    require(action->text() == QStringLiteral("Close Project"), "Action text did not return to English");
    require(lateLabel->text() == QStringLiteral("Red"), "A dynamically added widget did not return to English");
    require(message->windowTitle() == QStringLiteral("Unsaved Changes") && message->text() == QStringLiteral("Save changes before closing this project?") && message->button(QMessageBox::Save)->text() == QStringLiteral("Save"), "Message box content did not return to English");
    require(compositor::ui::LanguageManager::translateText(QStringLiteral("撤销裁剪")) == QStringLiteral("Undo Crop"), "Parameterized undo label did not return to English");
    require(compositor::ui::LanguageManager::translateText(QStringLiteral("添加噪点…")) == QStringLiteral("Add Noise…"), "Filter menu text did not return to English");
    require(QSettings().value(QStringLiteral("ui/language")).toString() == QStringLiteral("en"), "English language choice was not remembered");
    return 0;
}
