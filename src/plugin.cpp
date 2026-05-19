/*
    SPDX-FileCopyrightText: 2025 Daniele Mte90 Scasciafratte <mte90net@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "plugin.h"
#include "settings.h"

// KF headers
#include <KActionCollection>
#include <KConfigGroup>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KTextEditor/Application>
#include <KTextEditor/Document>
#include <KTextEditor/Editor>
#include <KTextEditor/MainWindow>
#include <KTextEditor/Plugin>
#include <KTextEditor/View>
#include <KXMLGUIClient>
#include <KXMLGUIFactory>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QString>

K_PLUGIN_FACTORY_WITH_JSON(KateOllamaFactory, "kateollama.json", registerPlugin<KateOllamaPlugin>();)

enum MessageType {
    Log,
    Info,
    Warn,
    Error
};

static void showMessage(const QString &message, MessageType type, KTextEditor::MainWindow *mainWindow)
{
    QVariantMap msg;
    static const QString msgToString[] = {
        QStringLiteral("Log"),
        QStringLiteral("Info"),
        QStringLiteral("Warning"),
        QStringLiteral("Error"),
    };
    msg.insert(QStringLiteral("type"), msgToString[type]);
    msg.insert(QStringLiteral("category"), QStringLiteral("Kate Ollama"));
    msg.insert(QStringLiteral("categoryIcon"), QIcon()); // todo icon
    msg.insert(QStringLiteral("text"), message);

    // Shows the message in Output pane
    mainWindow->showMessage(msg);
}

KateOllamaPlugin::KateOllamaPlugin(QObject *parent, const QVariantList &)
    : KTextEditor::Plugin(parent)
{
}

KateOllamaView::KateOllamaView(KateOllamaPlugin *plugin, KTextEditor::MainWindow *mainwindow)
    : KXMLGUIClient()
    , m_plugin(plugin)
    , m_mainWindow(mainwindow)
{
    KXMLGUIClient::setComponentName(QStringLiteral("kateollama"), i18n("Kate-Ollama"));
    KConfigGroup group(KSharedConfig::openConfig(), "KateOllama");

    m_plugin->model = group.readEntry("Model");
    m_plugin->systemPrompt = group.readEntry("SystemPrompt");
    m_plugin->ollamaURL = group.readEntry("URL");
    m_plugin->responseToNamedDoc = group.readEntry("ResponseToNamedDoc", false);
    m_plugin->responseDocName = group.readEntry("ResponseDocName", QStringLiteral("AI Response"));

    auto ac = actionCollection();
    QAction *a = ac->addAction(QStringLiteral("kateollama"));
    a->setText(i18n("Run Ollama"));
    a->setIcon(QIcon::fromTheme(QStringLiteral("debug-run")));
    ac->setDefaultShortcut(a, QKeySequence(Qt::CTRL | Qt::Key_Semicolon));
    connect(a, &QAction::triggered, this, &KateOllamaView::onSinglePrompt);

    QAction *a2 = ac->addAction(QStringLiteral("kateollama-full-prompt"));
    a2->setText(i18n("Run Ollama Full Text"));
    a2->setIcon(QIcon::fromTheme(QStringLiteral("debug-run")));
    ac->setDefaultShortcut(a2, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Semicolon));
    connect(a2, &QAction::triggered, this, &KateOllamaView::onFullPrompt);

    QAction *a3 = ac->addAction(QStringLiteral("kateollama-command"));
    a3->setText(i18n("Add Ollama Command"));
    ac->setDefaultShortcut(a3, QKeySequence(Qt::CTRL | Qt::Key_Slash));
    connect(a3, &QAction::triggered, this, &KateOllamaView::printCommand);

    m_mainWindow->guiFactory()->addClient(this);
}

void KateOllamaView::printCommand()
{
    KTextEditor::View *view = m_mainWindow->activeView();
    ;
    if (view) {
        KTextEditor::Document *document = view->document();
        QString text = document->text();
        KTextEditor::Cursor cursor = view->cursorPosition();
        document->insertText(cursor, "// AI: ");
    }
}

void KateOllamaView::ollamaRequest(QString prompt)
{
    KTextEditor::View *targetView = nullptr;
    KTextEditor::Document *targetDoc = nullptr;

    if (!m_plugin->responseToNamedDoc) {
        targetView = m_mainWindow->activeView();
        if (!targetView) return;
        targetDoc = targetView->document();
    } else {
        const QString targetName = m_plugin->responseDocName;

        // Check our cached map first (handles docs created by this plugin)
        if (m_namedDocs.contains(targetName) && m_namedDocs[targetName]) {
            targetDoc = m_namedDocs[targetName];
            m_mainWindow->activateView(targetDoc);
            targetView = m_mainWindow->activeView();
        }

        // Search all open documents by name
        if (!targetDoc) {
            const auto docs = KTextEditor::Editor::instance()->documents();
            for (auto *doc : docs) {
                if (doc->documentName() == targetName) {
                    targetDoc = doc;
                    m_namedDocs[targetName] = doc;
                    m_mainWindow->activateView(targetDoc);
                    targetView = m_mainWindow->activeView();
                    break;
                }
            }
        }

        // Create a new document named after targetName.
        // Opening a non-existent local path gives the document that filename as its title.
        if (!targetDoc) {
            QUrl namedUrl = QUrl::fromLocalFile(QDir::tempPath() + QLatin1Char('/') + targetName);
            targetView = m_mainWindow->openUrl(namedUrl);
            if (!targetView) return;
            targetDoc = targetView->document();
            m_namedDocs[targetName] = targetDoc;
        }
    }

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);

    QNetworkRequest request(QUrl(m_plugin->ollamaURL + "/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json.insert("model", m_plugin->model);
    json.insert("system", m_plugin->systemPrompt);
    json.insert("prompt", prompt);
    QJsonDocument doc(json);

    QNetworkReply *reply = manager->post(request, doc.toJson());

    connect(reply, &QNetworkReply::metaDataChanged, this, [=]() {
        KTextEditor::Cursor cursor = targetView->cursorPosition();
        targetDoc->insertText(cursor, "\n");
        showMessage(QStringLiteral("Info: Request started..."), MessageType::Info, m_mainWindow);
    });

    connect(reply, &QNetworkReply::readyRead, this, [=]() {
        QString responseChunk = reply->readAll();
        QJsonDocument jsonDoc = QJsonDocument::fromJson(responseChunk.toUtf8());
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj.contains("response")) {
            QString responseText = jsonObj["response"].toString();
            KTextEditor::Cursor cursor = targetView->cursorPosition();
            targetDoc->insertText(cursor, responseText);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() != QNetworkReply::NoError) {
            showMessage(QStringLiteral("Error: ").arg(reply->errorString()), MessageType::Error, m_mainWindow);
            qDebug() << "Error:" << reply->errorString();
            qDebug() << "Model:" << m_plugin->model;
            qDebug() << "System prompt:" << m_plugin->systemPrompt;
        }
        reply->deleteLater();

        KTextEditor::Cursor cursor = targetView->cursorPosition();
        targetDoc->insertText(cursor, "\n");

        m_mainWindow->activateView(targetDoc);
    });
}

QString KateOllamaView::getPrompt()
{
    KTextEditor::View *view = m_mainWindow->activeView();
    KTextEditor::Document *document = view->document();
    QString text = document->text();

    QRegularExpression re("// AI:(.*)");
    QRegularExpressionMatchIterator matchIterator = re.globalMatch(text);

    QString lastMatch;

    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        lastMatch = match.captured(1).trimmed();
    }
    qDebug() << "Ollama prompt:" << lastMatch;

    return lastMatch;
}

void KateOllamaView::onSinglePrompt()
{
    KTextEditor::View *view = m_mainWindow->activeView();
    if (view) {
        QString prompt = KateOllamaView::getPrompt();
        if (!prompt.isEmpty()) {
            KateOllamaView::ollamaRequest(prompt);
        }
    }
}

void KateOllamaView::onFullPrompt()
{
    KTextEditor::View *view = m_mainWindow->activeView();
    KTextEditor::Document *document = view->document();
    QString text = document->text();
    if (view) {
        QString prompt = KateOllamaView::getPrompt();
        if (!prompt.isEmpty()) {
            KateOllamaView::ollamaRequest(text + "\n" + prompt);
        }
    }
}

KateOllamaView::~KateOllamaView()
{
    m_mainWindow->guiFactory()->removeClient(this);
}

QObject *KateOllamaPlugin::createView(KTextEditor::MainWindow *mainwindow)
{
    return new KateOllamaView(this, mainwindow);
}

KTextEditor::ConfigPage *KateOllamaPlugin::configPage(int number, QWidget *parent)
{
    if (number != 0) {
        return nullptr;
    }
    return new KateOllamaConfigPage(parent, this);
}

#include <plugin.moc>
