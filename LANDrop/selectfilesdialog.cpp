/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, LANDrop
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QDir>
#include <QtWidgets>

#include "selectfilesdialog.h"
#include "sendtodialog.h"
#include "ui_selectfilesdialog.h"

SelectFilesDialog::SelectFilesDialog(QWidget *parent, DiscoveryService &discoveryService) :
    QDialog(parent), ui(new Ui::SelectFilesDialog), discoveryService(discoveryService)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    connect(ui->addButton, &QPushButton::clicked, this, &SelectFilesDialog::addButtonClicked);
    connect(ui->removeButton, &QPushButton::clicked, this, &SelectFilesDialog::removeButtonClicked);
    connect(ui->folderButton, &QPushButton::clicked, this, &SelectFilesDialog::folderButtonClicked);
    ui->filesListView->setModel(&filesStringListModel);

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Send"));
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
}

SelectFilesDialog::~SelectFilesDialog()
{
    delete ui;
}

void SelectFilesDialog::addFile(const QString &filename, const QString &relativepath)
{
    // Check if file already in file list
    foreach (QSharedPointer<QFile> file, files) {
        if (file->fileName() == filename)
            return;
    }

    // check if file exist or regular
    QSharedPointer<QFile> fp = QSharedPointer<QFile>::create(filename);
    if (!fp->open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("Unable to open file %1. Skipping.")
                              .arg(filename));
        return;
    }
    if (fp->isSequential()) {
        QMessageBox::critical(this, QApplication::applicationName(),
                              tr("%1 is not a regular file. Skipping.")
                              .arg(filename));
        return;
    }
    files.append(fp);
    filepaths.append(relativepath);
}

void SelectFilesDialog::updateFileStringListModel()
{
    QStringList l;
    for(qint16 i = 0; i < files.count(); i++)
    {
        QString filepath = filepaths[i];
        QString filename;
        if("" == filepath) filename = QFileInfo(*files[i]).fileName();
        else{
            filename = filepaths[i] + QDir::separator() + QFileInfo(*files[i]).fileName();
        }
        l.append(filename);
    }
    filesStringListModel.setStringList(l);
}

void SelectFilesDialog::addButtonClicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(nullptr, tr("Select File(s) to be Sent"));
    if (filenames.empty())
        return;

    foreach (const QString &filename, filenames) {
        addFile(filename, tr(""));
    }

    updateFileStringListModel();
}

void SelectFilesDialog::folderButtonClicked()
{
    QString selectedDir = QFileDialog::getExistingDirectory(this,"Select folder to be Sent");
    if (selectedDir.isEmpty()) {
        return;
    }
    QDirIterator folderiter(selectedDir,
                      QDir::Files | QDir::NoSymLinks,
                      QDirIterator::Subdirectories);
    while(folderiter.hasNext()){
        folderiter.next();
        QString filename = folderiter.fileInfo().absoluteFilePath();
        QDir dir(selectedDir);
        QDir filedir(filename);
        dir.cdUp();
        filedir.cdUp();
        QString relative = dir.relativeFilePath(filedir.absolutePath());
        addFile(filename, relative);
    }

    updateFileStringListModel();
}

void SelectFilesDialog::removeButtonClicked()
{
    QModelIndexList indexes = ui->filesListView->selectionModel()->selectedIndexes();
    QList<const QSharedPointer<QFile> *> removeList;
    QList<const QString *> pathremoveList;
    foreach (const QModelIndex &i, indexes) {
        removeList.append(&files.at(i.row()));
        pathremoveList.append(&filepaths.at(i.row()));
    }
    foreach (const QSharedPointer<QFile> *fp, removeList) {
        files.removeOne(*fp);
    }
    foreach (const QString *path, pathremoveList) {
        filepaths.removeOne(*path);
    }
    updateFileStringListModel();
}

void SelectFilesDialog::accept()
{
    if (files.empty()) {
        QMessageBox::warning(this, QApplication::applicationName(), tr("No file to be sent."));
        return;
    }

    SendToDialog *d = new SendToDialog(nullptr, files, filepaths, discoveryService);
    d->setAttribute(Qt::WA_DeleteOnClose);
    d->show();

    done(Accepted);
}

void SelectFilesDialog::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void SelectFilesDialog::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        foreach (const QUrl &url, event->mimeData()->urls()) {
            addFile(url.toLocalFile(), tr(""));
        }
        updateFileStringListModel();
        event->acceptProposedAction();
    }
}



