/*
 S PDX-FileCopyrightText*: 2025 Daniele Mte90 Scasciafratte <mte90net@gmail.com>

 SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef KATEOLLAMACONFIGPAGE_H
#define KATEOLLAMACONFIGPAGE_H

#include <KTextEditor/ConfigPage>

class KateOllamaPlugin;
class QLabel;
class QComboBox;
class QLineEdit;
class QRadioButton;
class QTextEdit;
class QWidget;

class KateOllamaConfigPage : public KTextEditor::ConfigPage
{
    Q_OBJECT

public:
    explicit KateOllamaConfigPage(QWidget *parent = nullptr, KateOllamaPlugin *plugin = nullptr);

    void loadSettings();
    void fetchModelList();

public:
    QString name() const override;
    QString fullName() const override;
    QIcon icon() const override;

public:
    void apply() override;
    void defaults() override;
    void reset() override;

private:
    QComboBox *m_modelsComboBox;
    QTextEdit *m_systemPromptEdit;
    QLineEdit *m_ollamaURLText;
    QRadioButton *m_radioCurrentDoc;
    QRadioButton *m_radioNamedDoc;
    QLineEdit *m_docNameEdit;
    QLabel *m_infoLabel;

    KateOllamaPlugin *const m_plugin;
};

#endif // KATEOLLAMACONFIGPAGE_H
