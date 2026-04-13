/*
 *  SPDX-FileCopyrightText: 2025 Daniele Mte90 Scasciafratte <mte90net@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "settings.h"
#include "plugin.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>
#include <KTextEditor/ConfigPage>

#include <QComboBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QVBoxLayout>
#include <QList>
#include <algorithm>

KateOllamaConfigPage::KateOllamaConfigPage(QWidget *parent, KateOllamaPlugin *plugin)
    : KTextEditor::ConfigPage(parent)
    , m_plugin(plugin)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    
    // URL
    {
        auto *hl = new QHBoxLayout;

        auto label = new QLabel(i18n("Ollama URL"));
        hl->addWidget(label);

        m_ollamaURLText = new QLineEdit(this);
        hl->addWidget(m_ollamaURLText);

        layout->addLayout(hl);
    }

    // Available Models
    {
        auto *hl = new QHBoxLayout;

        auto label = new QLabel(i18n("Available Models"));
        hl->addWidget(label);

        m_modelsComboBox = new QComboBox(this);
        hl->addWidget(m_modelsComboBox);

        layout->addLayout(hl);
    }

    // System Prompt
    {
        auto *hl = new QHBoxLayout;

        auto label = new QLabel(i18n("System Prompt"));
        hl->addWidget(label);

        m_systemPromptEdit = new QTextEdit(this);
        m_systemPromptEdit->setGeometry(100, 100, 300, 200);
        hl->addWidget(m_systemPromptEdit);

        layout->addLayout(hl);
    }

    layout->addStretch();

    // Error/Info label
    {
        m_infoLabel = new QLabel(this);
        m_infoLabel->setVisible(false); // its hidden initially
        m_infoLabel->setWordWrap(true);
        layout->addWidget(m_infoLabel);
    }

    setLayout(layout);
    
    loadSettings();
    QObject::connect(m_modelsComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &KateOllamaConfigPage::changed);
    QObject::connect(m_systemPromptEdit, &QTextEdit::textChanged, this, &KateOllamaConfigPage::changed);
    QObject::connect(m_ollamaURLText, &QLineEdit::textEdited, this, &KateOllamaConfigPage::changed);
}

void KateOllamaConfigPage::fetchModelList()
{
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [this](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            m_infoLabel->setVisible(false); // Hide the label on success

            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

            if (jsonDoc.isObject()) {
                QJsonObject jsonObj = jsonDoc.object();
                if (jsonObj.contains("models") && jsonObj["models"].isArray()) {
                    QJsonArray modelsArray = jsonObj["models"].toArray();
                    QList<QJsonValue> modelsList;
                    for (const QJsonValue &value : modelsArray) {
                        modelsList.append(value);
                    }
                    std::sort(modelsList.begin(), modelsList.end(), [](const QJsonValue &a, const QJsonValue &b) {
                        return a.toObject()["name"].toString().toLower() < b.toObject()["name"].toString().toLower();
                    });
                    
                    int modelSelected = -1;
                    for (const QJsonValue &modelValue : modelsList) {
                        QJsonObject modelObj = modelValue.toObject();
                        if (modelObj.contains("name")) {
                            m_modelsComboBox->addItem(modelObj["name"].toString());
                        }
                        
                        if (modelObj["name"].toString() == m_plugin->model) {
                            modelSelected = m_modelsComboBox->count();
                        }
                    }
                    
                    if (modelSelected != -1) {
                        m_modelsComboBox->setCurrentIndex(modelSelected - 1);
                    }
                }
            }
        } else {
            qWarning() << "Error fetching model list:" << reply->errorString();
            // Show error in UI
            m_infoLabel->setText(i18n("Error fetching model list: %1", reply->errorString()));
        }
        reply->deleteLater();
    });

    QUrl url(m_ollamaURLText->text() + "/api/tags");
    QNetworkRequest request(url);
    manager->get(request);

    m_infoLabel->setText(i18n("Loading model list..."));
    m_infoLabel->setVisible(true);
}

QString KateOllamaConfigPage::name() const
{
    return i18n("Ollama");
}

QString KateOllamaConfigPage::fullName() const
{
    return i18nc("Groupbox title", "Ollama Settings");
}

QIcon KateOllamaConfigPage::icon() const
{
    return QIcon::fromTheme(QLatin1String("project-open"), QIcon::fromTheme(QLatin1String("view-list-tree")));
}

void KateOllamaConfigPage::apply()
{
    // Save settings to disk
    KConfigGroup group(KSharedConfig::openConfig(), "KateOllama");
    group.writeEntry("Model", m_modelsComboBox->currentText());
    group.writeEntry("URL", m_ollamaURLText->text());
    group.writeEntry("SystemPrompt", m_systemPromptEdit->toPlainText());
    group.sync();

    // Update the cached variables in Plugin
    m_plugin->model = m_modelsComboBox->currentText();
    m_plugin->systemPrompt = m_systemPromptEdit->toPlainText();
    m_plugin->ollamaURL = m_ollamaURLText->text();
}

void KateOllamaConfigPage::defaults()
{
    m_ollamaURLText->setText("http://localhost:11434");
    m_systemPromptEdit->setPlainText("You are a smart coder assistant, code comments are in the prompt language. You don't explain, you add only code comments.");
}

void KateOllamaConfigPage::reset()
{
    // Reset the UI values to last known settings
    m_modelsComboBox->setCurrentText(m_plugin->model);
    m_systemPromptEdit->setPlainText(m_plugin->systemPrompt);
    m_ollamaURLText->setText(m_plugin->ollamaURL);
}

void KateOllamaConfigPage::loadSettings()
{
    KConfigGroup group(KSharedConfig::openConfig(), "KateOllama");
    
    QString model = group.readEntry("Model");
    QString url = group.readEntry("URL");
    QString systemPrompt = group.readEntry("SystemPrompt");

    if (url.isEmpty()) {
        defaults();
    } 
    
    m_ollamaURLText->setText(url);
    m_systemPromptEdit->setPlainText(systemPrompt);
    
    m_plugin->systemPrompt = m_systemPromptEdit->toPlainText();
    m_plugin->ollamaURL = m_ollamaURLText->text();
    m_plugin->model = model;

    fetchModelList();
}
