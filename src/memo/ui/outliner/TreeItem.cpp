#include "TreeItem.h"
#include <QStringList>
#include <QModelIndex>

TreeItem::TreeItem(TreeItem* parent) : parentItem(parent) {
}

TreeItem::~TreeItem() {
    qDeleteAll(childItems);
}

TreeItem* TreeItem::child(int number) {
    return childItems.value(number);
}

int TreeItem::childCount() const {
    return childItems.count();
}

int TreeItem::childNumber() const {
    if (parentItem) {
        return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));
    }

    return 0;
}

QVariant TreeItem::data() const {
    return itemData;
}

TreeItem* TreeItem::parent() {
    return parentItem;
}

void TreeItem::setParent(TreeItem* parent) {
    parentItem = parent;
}

TreeItem* TreeItem::find(int id) {
    if (itemId == id) return this;

    for (TreeItem* child : childItems) {
        TreeItem* item = child->find(id);
        if (item != nullptr) {
            return item;
        }
    }

    return nullptr;
}

bool TreeItem::insertChild(int position, TreeItem* item) {
    TreeItem* childItem = item ? item : new TreeItem;
    childItem->setParent(this);
    childItems.insert(position, childItem);

    return true;
}

bool TreeItem::removeChild(int position) {
    if (position < 0 || position > childItems.count() - 1) return false;

    childItems.removeAt(position);

    return true;
}

void TreeItem::setData(const QVariant& value) {
    itemData = value;
}

int TreeItem::id() const {
    return itemId;
}

void TreeItem::setId(int id) {
    itemId = id;
}

int TreeItem::depth() {
    int counter = 0;
    TreeItem* item = this;

    while (item->id() != 0) {
        counter++;
        item = item->parent();
    }

    return counter;
}