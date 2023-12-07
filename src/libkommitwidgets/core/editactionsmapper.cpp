/*
SPDX-FileCopyrightText: 2021 Hamed Masafi <hamed.masfi@gmail.com>

SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "editactionsmapper.h"

#include <KActionCollection>
#include <QEvent>
#include <QPlainTextEdit>

#include <KStandardAction>

EditActionsMapper::EditActionsMapper(QObject *parent)
    : QObject(parent)
{
}

void EditActionsMapper::addTextEdit(QPlainTextEdit *control)
{
    mTextEdits.append(control);

    control->installEventFilter(this);

    control->setContextMenuPolicy(Qt::DefaultContextMenu);
    connect(control, &QPlainTextEdit::copyAvailable, this, &EditActionsMapper::control_copyAvailable);
    connect(control, &QPlainTextEdit::selectionChanged, this, &EditActionsMapper::control_selectionChanged);
    connect(control, &QPlainTextEdit::undoAvailable, this, &EditActionsMapper::control_undoAvailable);
    connect(control, &QPlainTextEdit::redoAvailable, this, &EditActionsMapper::control_redoAvailable);
}

void EditActionsMapper::init(KActionCollection *actionCollection)
{
    mActionCut = KStandardAction::cut(this, &EditActionsMapper::actionCut_triggered, actionCollection);
    mActionCopy = KStandardAction::copy(this, &EditActionsMapper::actionCopy_triggered, actionCollection);
    mActionPaste = KStandardAction::paste(this, &EditActionsMapper::actionPaste_triggered, actionCollection);
    mSelectAll = KStandardAction::selectAll(this, &EditActionsMapper::actionSelectAll_triggered, actionCollection);
    mActionUndo = KStandardAction::undo(this, &EditActionsMapper::actionUndo_triggered, actionCollection);
    mActionRedo = KStandardAction::redo(this, &EditActionsMapper::actionRedo_triggered, actionCollection);
}

void EditActionsMapper::control_undoAvailable(bool b)
{
    auto control = qobject_cast<QPlainTextEdit *>(sender());
    if (control != mActiveControl)
        return;

    mActionUndo->setEnabled(b);
}

void EditActionsMapper::control_redoAvailable(bool b)
{
    auto control = qobject_cast<QPlainTextEdit *>(sender());
    if (control != mActiveControl)
        return;

    mActionRedo->setEnabled(b);
}

void EditActionsMapper::control_copyAvailable(bool b)
{
    auto control = qobject_cast<QPlainTextEdit *>(sender());
    if (control != mActiveControl)
        return;

    mActionCopy->setEnabled(b);
}

void EditActionsMapper::control_selectionChanged()
{
}

void EditActionsMapper::actionUndo_triggered()
{
    if (mActiveControl)
        mActiveControl->undo();
}

void EditActionsMapper::actionRedo_triggered()
{
    if (mActiveControl)
        mActiveControl->redo();
}

void EditActionsMapper::actionCopy_triggered()
{
    if (mActiveControl)
        mActiveControl->copy();
}

void EditActionsMapper::actionCut_triggered()
{
    if (mActiveControl)
        mActiveControl->cut();
}

void EditActionsMapper::actionPaste_triggered()
{
    if (mActiveControl)
        mActiveControl->paste();
}

void EditActionsMapper::actionSelectAll_triggered()
{
    if (mActiveControl)
        mActiveControl->selectAll();
}

void EditActionsMapper::actionDelete_triggered()
{
}

bool EditActionsMapper::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() != QEvent::FocusIn && event->type() != QEvent::FocusOut)
        return QObject::eventFilter(watched, event);
    auto textEdit = qobject_cast<QPlainTextEdit *>(watched);
    if (!textEdit || textEdit == mActiveControl)
        return QObject::eventFilter(watched, event);
    //    auto e = static_cast<QFocusEvent*>(event);

    if (event->type() != QEvent::FocusIn)
        mActiveControl = textEdit;
    else
        mActiveControl = nullptr;

    return QObject::eventFilter(watched, event);
}

// #include "moc_editactionsmapper.cpp"
