#include "ModifierListModel.h"

ModifierListModel::ModifierListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ModifierListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;
    return m_modifiers.size();
}

QVariant ModifierListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_modifiers.size())
        return QVariant();

    const ModifierInfo& modifier = m_modifiers.at(index.row());

    switch (role) {
    case NameRole:
        return modifier.name;
    case GameVersionRole:
        return modifier.gameVersion;
    case LastUpdateRole:
        return modifier.lastUpdate;
    case OptionsCountRole:
        return modifier.optionsCount;
    case UrlRole:
        return modifier.url;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ModifierListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[GameVersionRole] = "gameVersion";
    roles[LastUpdateRole] = "lastUpdate";
    roles[OptionsCountRole] = "optionsCount";
    roles[UrlRole] = "url";
    // Note: ModifierInfo uses 'url' field for detail URL
    return roles;
}

void ModifierListModel::clear()
{
    beginResetModel();
    m_modifiers.clear();
    endResetModel();
    emit countChanged();
}

void ModifierListModel::setModifiers(const QList<ModifierInfo>& modifiers)
{
    beginResetModel();
    m_modifiers = modifiers;
    endResetModel();
    emit countChanged();
}

ModifierInfo ModifierListModel::getModifier(int index) const
{
    if (index >= 0 && index < m_modifiers.size()) {
        return m_modifiers.at(index);
    }
    return ModifierInfo();
}

QString ModifierListModel::getModifierName(int index) const
{
    if (index >= 0 && index < m_modifiers.size()) {
        return m_modifiers.at(index).name;
    }
    return QString();
}
