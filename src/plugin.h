/*
    SPDX-FileCopyrightText: 2025 Daniele Mte90 Scasciafratte <mte90net@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KATEOLLAMAPLUGIN_H
#define KATEOLLAMAPLUGIN_H

// KF headers
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/SessionConfigInterface>
#include <KTextEditor/View>
#include <KXMLGUIClient>
#include <QMap>
#include <QPointer>
#include <QString>

class KateOllamaPlugin : public KTextEditor::Plugin
{
    Q_OBJECT

public:
    explicit KateOllamaPlugin(QObject *parent, const QVariantList & = QVariantList());

    QObject *createView(KTextEditor::MainWindow *mainWindow) override;

    void readSettings();

    int configPages() const override
    {
        return 1;
    }

    KTextEditor::ConfigPage *configPage(int number = 0, QWidget *parent = nullptr) override;

    QString model;
    QString systemPrompt;
    QString ollamaURL;
    bool responseToNamedDoc = false;
    QString responseDocName = QStringLiteral("AI Response");
};
class KateOllamaView : public QObject, public KXMLGUIClient
{
    Q_OBJECT
public:
    explicit KateOllamaView(KateOllamaPlugin *plugin, KTextEditor::MainWindow *mainwindow);
    ~KateOllamaView();

private slots:
    void onSinglePrompt();
    void onFullPrompt();
    void ollamaRequest(QString prompt);
    void printCommand();
    QString getPrompt();

private:
    KateOllamaPlugin *m_plugin = nullptr;
    KTextEditor::MainWindow *m_mainWindow = nullptr;
    QMap<QString, QPointer<KTextEditor::Document>> m_namedDocs;
};
#endif // KATEOLLAMAPLUGIN_H
