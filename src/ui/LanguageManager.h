#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class QApplication;
class QEvent;
class QTranslator;
class QTreeWidget;
class QTreeWidgetItem;

namespace compositor::ui {

class LanguageManager final : public QObject {
public:
    static void initialize(QApplication& application);
    static QString language();
    static bool setLanguage(const QString& language, bool remember = true);
    static QString translateText(const QString& text);
    static QString toEnglish(const QString& text);
    static QString toChinese(const QString& text);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    explicit LanguageManager(QApplication& application);
    void activate(const QString& language);
    void translateVisibleWidgets();
    void translateTree(QObject* root);
    void translateObject(QObject* object);
    void translateTreeItem(QTreeWidgetItem* item);

    QApplication& application_;
    QTranslator* appTranslator_{};
    QTranslator* qtTranslator_{};
    QStringList originalHanFallbacks_;
    QString language_{QStringLiteral("en")};
};

}
