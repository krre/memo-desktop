#include "NoteTaking.h"
#include "TreeModel.h"
#include "TreeItem.h"
#include "NoteProperties.h"
#include "database/Database.h"
#include "database/DatabaseException.h"
#include <QtGui/private/qzipwriter_p.h>
#include <QtWidgets>
#include <QtSql>

NoteTaking::NoteTaking(Database* database) : m_database(database) {
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QTreeView::customContextMenuRequested, this, &NoteTaking::onCustomContextMenu);
    createContextMenu();
    header()->setVisible(false);

    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::SingleSelection);

    clear();

    connect(itemDelegate(), &QAbstractItemDelegate::closeEditor, this, [=, this] {
        TreeItem* item = m_model->item(selectionModel()->currentIndex());
        database->updateValue(item->id(), "title", item->data());
    });
}

void NoteTaking::exportAllNotes(const QString& filePath) {
    QFileInfo fi(filePath);
    QString dirPath = fi.absolutePath() + "/" + fi.baseName();

    auto sg = qScopeGuard([=] {
        QDir dir(dirPath);
        dir.removeRecursively();
    });

    int count = exportNote(0, dirPath);
    compressDir(dirPath);

    QMessageBox::information(this, tr("Export Finished"), tr("Count of notes: %1").arg(count));
}

void NoteTaking::updateActions() {
    bool hasCurrent = selectionModel()->currentIndex().isValid();

    if (hasCurrent) {
        closePersistentEditor(selectionModel()->currentIndex());
    }
}

void NoteTaking::build() {
    clear();
    QVector<Database::Note> notes = m_database->notes();
    int selectedId = m_database->metaValue("selected_id").toInt();

    TreeItem* rootItem = m_model->root();

    for (const Database::Note& note : notes) {
        TreeItem* parentItem = rootItem->find(note.parentId);
        QModelIndex parentIndex = m_model->index(parentItem);
        m_model->insertRow(note.pos, parentIndex);

        QModelIndex index = m_model->index(note.pos, 0, parentIndex);
        m_model->setData(index, QVariant(note.title), Qt::EditRole);
        m_model->item(index)->setId(note.id);
    }

    if (selectedId == 0) {
        setCurrentIndex(QModelIndex());
    } else {
        TreeItem* item = m_model->root()->find(selectedId);
        QModelIndex index = m_model->index(item);
        setCurrentIndex(index);

        TreeItem* parent = item->parent();

        while (parent != rootItem) {
            setExpanded(m_model->index(parent), true);
            parent = parent->parent();
        }
    }
}

void NoteTaking::clear() {
    m_isInited = false;
    m_model.reset(new TreeModel);
    setModel(m_model.data());
    connect(m_model.data(), &TreeModel::itemDropped, this, &NoteTaking::moveTree);

    m_isInited = true;
}

void NoteTaking::onCustomContextMenu(const QPoint& point) {
    if (m_database->isOpen()) {
        updateContextMenu();
        m_contextMenu->exec(mapToGlobal(point));
    }
}

void NoteTaking::addNote() {
    bool ok;
    QString text = QInputDialog::getText(this, tr("Add Note"), tr("Name:"), QLineEdit::Normal, "", &ok);

    if (ok && !text.isEmpty()) {
        insertChild(text);
    }
}

void NoteTaking::removeNotes() {
    QModelIndex index = selectionModel()->currentIndex();
    TreeItem* parentItem = m_model->item(index.parent());

    int result = QMessageBox::question(this, tr("Remove Notes"), tr("Remove %1?").arg(m_model->data(index).toString()));

    if (result == QMessageBox::Yes) {
        TreeItem* item = m_model->item(index);
        QVector<Id> ids = m_model->childIds(item);
        m_model->removeRow(index.row(), index.parent());

        for (Id id : ids) {
            m_database->removeNote(id);
        }

        for (int i = 0; i < parentItem->childCount(); i++) {
            Id id = parentItem->child(i)->id();
            m_database->updateValue(id, "pos", i);
        }
    }
}

void NoteTaking::renameNote() {
    edit(selectionModel()->currentIndex());
}

void NoteTaking::moveUp() {
    int row = currentIndex().row();

    Id id1 = m_model->item(currentIndex())->id();
    Id id2 = m_model->item(currentIndex().sibling(row - 1, 0))->id();

    m_model->moveRow(currentIndex().parent(), row, currentIndex().parent(), row - 1);

    m_database->updateValue(id1, "pos", row - 1);
    m_database->updateValue(id2, "pos", row);
}

void NoteTaking::moveDown() {
    int row = currentIndex().row();

    Id id1 = m_model->item(currentIndex())->id();
    Id id2 = m_model->item(currentIndex().sibling(row + 1, 0))->id();

    m_model->moveRow(currentIndex().parent(), row, currentIndex().parent(), row + 2);

    m_database->updateValue(id1, "pos", row + 1);
    m_database->updateValue(id2, "pos", row);
}

void NoteTaking::moveTree(const QModelIndex& index) {
    selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
    expand(index.parent());

    // Move tree in database.
    TreeItem* targetItem = m_model->item(index);
    Id sourceId = targetItem->id();
    Id sourceParentId = m_database->value(sourceId, "parent_id").toInt();
    Id destinationParentId = targetItem->parent()->id();

    TreeItem* sourceParentItem = m_model->root()->find(sourceParentId);

    // Rewrite note positions on source parent.
    for (int i = 0; i < sourceParentItem->childCount(); i++) {
         m_database->updateValue(sourceParentItem->child(i)->id(), "pos", i);
    }

    if (sourceParentId != destinationParentId) {
         m_database->updateValue(sourceId, "parent_id", destinationParentId);

         TreeItem* destinationParentItem = m_model->root()->find(destinationParentId);

         // Rewrite note positions on destination parent.
         for (int i = 0; i < destinationParentItem->childCount(); i++) {
             m_database->updateValue(destinationParentItem->child(i)->id(), "pos", i);
         }

         // Rewrite depth in all children of target note.
         QVector<Id> childIds = m_model->childIds(targetItem);
         for (Id id : childIds) {
             int depth = targetItem->find(id)->depth() - 1;
             m_database->updateValue(id, "depth", depth);
         }
    }
}

void NoteTaking::showProperties() {
    Id id = m_model->item(currentIndex())->id();
    QSqlQuery query = m_database->note(id);

    NoteProperties::Data data;
    data.title = query.value("title").toString();
    data.createdAt = query.value("created_at").toString();
    data.updatedAt = query.value("updated_at").toString();

    NoteProperties props(data);
    props.exec();
}

void NoteTaking::createContextMenu() {
    m_contextMenu = new QMenu(this);
    m_contextMenu->addAction(tr("Add..."), this, &NoteTaking::addNote);
    m_removeAction = m_contextMenu->addAction(tr("Remove..."), this, &NoteTaking::removeNotes);
    m_renameAction = m_contextMenu->addAction(tr("Rename"), this, &NoteTaking::renameNote);
    m_contextMenu->addSeparator();
    m_moveUpAction = m_contextMenu->addAction(tr("Move Up"), this, &NoteTaking::moveUp);
    m_moveDownAction = m_contextMenu->addAction(tr("Move Down"), this, &NoteTaking::moveDown);
    m_contextMenu->addSeparator();
    m_propertiesAction = m_contextMenu->addAction(tr("Properties..."), this, &NoteTaking::showProperties);
}

void NoteTaking::updateContextMenu() {
    bool enabled = currentIndex().isValid();
    m_removeAction->setEnabled(enabled);
    m_renameAction->setEnabled(enabled);

    m_moveUpAction->setEnabled(enabled && currentIndex().row() > 0);
    m_moveDownAction->setEnabled(enabled && currentIndex().row() < m_model->rowCount(currentIndex().parent()) - 1);

    m_propertiesAction->setEnabled(enabled);
}

void NoteTaking::insertChild(const QString& title) {
    QModelIndex currentIndex = selectionModel()->currentIndex();
    TreeItem* currentItem = m_model->item(currentIndex);

    Id currentId = currentItem->id();
    int childRow = currentItem->childCount();
    int childDepth = currentItem->depth();
    int childId = m_database->insertNote(currentId, childRow, childDepth, title);

    if (!m_model->insertRow(childRow, currentIndex)) {
        return;
    }

    QModelIndex child = m_model->index(childRow, 0, currentIndex);
    m_model->setData(child, QVariant(title), Qt::EditRole);
    m_model->item(child)->setId(childId);

    selectionModel()->setCurrentIndex(m_model->index(childRow, 0, currentIndex), QItemSelectionModel::ClearAndSelect);
    setExpanded(currentIndex, true);

    updateActions();
}

int NoteTaking::exportNote(Id parentId, const QString& path) {
    QDir dir;
    dir.mkdir(path);

    int count = 0;
    TreeItem* parentItem = m_model->root()->find(parentId);

    for (int i = 0; i < parentItem->childCount(); i++) {
        TreeItem* childItem = parentItem->child(i);
        QString title = childItem->data().toString();
        QString note = m_database->value(childItem->id(), "note").toString();
        QString notePath = path + "/" + title;

        QString filename = notePath + ".txt";
        QFile file(filename);

        if (file.open(QIODevice::ReadWrite)) {
            QTextStream stream(&file);
            stream << note;
            file.close();
        }

        count++;

        if (childItem->childCount()) {
            count += exportNote(childItem->id(), notePath);
        }
    }

    return count;
}

void NoteTaking::compressDir(const QString& dirPath) {
    QZipWriter zipWriter(dirPath + ".zip");

    switch (zipWriter.status()) {
        case QZipWriter::FileWriteError: throw RuntimeError("Write file error");
        case QZipWriter::FileOpenError: throw RuntimeError("Open file error");
        case QZipWriter::FilePermissionsError: throw RuntimeError("Permissions file error");
        case QZipWriter::FileError: throw RuntimeError("File error");
        case QZipWriter::NoError: break;
    }

    zipWriter.setCompressionPolicy(QZipWriter::AutoCompress);
    zipWriter.setCreationPermissions(QFile::permissions(dirPath));

    QFileInfo fi(dirPath);
    zipWriter.addDirectory(fi.fileName());

    QDirIterator it(dirPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString absoluteFilePath = it.next();
        QString relativeFilePath = fi.fileName() + QString(absoluteFilePath).remove(dirPath);

        if (it.fileInfo().isDir()) {
            zipWriter.setCreationPermissions(QFile::permissions(absoluteFilePath));
            zipWriter.addDirectory(relativeFilePath);
        } else if (it.fileInfo().isFile()) {
            QFile file(absoluteFilePath);
            if (!file.open(QIODevice::ReadOnly)) continue;

            QByteArray data = file.readAll();

            if (!data.isEmpty()) {
                zipWriter.setCreationPermissions(QFile::permissions(absoluteFilePath));
                zipWriter.addFile(relativeFilePath, data);
            }
        }
    }

    zipWriter.close();
}

void NoteTaking::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && !indexAt(event->position().toPoint()).isValid()) {
        selectionModel()->clearSelection();
        setCurrentIndex(QModelIndex());
    } else {
        QTreeView::mousePressEvent(event);
    }
}

void NoteTaking::currentChanged(const QModelIndex& current, const QModelIndex& previous) {
    Q_UNUSED(previous)
    if (!m_isInited) return;

    try {
        Id id = m_model->item(current)->id();
        m_database->updateMetaValue("selected_id", id);
        emit noteChanged(id);
    } catch (const SqlQueryError& e) {
        qCritical() << "Error update selected_id: " << e.error();
    }
}
