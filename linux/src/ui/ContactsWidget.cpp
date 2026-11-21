/*
 * MicroSIP - Native Linux / NixOS Port (Qt 6 + PJSIP)
 * Based on MicroSIP (https://www.microsip.org)
 * Copyright (C) 2011-2026 MicroSIP (https://www.microsip.org)
 * Native Linux/NixOS Qt6 Port Copyright (C) 2026 Gabriel Aguiar Rocha (RAGton) <aguiarrocha37@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "ContactsWidget.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QTextStream>
#include <QVBoxLayout>
#include <QXmlStreamReader>

namespace msip {

ContactsWidget::ContactsWidget(Database* db, QWidget* parent)
    : QWidget(parent)
    , m_db(db) {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(6);

    auto* topRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Pesquisar contatos..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& t) { populateTable(t); });

    m_addBtn = new QPushButton(tr("+ Novo Contato"), this);
    m_addBtn->setObjectName("primaryBtn");
    connect(m_addBtn, &QPushButton::clicked, this, [this]() { openAddContactDialog(); });

    auto* impExpBtn = new QPushButton(tr("⇅"), this);
    impExpBtn->setToolTip(tr("Importar / Exportar Contatos (CSV ou Contacts.xml)"));
    impExpBtn->setFixedWidth(34);
    auto* ieMenu = new QMenu(impExpBtn);
    ieMenu->addAction(tr("Importar CSV / Contacts.xml..."), this, &ContactsWidget::importContacts);
    ieMenu->addAction(tr("Exportar para CSV..."), this, &ContactsWidget::exportContactsCsv);
    impExpBtn->setMenu(ieMenu);

    topRow->addWidget(m_searchEdit, 1);
    topRow->addWidget(m_addBtn);
    topRow->addWidget(impExpBtn);
    root->addLayout(topRow);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("BLF"), tr("Nome"), tr("Número"), tr("Info")});
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_table, &QTableWidget::customContextMenuRequested, this, &ContactsWidget::onContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* item = m_table->item(row, 2)) {
            emit callNumberRequested(item->text());
        }
    });

    root->addWidget(m_table);
    reload();
}

void ContactsWidget::reload() {
    QHash<QString, PresenceStatus> oldStatus;
    QHash<QString, bool> oldRinging;
    for (const auto& c : m_contacts) {
        oldStatus.insert(c.number, c.presenceStatus);
        oldRinging.insert(c.number, c.ringing);
    }

    m_contacts = m_db->getAllContacts();
    for (auto& c : m_contacts) {
        if (oldStatus.contains(c.number)) {
            c.presenceStatus = oldStatus.value(c.number);
            c.ringing = oldRinging.value(c.number);
        }
    }
    populateTable(m_searchEdit->text());
}

void ContactsWidget::updatePresence(const QString& number, PresenceStatus status, bool ringing, const QString& statusText) {
    bool changed = false;
    for (auto& c : m_contacts) {
        if (c.number == number) {
            c.presenceStatus = status;
            c.ringing = ringing;
            if (!statusText.isEmpty()) c.info = statusText;
            changed = true;
        }
    }
    if (changed) {
        populateTable(m_searchEdit->text());
    }
}

void ContactsWidget::populateTable(const QString& filter) {
    m_table->setRowCount(0);
    QString f = filter.trimmed();

    for (int i = 0; i < m_contacts.size(); ++i) {
        const auto& c = m_contacts[i];
        if (!f.isEmpty() &&
            !c.name.contains(f, Qt::CaseInsensitive) &&
            !c.number.contains(f, Qt::CaseInsensitive) &&
            !c.comment.contains(f, Qt::CaseInsensitive)) {
            continue;
        }

        int row = m_table->rowCount();
        m_table->insertRow(row);

        QString dot = "○";
        QColor color("#9e9e9e");
        if (c.presence) {
            switch (c.presenceStatus) {
                case PresenceStatus::Online:     dot = "●"; color = QColor("#2e7d32"); break;
                case PresenceStatus::Away:       dot = "●"; color = QColor("#f9a825"); break;
                case PresenceStatus::Busy:
                case PresenceStatus::OnThePhone: dot = "●"; color = QColor("#c62828"); break;
                case PresenceStatus::Ringing:    dot = "◉"; color = QColor("#ef6c00"); break;
                case PresenceStatus::Offline:    dot = "●"; color = QColor("#757575"); break;
                default:                         dot = "○"; color = QColor("#9e9e9e"); break;
            }
        }

        auto* blfItem = new QTableWidgetItem(c.starred ? ("★ " + dot) : dot);
        blfItem->setForeground(color);
        blfItem->setData(Qt::UserRole, c.id);
        blfItem->setTextAlignment(Qt::AlignCenter);

        m_table->setItem(row, 0, blfItem);
        m_table->setItem(row, 1, new QTableWidgetItem(c.name));
        m_table->setItem(row, 2, new QTableWidgetItem(c.number));
        m_table->setItem(row, 3, new QTableWidgetItem(c.info.isEmpty() ? c.comment : c.info));
    }
    m_table->resizeColumnsToContents();
}

void ContactsWidget::openAddContactDialog(const QString& number, const QString& name) {
    Contact c;
    c.number = number;
    c.name = name;
    if (editContactDialog(c, true)) {
        m_db->addContact(c);
        reload();
        emit contactsModified();
    }
}

bool ContactsWidget::editContactDialog(Contact& c, bool isNew) {
    QDialog dlg(this);
    dlg.setWindowTitle(isNew ? tr("Add Contact") : tr("Edit Contact"));
    dlg.setMinimumWidth(340);

    auto* form = new QFormLayout(&dlg);
    auto* nameEdit = new QLineEdit(c.name, &dlg);
    auto* numberEdit = new QLineEdit(c.number, &dlg);
    auto* emailEdit = new QLineEdit(c.email, &dlg);
    auto* commentEdit = new QLineEdit(c.comment, &dlg);
    auto* presenceCheck = new QCheckBox(tr("Presence Subscription (BLF)"), &dlg);
    presenceCheck->setChecked(c.presence);
    auto* starredCheck = new QCheckBox(tr("Favorite / Starred"), &dlg);
    starredCheck->setChecked(c.starred);

    form->addRow(tr("Name:"), nameEdit);
    form->addRow(tr("Number / SIP URI:"), numberEdit);
    form->addRow(tr("Email:"), emailEdit);
    form->addRow(tr("Comment:"), commentEdit);
    form->addRow("", presenceCheck);
    form->addRow("", starredCheck);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() != QDialog::Accepted) return false;
    if (numberEdit->text().trimmed().isEmpty()) return false;

    c.name = nameEdit->text().trimmed();
    if (c.name.isEmpty()) c.name = numberEdit->text().trimmed();
    c.number = numberEdit->text().trimmed();
    c.email = emailEdit->text().trimmed();
    c.comment = commentEdit->text().trimmed();
    c.presence = presenceCheck->isChecked();
    c.starred = starredCheck->isChecked();
    return true;
}

void ContactsWidget::onContextMenu(const QPoint& pos) {
    int row = m_table->rowAt(pos.y());
    QMenu menu(this);

    menu.addAction(tr("Add Contact..."), this, [this]() { openAddContactDialog(); });

    if (row >= 0) {
        int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
        QString num = m_table->item(row, 2)->text();

        menu.addSeparator();
        menu.addAction(tr("Call"), this, [this, num]() { emit callNumberRequested(num); });
        menu.addAction(tr("Send Message"), this, [this, num]() { emit messageNumberRequested(num); });
        menu.addAction(tr("Edit..."), this, [this, id]() {
            for (auto& c : m_contacts) {
                if (c.id == id) {
                    Contact copy = c;
                    if (editContactDialog(copy, false)) {
                        m_db->updateContact(copy);
                        reload();
                        emit contactsModified();
                    }
                    break;
                }
            }
        });
        menu.addAction(tr("Delete"), this, [this, id]() {
            if (QMessageBox::question(this, tr("Delete Contact"), tr("Delete selected contact?")) == QMessageBox::Yes) {
                m_db->deleteContact(id);
                reload();
                emit contactsModified();
            }
        });
    }

    menu.exec(m_table->viewport()->mapToGlobal(pos));
}

void ContactsWidget::importContacts() {
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Importar Contatos"),
        QString(),
        tr("Contatos MicroSIP (*.xml *.csv);;Todos os Arquivos (*)")
    );
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Erro"), tr("Não foi possível abrir o arquivo selecionado."));
        return;
    }

    int imported = 0;
    if (path.endsWith(".xml", Qt::CaseInsensitive)) {
        QXmlStreamReader xml(&file);
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement() && xml.name().compare(QStringLiteral("contact"), Qt::CaseInsensitive) == 0) {
                auto attrs = xml.attributes();
                Contact c;
                c.number = attrs.value("number").toString().trimmed();
                c.name = attrs.value("name").toString().trimmed();
                c.email = attrs.value("email").toString().trimmed();
                c.comment = attrs.value("comment").toString().trimmed();
                c.presence = attrs.value("presence").toString() == "1";
                c.starred = attrs.value("starred").toString() == "1";
                if (!c.number.isEmpty()) {
                    if (c.name.isEmpty()) c.name = c.number;
                    m_db->addContact(c);
                    ++imported;
                }
            }
        }
    } else {
        QTextStream in(&file);
        bool firstLine = true;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty()) continue;
            QStringList cols = line.split(line.contains(';') ? ';' : ',');
            if (firstLine && cols.size() >= 2 &&
                (cols[0].contains("name", Qt::CaseInsensitive) || cols[0].contains("nome", Qt::CaseInsensitive) ||
                 cols[0].contains("number", Qt::CaseInsensitive))) {
                firstLine = false;
                continue;
            }
            firstLine = false;
            if (cols.size() >= 2) {
                Contact c;
                c.name = cols[0].trimmed().remove('"');
                c.number = cols[1].trimmed().remove('"');
                if (cols.size() >= 3) c.email = cols[2].trimmed().remove('"');
                if (cols.size() >= 4) c.comment = cols[3].trimmed().remove('"');
                if (cols.size() >= 5) c.presence = (cols[4].trimmed() == "1");
                if (!c.number.isEmpty()) {
                    if (c.name.isEmpty()) c.name = c.number;
                    m_db->addContact(c);
                    ++imported;
                }
            }
        }
    }

    reload();
    emit contactsModified();
    QMessageBox::information(this, tr("Importação Concluída"), tr("%1 contato(s) importado(s).").arg(imported));
}

void ContactsWidget::exportContactsCsv() {
    QString path = QFileDialog::getSaveFileName(
        this,
        tr("Exportar Contatos"),
        "contacts.csv",
        tr("Arquivo CSV (*.csv)")
    );
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("Erro"), tr("Não foi possível salvar o arquivo."));
        return;
    }

    QTextStream out(&file);
    out << "Name,Number,Email,Comment,Presence\n";
    auto esc = [](QString s) {
        s.replace('"', "\"\"");
        return "\"" + s + "\"";
    };
    for (const auto& c : m_contacts) {
        out << esc(c.name) << ","
            << esc(c.number) << ","
            << esc(c.email) << ","
            << esc(c.comment) << ","
            << (c.presence ? "1" : "0") << "\n";
    }
    QMessageBox::information(this, tr("Exportação Concluída"), tr("%1 contato(s) exportado(s).").arg(m_contacts.size()));
}

} // namespace msip
