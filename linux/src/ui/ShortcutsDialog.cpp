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

#include "ShortcutsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace msip {

ShortcutsDialog::ShortcutsDialog(const QVector<Shortcut>& shortcuts, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Shortcuts & BLF Buttons"));
    setMinimumSize(580, 360);

    auto* root = new QVBoxLayout(this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(4);
    m_table->setHorizontalHeaderLabels({tr("Label"), tr("Number"), tr("Action Type"), tr("BLF / Presence")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(false);
    root->addWidget(m_table, 1);

    for (const auto& sc : shortcuts) {
        addShortcutRow(sc);
    }

    auto* toolRow = new QHBoxLayout();
    auto* addBtn = new QPushButton(tr("+ Add Shortcut"), this);
    auto* remBtn = new QPushButton(tr("- Remove Selected"), this);
    connect(addBtn, &QPushButton::clicked, this, &ShortcutsDialog::onAddRow);
    connect(remBtn, &QPushButton::clicked, this, &ShortcutsDialog::onRemoveRow);
    toolRow->addWidget(addBtn);
    toolRow->addWidget(remBtn);
    toolRow->addStretch();
    root->addLayout(toolRow);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    root->addWidget(buttons);
}

void ShortcutsDialog::addShortcutRow(const Shortcut& sc) {
    int row = m_table->rowCount();
    m_table->insertRow(row);

    auto* labelEdit = new QLineEdit(sc.label, m_table);
    auto* numEdit = new QLineEdit(sc.number, m_table);

    auto* typeCombo = new QComboBox(m_table);
    typeCombo->addItem(tr("Call"), "call");
    typeCombo->addItem(tr("Blind Transfer"), "transfer");
    typeCombo->addItem(tr("Attended Transfer"), "attendedTransfer");
    typeCombo->addItem(tr("Call / Blind Transfer"), "callOrBT");
    typeCombo->addItem(tr("Call / Attended Transfer"), "callOrAT");
    typeCombo->addItem(tr("DTMF"), "dtmf");
    typeCombo->addItem(tr("Message"), "message");
    typeCombo->addItem(tr("Conference"), "conference");
    int tIdx = typeCombo->findData(sc.type);
    if (tIdx >= 0) typeCombo->setCurrentIndex(tIdx);

    auto* blfCheck = new QCheckBox(tr("Subscribe"), m_table);
    blfCheck->setChecked(sc.presence);

    m_table->setCellWidget(row, 0, labelEdit);
    m_table->setCellWidget(row, 1, numEdit);
    m_table->setCellWidget(row, 2, typeCombo);
    m_table->setCellWidget(row, 3, blfCheck);
}

void ShortcutsDialog::onAddRow() {
    addShortcutRow(Shortcut{});
}

void ShortcutsDialog::onRemoveRow() {
    int row = m_table->currentRow();
    if (row >= 0) {
        m_table->removeRow(row);
    }
}

QVector<Shortcut> ShortcutsDialog::resultShortcuts() const {
    QVector<Shortcut> list;
    for (int r = 0; r < m_table->rowCount(); ++r) {
        auto* labelEdit = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 0));
        auto* numEdit = qobject_cast<QLineEdit*>(m_table->cellWidget(r, 1));
        auto* typeCombo = qobject_cast<QComboBox*>(m_table->cellWidget(r, 2));
        auto* blfCheck = qobject_cast<QCheckBox*>(m_table->cellWidget(r, 3));

        Shortcut sc;
        sc.label = labelEdit ? labelEdit->text().trimmed() : "";
        sc.number = numEdit ? numEdit->text().trimmed() : "";
        sc.type = typeCombo ? typeCombo->currentData().toString() : "call";
        sc.presence = blfCheck ? blfCheck->isChecked() : false;
        if (!sc.label.isEmpty() || !sc.number.isEmpty()) {
            if (sc.label.isEmpty()) sc.label = sc.number;
            list.push_back(sc);
        }
    }
    return list;
}

} // namespace msip
