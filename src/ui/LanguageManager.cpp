#include "LanguageManager.h"

#include <QAbstractButton>
#include <QAbstractSpinBox>
#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QDockWidget>
#include <QFile>
#include <QFontDatabase>
#include <QHash>
#include <QGroupBox>
#include <QLibraryInfo>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QSettings>
#include <QStatusBar>
#include <QSpinBox>
#include <QTimer>
#include <QToolBar>
#include <QToolTip>
#include <QTranslator>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVector>
#include <QXmlStreamReader>
#include <QWidget>
#include <algorithm>
#include <utility>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace compositor::ui {
namespace {
struct CatalogEntry {
    QString source;
    QString translated;
    QRegularExpression sourcePattern;
    QRegularExpression translatedPattern;
    QStringList placeholders;
};

QVector<CatalogEntry>& catalog();

QRegularExpression patternFor(const QString& source, QStringList& placeholders) {
    const QRegularExpression token(QStringLiteral("%L?([1-9][0-9]?)"));
    QString pattern;
    int cursor = 0;
    auto matches = token.globalMatch(source);
    while (matches.hasNext()) {
        const auto match = matches.next();
        pattern += QRegularExpression::escape(source.mid(cursor, match.capturedStart() - cursor));
        pattern += QStringLiteral("(.+?)");
        placeholders.append(match.captured(1));
        cursor = match.capturedEnd();
    }
    pattern += QRegularExpression::escape(source.mid(cursor));
    return QRegularExpression(QStringLiteral("^") + pattern + QStringLiteral("$"));
}

QString exactTranslation(const QString& text, bool toChinese) {
    for (const auto& entry : catalog()) {
        if (toChinese && entry.source == text) return entry.translated;
        if (!toChinese && entry.translated == text) return entry.source;
    }
    return text;
}

QVector<CatalogEntry>& catalog() {
    static QVector<CatalogEntry> entries;
    static bool loaded = false;
    if (loaded) return entries;
    loaded = true;
    QFile file(QStringLiteral(":/catalog/Compositor_zh_CN.ts"));
    if (!file.open(QIODevice::ReadOnly)) return entries;
    QXmlStreamReader xml(&file);
    const QRegularExpression placeholder(QStringLiteral("%L?([1-9][0-9]?)"));
    while (!xml.atEnd()) {
        xml.readNext();
        if (!xml.isStartElement() || xml.name() != QStringLiteral("message")) continue;
        QString source, translated;
        bool unfinished = false;
        while (xml.readNextStartElement()) {
            if (xml.name() == QStringLiteral("source")) source = xml.readElementText();
            else if (xml.name() == QStringLiteral("translation")) {
                unfinished = xml.attributes().value(QStringLiteral("type")) == QStringLiteral("unfinished");
                translated = xml.readElementText();
            } else xml.skipCurrentElement();
        }
        if (source.isEmpty() || translated.isEmpty() || unfinished) continue;
        CatalogEntry entry;
        entry.source = source;
        entry.translated = translated;
        QStringList translatedPlaceholders;
        if (source.contains(placeholder)) entry.sourcePattern = patternFor(source, entry.placeholders);
        if (translated.contains(placeholder)) entry.translatedPattern = patternFor(translated, translatedPlaceholders);
        if (!entry.placeholders.isEmpty() && entry.placeholders.size() == translatedPlaceholders.size()) {
            // Both templates retain the same numbered Qt placeholders; their order may differ.
        } else if (!entry.placeholders.isEmpty()) {
            entry.sourcePattern = QRegularExpression();
            entry.translatedPattern = QRegularExpression();
        }
        entries.append(std::move(entry));
    }
    return entries;
}

QString translatePattern(const QString& text, const CatalogEntry& entry, bool toChinese) {
    const auto match = (toChinese ? entry.sourcePattern : entry.translatedPattern).match(text);
    if (!match.hasMatch()) return {};
    const QString sourceTemplate = toChinese ? entry.source : entry.translated;
    const QString targetTemplate = toChinese ? entry.translated : entry.source;
    const QRegularExpression token(QStringLiteral("%L?([1-9][0-9]?)"));
    QStringList sourcePlaceholders, values;
    auto placeholders = token.globalMatch(sourceTemplate);
    while (placeholders.hasNext()) sourcePlaceholders.append(placeholders.next().captured(1));
    const bool commandName = entry.source.startsWith(QStringLiteral("Undo ")) || entry.source.startsWith(QStringLiteral("Redo "));
    for (int i = 0; i < sourcePlaceholders.size(); ++i) {
        auto value = match.captured(i + 1);
        if (commandName) value = exactTranslation(value, toChinese);
        values.append(value);
    }
    QHash<QString, QString> valueByPlaceholder;
    for (int i = 0; i < sourcePlaceholders.size(); ++i) valueByPlaceholder.insert(sourcePlaceholders[i], values.value(i));
    QString result = targetTemplate;
    auto targetMatches = token.globalMatch(targetTemplate);
    QVector<QPair<int, QString>> replacements;
    while (targetMatches.hasNext()) {
        const auto targetMatch = targetMatches.next();
        replacements.append({targetMatch.capturedStart(), valueByPlaceholder.value(targetMatch.captured(1))});
    }
    for (auto it = replacements.crbegin(); it != replacements.crend(); ++it) {
        const auto targetMatch = token.match(result, it->first);
        if (targetMatch.hasMatch()) result.replace(targetMatch.capturedStart(), targetMatch.capturedLength(), it->second);
    }
    return result;
}

QString osDefaultLanguage() {
#ifdef Q_OS_WIN
    wchar_t localeName[LOCALE_NAME_MAX_LENGTH]{};
    if (LCIDToLocaleName(GetUserDefaultUILanguage(), localeName, LOCALE_NAME_MAX_LENGTH, 0)) {
        const auto name = QString::fromWCharArray(localeName).toLower();
        if (name.startsWith(QStringLiteral("zh-cn")) || name.startsWith(QStringLiteral("zh-sg"))) return QStringLiteral("zh_CN");
    }
#endif
    const auto locale = QLocale::system();
    if (locale.language() == QLocale::Chinese &&
        (locale.script() == QLocale::SimplifiedChineseScript || locale.territory() == QLocale::China || locale.territory() == QLocale::Singapore))
        return QStringLiteral("zh_CN");
    for (const auto& uiLanguage : locale.uiLanguages()) {
        const auto candidate = uiLanguage.toLower();
        if (candidate.startsWith(QStringLiteral("zh-hans")) || candidate.startsWith(QStringLiteral("zh-cn")) || candidate.startsWith(QStringLiteral("zh-sg")))
            return QStringLiteral("zh_CN");
    }
    return QStringLiteral("en");
}

}

LanguageManager* manager = nullptr;

void LanguageManager::initialize(QApplication& application) {
    if (manager) return;
    manager = new LanguageManager(application);
    const auto saved = QSettings().value(QStringLiteral("ui/language")).toString();
    manager->activate(saved == QStringLiteral("zh_CN") || saved == QStringLiteral("en") ? saved : osDefaultLanguage());
}

LanguageManager::LanguageManager(QApplication& application)
    : QObject(&application), application_(application), appTranslator_(new QTranslator(this)), qtTranslator_(new QTranslator(this)) {
    originalHanFallbacks_ = QFontDatabase::applicationFallbackFontFamilies(QChar::Script_Han);
    application_.installEventFilter(this);
    auto* timer = new QTimer(this);
    timer->setInterval(250);
    connect(timer, &QTimer::timeout, this, &LanguageManager::translateVisibleWidgets);
    timer->start();
}

QString LanguageManager::language() { return manager ? manager->language_ : QStringLiteral("en"); }

bool LanguageManager::setLanguage(const QString& language, bool remember) {
    if (!manager || (language != QStringLiteral("en") && language != QStringLiteral("zh_CN"))) return false;
    if (remember) {
        QSettings settings;
        settings.setValue(QStringLiteral("ui/language"), language);
        settings.sync();
    }
    manager->activate(language);
    return true;
}

QString LanguageManager::translateText(const QString& text) {
    if (!manager) return text;
    return manager->language_ == QStringLiteral("zh_CN") ? toChinese(text) : toEnglish(text);
}

QString LanguageManager::toEnglish(const QString& text) {
    if (text.isEmpty()) return text;
    const auto& entries = catalog();
    for (const auto& entry : entries) {
        if (entry.translated == text) return entry.source;
    }
    for (const auto& entry : entries) {
        if (entry.sourcePattern.pattern().isEmpty()) continue;
        const auto source = translatePattern(text, entry, false);
        if (!source.isEmpty()) return source;
    }
    return text;
}

QString LanguageManager::toChinese(const QString& text) {
    if (text.isEmpty()) return text;
    const auto& entries = catalog();
    for (const auto& entry : entries) if (entry.source == text) return entry.translated;
    for (const auto& entry : entries) {
        if (entry.sourcePattern.pattern().isEmpty()) continue;
        const auto translated = translatePattern(text, entry, true);
        if (!translated.isEmpty()) return translated;
    }
    return text;
}

void LanguageManager::activate(const QString& language) {
    language_ = language;
    if (language_ == QStringLiteral("zh_CN")) {
        QStringList fallbackFonts;
        const auto installedFonts = QFontDatabase::families();
        for (const auto& candidate : {QStringLiteral("Microsoft YaHei UI"), QStringLiteral("Microsoft YaHei"), QStringLiteral("SimSun")})
            if (installedFonts.contains(candidate, Qt::CaseInsensitive)) fallbackFonts.append(candidate);
        QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han, fallbackFonts.isEmpty() ? originalHanFallbacks_ : fallbackFonts);
    } else {
        QFontDatabase::setApplicationFallbackFontFamilies(QChar::Script_Han, originalHanFallbacks_);
    }
    application_.setProperty("ui.language", language_);
    (void)application_.removeTranslator(qtTranslator_);
    (void)application_.removeTranslator(appTranslator_);
    if (language_ == QStringLiteral("zh_CN")) {
        (void)appTranslator_->load(QStringLiteral(":/i18n/Compositor_zh_CN.qm"));
        if (QFile::exists(QCoreApplication::applicationDirPath() + QStringLiteral("/translations/qtbase_zh_CN.qm")))
            (void)qtTranslator_->load(QCoreApplication::applicationDirPath() + QStringLiteral("/translations/qtbase_zh_CN.qm"));
        else
            (void)qtTranslator_->load(QStringLiteral("qtbase_zh_CN"), QLibraryInfo::path(QLibraryInfo::TranslationsPath));
        application_.installTranslator(qtTranslator_);
        application_.installTranslator(appTranslator_);
    }
    for (QWidget* window : QApplication::topLevelWidgets()) {
        QEvent event(QEvent::LanguageChange);
        QApplication::sendEvent(window, &event);
        translateTree(window);
    }
}

void LanguageManager::translateVisibleWidgets() {
    for (QWidget* window : QApplication::topLevelWidgets()) if (window->isVisible()) translateTree(window);
}

void LanguageManager::translateTree(QObject* root) {
    if (!root) return;
    translateObject(root);
    for (QObject* child : root->children()) translateTree(child);
}

void LanguageManager::translateObject(QObject* object) {
    const auto localize = [this](QObject* target, const char* role, const QString& text, auto&& setter) {
        if (text.isEmpty()) return;
        const QByteArray sourceKey = QByteArrayLiteral("compositorLanguageSource_") + role;
        const QByteArray appliedKey = QByteArrayLiteral("compositorLanguageApplied_") + role;
        const auto storedSource = target->property(sourceKey.constData());
        const auto lastApplied = target->property(appliedKey.constData());
        const QString source = storedSource.isValid() && lastApplied.isValid() && text == lastApplied.toString()
            ? storedSource.toString() : toEnglish(text);
        const QString localized = language_ == QStringLiteral("zh_CN") ? toChinese(source) : source;
        target->setProperty(sourceKey.constData(), source);
        target->setProperty(appliedKey.constData(), localized);
        if (localized != text) setter(localized);
    };
    if (auto* action = qobject_cast<QAction*>(object)) {
        localize(action, "actionText", action->text(), [action](const QString& text) { action->setText(text); });
        localize(action, "actionToolTip", action->toolTip(), [action](const QString& text) { action->setToolTip(text); });
        localize(action, "actionStatusTip", action->statusTip(), [action](const QString& text) { action->setStatusTip(text); });
    }
    auto* widget = qobject_cast<QWidget*>(object);
    if (!widget) return;
    localize(widget, "toolTip", widget->toolTip(), [widget](const QString& text) { widget->setToolTip(text); });
    localize(widget, "statusTip", widget->statusTip(), [widget](const QString& text) { widget->setStatusTip(text); });
    localize(widget, "accessibleName", widget->accessibleName(), [widget](const QString& text) { widget->setAccessibleName(text); });
    localize(widget, "accessibleDescription", widget->accessibleDescription(), [widget](const QString& text) { widget->setAccessibleDescription(text); });
    localize(widget, "windowTitle", widget->windowTitle(), [widget](const QString& text) { widget->setWindowTitle(text); });
    if (auto* message = qobject_cast<QMessageBox*>(widget)) {
        localize(message, "messageText", message->text(), [message](const QString& text) { message->setText(text); });
        localize(message, "messageInformativeText", message->informativeText(), [message](const QString& text) { message->setInformativeText(text); });
        localize(message, "messageDetailedText", message->detailedText(), [message](const QString& text) { message->setDetailedText(text); });
    }
    if (auto* button = qobject_cast<QAbstractButton*>(widget)) {
        localize(button, "buttonText", button->text(), [button](const QString& text) { button->setText(text); });
    }
    if (auto* label = qobject_cast<QLabel*>(widget)) {
        localize(label, "labelText", label->text(), [label](const QString& text) { label->setText(text); });
    }
    if (auto* group = qobject_cast<QGroupBox*>(widget)) {
        localize(group, "groupTitle", group->title(), [group](const QString& text) { group->setTitle(text); });
    }
    if (auto* menu = qobject_cast<QMenu*>(widget)) {
        localize(menu, "menuTitle", menu->title(), [menu](const QString& text) { menu->setTitle(text); });
    }
    if (auto* bar = qobject_cast<QToolBar*>(widget)) {
        localize(bar, "toolBarTitle", bar->windowTitle(), [bar](const QString& text) { bar->setWindowTitle(text); });
    }
    if (auto* line = qobject_cast<QLineEdit*>(widget)) {
        localize(line, "lineEditPlaceholder", line->placeholderText(), [line](const QString& text) { line->setPlaceholderText(text); });
    }
    if (auto* combo = qobject_cast<QComboBox*>(widget)) {
        localize(combo, "comboPlaceholder", combo->placeholderText(), [combo](const QString& text) { combo->setPlaceholderText(text); });
        if (!combo->isEditable() && !combo->property("dynamicChoices").toBool()) {
            const QSignalBlocker blocker(combo);
            for (int i = 0; i < combo->count(); ++i) {
                constexpr int sourceTextRole = Qt::UserRole + 1001;
                auto sourceText = combo->itemData(i, sourceTextRole).toString();
                if (sourceText.isEmpty()) {
                    sourceText = toEnglish(combo->itemText(i));
                    combo->setItemData(i, sourceText, sourceTextRole);
                }
                const auto item = language_ == QStringLiteral("zh_CN") ? toChinese(sourceText) : sourceText;
                if (item != combo->itemText(i)) combo->setItemText(i, item);
            }
        }
    }
    if (auto* spin = qobject_cast<QSpinBox*>(widget)) {
        localize(spin, "spinPrefix", spin->prefix(), [spin](const QString& text) { spin->setPrefix(text); });
        localize(spin, "spinSuffix", spin->suffix(), [spin](const QString& text) { spin->setSuffix(text); });
    }
    if (auto* spin = qobject_cast<QDoubleSpinBox*>(widget)) {
        localize(spin, "spinPrefix", spin->prefix(), [spin](const QString& text) { spin->setPrefix(text); });
        localize(spin, "spinSuffix", spin->suffix(), [spin](const QString& text) { spin->setSuffix(text); });
    }
    if (auto* tree = qobject_cast<QTreeWidget*>(widget)) {
        for (int index = 0; index < tree->topLevelItemCount(); ++index) translateTreeItem(tree->topLevelItem(index));
    }
}

void LanguageManager::translateTreeItem(QTreeWidgetItem* item) {
    if (!item) return;
    constexpr int sourceToolTipRole = Qt::UserRole + 0x5100;
    constexpr int appliedToolTipRole = Qt::UserRole + 0x5101;
    constexpr int sourceAccessibleTextRole = Qt::UserRole + 0x5102;
    constexpr int appliedAccessibleTextRole = Qt::UserRole + 0x5103;
    constexpr int sourceAccessibleDescriptionRole = Qt::UserRole + 0x5104;
    constexpr int appliedAccessibleDescriptionRole = Qt::UserRole + 0x5105;
    const auto apply = [this, item](int column, int sourceRole, int appliedRole, const QString& text, auto&& setter) {
        if (text.isEmpty()) return;
        const auto storedSource = item->data(column, sourceRole);
        const auto lastApplied = item->data(column, appliedRole);
        const QString source = storedSource.isValid() && lastApplied.isValid() && text == lastApplied.toString()
            ? storedSource.toString() : toEnglish(text);
        const QString localized = language_ == QStringLiteral("zh_CN") ? toChinese(source) : source;
        item->setData(column, sourceRole, source);
        item->setData(column, appliedRole, localized);
        if (localized != text) setter(localized);
    };
    for (int column = 0; column < item->columnCount(); ++column) {
        apply(column, sourceToolTipRole, appliedToolTipRole, item->toolTip(column), [item, column](const QString& text) { item->setToolTip(column, text); });
        apply(column, sourceAccessibleTextRole, appliedAccessibleTextRole, item->data(column, Qt::AccessibleTextRole).toString(), [item, column](const QString& text) { item->setData(column, Qt::AccessibleTextRole, text); });
        apply(column, sourceAccessibleDescriptionRole, appliedAccessibleDescriptionRole, item->data(column, Qt::AccessibleDescriptionRole).toString(), [item, column](const QString& text) { item->setData(column, Qt::AccessibleDescriptionRole, text); });
    }
    for (int index = 0; index < item->childCount(); ++index) translateTreeItem(item->child(index));
}

bool LanguageManager::eventFilter(QObject* watched, QEvent* event) {
    if (event->type() == QEvent::Show) {
        QPointer<QObject> target(watched);
        QTimer::singleShot(0, this, [this, target] { if (target) translateTree(target); });
    }
    return QObject::eventFilter(watched, event);
}

}
