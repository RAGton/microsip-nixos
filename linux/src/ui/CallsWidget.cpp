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

#include "CallsWidget.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QHeaderView>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>

namespace msip {

CallsWidget::CallsWidget(Database* db, QWidget* parent)
    : QWidget(parent)
    , m_db(db) {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(6);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("Pesquisar histórico de chamadas..."));
    m_searchEdit->setClearButtonEnabled(true);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &CallsWidget::onFilterChanged);
    layout->addWidget(m_searchEdit);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("Tipo"), tr("Nome / Número"), tr("Data / Hora"), tr("Duração"), tr("Info")});
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setDefaultSectionSize(30);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_table, &QTableWidget::customContextMenuRequested, this, &CallsWidget::onContextMenu);
    connect(m_table, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        if (auto* item = m_table->item(row, 1)) {
            QString num = item->data(Qt::UserRole).toString();
            if (!num.isEmpty()) emit callNumberRequested(num);
        }
    });

    layout->addWidget(m_table);
    reload();
}

void CallsWidget::reload() {
    m_records = m_db->getAllCalls(500);
    populateTable(m_searchEdit->text());
}

void CallsWidget::onFilterChanged(const QString& text) {
    populateTable(text);
}

void CallsWidget::populateTable(const QString& filter) {
    m_table->setRowCount(0);
    QString f = filter.trimmed();

    for (const auto& rec : m_records) {
        if (!f.isEmpty() &&
            !rec.number.contains(f, Qt::CaseInsensitive) &&
            !rec.name.contains(f, Qt::CaseInsensitive) &&
            !rec.info.contains(f, Qt::CaseInsensitive)) {
            continue;
        }

        int row = m_table->rowCount();
        m_table->insertRow(row);

        QString dirText;
        QColor dirColor;
        switch (rec.type) {
            case CallDirection::Outgoing: dirText = "↗ Out"; dirColor = QColor("#1976d2"); break;
            case CallDirection::Incoming: dirText = "↙ In"; dirColor = QColor("#2e7d32"); break;
            case CallDirection::Missed:   dirText = "✕ Miss"; dirColor = QColor("#c62828"); break;
            default:                      dirText = "•"; dirColor = QColor("#757575"); break;
        }

        auto* typeItem = new QTableWidgetItem(dirText);
        typeItem->setForeground(dirColor);
        typeItem->setData(Qt::UserRole, rec.id);

        QString display = rec.name.isEmpty() ? rec.number : QString("%1 (%2)").arg(rec.name, rec.number);
        auto* numItem = new QTableWidgetItem(display);
        numItem->setData(Qt::UserRole, rec.number);
        numItem->setData(Qt::UserRole + 1, rec.name);

        QString timeStr = QDateTime::fromSecsSinceEpoch(rec.time).toString("yyyy-MM-dd HH:mm");
        QString durStr = QString("%1:%2").arg(rec.duration / 60, 2, 10, QChar('0')).arg(rec.duration % 60, 2, 10, QChar('0'));

        m_table->setItem(row, 0, typeItem);
        m_table->setItem(row, 1, numItem);
        m_table->setItem(row, 2, new QTableWidgetItem(timeStr));
        m_table->setItem(row, 3, new QTableWidgetItem(durStr));
        m_table->setItem(row, 4, new QTableWidgetItem(rec.info));
    }
    m_table->resizeColumnsToContents();
}

void CallsWidget::onContextMenu(const QPoint& pos) {
    int row = m_table->rowAt(pos.y());
    QMenu menu(this);

    if (row >= 0) {
        int id = m_table->item(row, 0)->data(Qt::UserRole).toInt();
        QString num = m_table->item(row, 1)->data(Qt::UserRole).toString();
        QString name = m_table->item(row, 1)->data(Qt::UserRole + 1).toString();

        menu.addAction(tr("Call"), this, [this, num]() { emit callNumberRequested(num); });
        menu.addAction(tr("Send Message"), this, [this, num]() { emit messageNumberRequested(num); });
        menu.addAction(tr("Add to Contacts"), this, [this, num, name]() { emit addContactRequested(num, name); });
        menu.addAction(tr("Copy Number"), this, [num]() { QApplication::clipboard()->setText(num); });
        menu.addSeparator();
        menu.addAction(tr("Delete"), this, [this, id]() {
            m_db->deleteCall(id);
            reload();
        });
    }

    menu.addAction(tr("Clear All History"), this, [this]() {
        if (QMessageBox::question(this, tr("Clear History"), tr("Delete all call history?")) == QMessageBox::Yes) {
            m_db->deleteAllCalls();
            reload();
        }
    });

    menu.exec(m_table->viewport()->mapToGlobal(pos));
}

} // namespace msip
