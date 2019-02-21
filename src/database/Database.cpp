#include "Database.h"
#include <QtSql>

Database::Database(QObject* parent) : QObject(parent) {
    db = QSqlDatabase::addDatabase("QSQLITE");
}

Database::~Database() {
    close();
}

bool Database::create(const QString& filepath) {
    qInfo().noquote() << "Create database:" << filepath;
    db.close();
    db.setDatabaseName(filepath);
    if (!db.open()) {
        databaseError(db.lastError());
        return false;
    }

    QSqlQuery query;
    if (!query.exec("CREATE TABLE notes("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                    "parent_id INTEGER,"
                    "pos INTEGER,"
                    "depth INTEGER,"
                    "title TEXT,"
                    "note TEXT,"
                    "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
                    "updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
                    ")")) {
        queryError(query);
        return false;
    }

    return true;
}

bool Database::open(const QString& filepath) {
    qDebug().noquote() << "Open database:" << filepath;
    db.close();
    db.setDatabaseName(filepath);
    if (!db.open()) {
        databaseError(db.lastError());
        return false;
    }

    return true;
}

void Database::close() {
    if (db.isOpen()) {
        qInfo().noquote() << "Close database:" << db.databaseName();
        db.close();
    }
}

int Database::insertRecord(int parentId, int pos, int depth, const QString& title) {
    QSqlQuery query;
    query.prepare("INSERT INTO notes (parent_id, pos, depth, title) VALUES (:parent_id, :pos, :depth, :title)");
    query.bindValue(":parent_id", parentId);
    query.bindValue(":pos", pos);
    query.bindValue(":depth", depth);
    query.bindValue(":title", title);

    if (!query.exec()) {
        queryError(query);
        return -1;
    }

    return query.lastInsertId().toInt();
}

bool Database::removeRecord(int id) {
    QSqlQuery query;
    query.prepare("DELETE FROM notes WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        queryError(query);
        return false;
    }

    return true;
}

bool Database::updateValue(int id, const QString& name, const QVariant& value) {
    QSqlQuery query;
    query.prepare(QString("UPDATE notes SET %1 = :value WHERE id = :id").arg(name));
    query.bindValue(":id", id);
    query.bindValue(":value", value);

    if (!query.exec()) {
        queryError(query);
        return false;
    }

    return true;
}

QVariant Database::value(int id, const QString& name) {
    QSqlQuery query;
    query.prepare(QString("SELECT %1 FROM notes WHERE id = :id").arg(name));
    query.bindValue(":id", id);

    if (!query.exec()) {
        queryError(query);
        return QVariant();
    }

    if (query.first()) {
        return query.value(name);
    }

    return QVariant();
}


QVector<Database::Title> Database::titles() {
    QVector<Title> list;

    QSqlQuery query("SELECT id, parent_id, pos, depth, title FROM notes ORDER BY depth, pos");
    while (query.next()) {
        Title title;
        title.id = query.value("id").toInt();
        title.parentId = query.value("parent_id").toInt();
        title.pos = query.value("pos").toInt();
        title.depth = query.value("depth").toInt();
        title.title = query.value("title").toString();

        list.append(title);
    }

    return list;
}

void Database::databaseError(const QSqlError& error) {
    qCritical("Database error: %s. %s", qUtf8Printable(error.databaseText()), qUtf8Printable(error.driverText()));
}

void Database::queryError(const QSqlQuery& query) {
    qCritical("SQL query error: %s. %s. %s", qUtf8Printable(query.lastError().databaseText()), qUtf8Printable(query.lastError().driverText()), qUtf8Printable(query.lastQuery()));
}
