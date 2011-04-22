#include <QtGui>

#include <QDebug> // TODO: Remove

#include <components/esm/esm_reader.hpp>
#include "../openmw/path.hpp"

#include "datafilespage.hpp"
#include "lineedit.hpp"

using namespace ESM;

DataFilesPage::DataFilesPage(QWidget *parent) : QWidget(parent)
{
    mDataFilesModel = new QStandardItemModel(); // Contains all plugins with masters
    mPluginsModel = new QStandardItemModel(); // Contains selectable plugins

    mPluginsSelectModel = new QItemSelectionModel(mPluginsModel);

    //QPushButton *deselectButton = new QPushButton(tr("Deselect All"));
    QLabel *filterLabel = new QLabel(tr("Filter:"), this);
    LineEdit *filterLineEdit = new LineEdit(this);

    QHBoxLayout *topLayout = new QHBoxLayout();
    QSpacerItem *hSpacer1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    topLayout->addItem(hSpacer1);
    topLayout->addWidget(filterLabel);
    topLayout->addWidget(filterLineEdit);

    mMastersWidget = new QTableWidget(this); // Contains the available masters
    mPluginsTable = new QTableView(this);

    // Add both tables to a splitter
    QSplitter *splitter = new QSplitter(this);
    splitter->setOrientation(Qt::Horizontal);

    splitter->addWidget(mMastersWidget);
    splitter->addWidget(mPluginsTable);

    // Adjust the default widget widths inside the splitter
    QList<int> sizeList;
    sizeList << 100 << 300;
    splitter->setSizes(sizeList);

    // Bottom part with profile options
    QLabel *profileLabel = new QLabel(tr("Current Profile:"), this);

    mProfilesModel = new QStringListModel();

    mProfilesComboBox = new ComboBox(this);
    mProfilesComboBox->setModel(mProfilesModel);

    mProfilesComboBox->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum));
    mProfilesComboBox->setInsertPolicy(QComboBox::InsertAtBottom);
    //mProfileComboBox->addItem(QString("New Profile"));

    QToolButton *NewProfileToolButton = new QToolButton(this);
    NewProfileToolButton->setIcon(QIcon::fromTheme("document-new"));

    QToolButton *CopyProfileToolButton = new QToolButton(this);
    CopyProfileToolButton->setIcon(QIcon::fromTheme("edit-copy"));

    QToolButton *DeleteProfileToolButton = new QToolButton(this);
    DeleteProfileToolButton->setIcon(QIcon::fromTheme("document-close"));

    QHBoxLayout *bottomLayout = new QHBoxLayout();

    bottomLayout->addWidget(profileLabel);
    bottomLayout->addWidget(mProfilesComboBox);
    bottomLayout->addWidget(NewProfileToolButton);
    bottomLayout->addWidget(CopyProfileToolButton);
    bottomLayout->addWidget(DeleteProfileToolButton);

    QVBoxLayout *pageLayout = new QVBoxLayout(this);
    // Add some space above and below the page items
    //QSpacerItem *vSpacer1 = new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Minimum);
    QSpacerItem *vSpacer2 = new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Minimum);
    QSpacerItem *vSpacer3 = new QSpacerItem(5, 5, QSizePolicy::Minimum, QSizePolicy::Minimum);

    //pageLayout->addItem(vSpacer1);
    pageLayout->addLayout(topLayout);
    pageLayout->addItem(vSpacer2);
    pageLayout->addWidget(splitter);
    pageLayout->addLayout(bottomLayout);
    pageLayout->addItem(vSpacer3);

    connect(mMastersWidget->selectionModel(),
            SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            this, SLOT(masterSelectionChanged(const QItemSelection&, const QItemSelection&)));

    connect(mPluginsTable, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(setCheckstate(QModelIndex)));
    connect(mPluginsModel, SIGNAL(dataChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(resizeRows()));

    //connect(mProfileComboBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(profileChanged(const QString&)));
    connect(mProfilesComboBox, SIGNAL(textChanged(const QString&, const QString&)), this, SLOT(profileChanged(const QString&, const QString&)));


    setupDataFiles();
    setupConfig();
}

void DataFilesPage::setupDataFiles()
{
    mMastersWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    mMastersWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    mMastersWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mMastersWidget->setAlternatingRowColors(true);
    mMastersWidget->horizontalHeader()->setStretchLastSection(true);
    mMastersWidget->horizontalHeader()->hide();
    mMastersWidget->verticalHeader()->hide();
    mMastersWidget->insertColumn(0);

    mPluginsTable->setModel(mPluginsModel);
    mPluginsTable->setSelectionModel(mPluginsSelectModel);
    mPluginsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mPluginsTable->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mPluginsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mPluginsTable->setAlternatingRowColors(true);
    mPluginsTable->horizontalHeader()->setStretchLastSection(true);
    mPluginsTable->horizontalHeader()->hide();


    mPluginsTable->setDragEnabled(true);
    mPluginsTable->setDragDropMode(QAbstractItemView::InternalMove);
    mPluginsTable->setDropIndicatorShown(true);
    mPluginsTable->setDragDropOverwriteMode(false);
    mPluginsTable->viewport()->setAcceptDrops(true);

    mPluginsTable->setContextMenuPolicy(Qt::CustomContextMenu);

    // Some testing TODO TODO TODO

    QDir dataFilesDir("data/");

    if (!dataFilesDir.exists())
        qWarning("Cannot find the plugin directory");

    dataFilesDir.setNameFilters((QStringList() << "*.esp")); // Only load plugins

    QStringList dataFiles = dataFilesDir.entryList();

    for (int i=0; i<dataFiles.count(); ++i)
    {
        ESMReader fileReader;
        QString currentFile = dataFiles.at(i);
        QStringList availableMasters; // Will contain all found masters

        QString path = QString("data/").append(currentFile);
        fileReader.open(path.toStdString());

        // First we fill the availableMasters and the mMastersWidget
        ESMReader::MasterList mlist = fileReader.getMasters();

        for (unsigned int i = 0; i < mlist.size(); ++i) {
            const QString currentMaster = QString::fromStdString(mlist[i].name);
            availableMasters.append(currentMaster);

            QList<QTableWidgetItem*> itemList = mMastersWidget->findItems(currentMaster, Qt::MatchExactly);

            if (itemList.isEmpty()) // Master is not yet in the widget
            {
                mMastersWidget->insertRow(i);
                QTableWidgetItem *item = new QTableWidgetItem(currentMaster);
                mMastersWidget->setItem(i, 0, item);
            }
        }

        availableMasters.sort(); // Sort the masters alphabetically

        // Now we put the currentFile in the mDataFilesModel under its masters
        QStandardItem *parent = new QStandardItem(availableMasters.join(","));
        QStandardItem *child = new QStandardItem(currentFile);

        QList<QStandardItem*> masterList = mDataFilesModel->findItems(availableMasters.join(","));

        if (masterList.isEmpty()) { // Masters node not yet in the mDataFilesModel
            parent->appendRow(child);
            mDataFilesModel->appendRow(parent);
        } else {
            // Masters node exists, append current plugin
            foreach (QStandardItem *currentItem, masterList) {
                currentItem->appendRow(child);
            }
        }
    }
}

void DataFilesPage::setupConfig()
{
    QFile config("launcher.cfg");

    if (config.exists())
    {
        qDebug() << "Using config file from current directory";
        mLauncherConfig = new QSettings("launcher.cfg", QSettings::IniFormat);
    } else {
        QString path = QString::fromStdString(OMW::Path::getPath(OMW::Path::GLOBAL_CFG_PATH,
                                                                 "openmw",
                                                                 "launcher.cfg"));
        qDebug() << "Using global config file from " << path;
        mLauncherConfig = new QSettings(path, QSettings::IniFormat);
    }

    config.close();

    mLauncherConfig->beginGroup("Profiles");
    QStringList profiles = mLauncherConfig->childGroups();

    if (profiles.isEmpty()) {
        // Add a default profile
        profiles.append("Default");
    }

    mProfilesModel->setStringList(profiles);

    QString currentProfile = mLauncherConfig->value("CurrentProfile").toString();
    if (currentProfile.isEmpty()) {
        currentProfile = "Default";
    }
    mProfilesComboBox->setCurrentIndex(mProfilesComboBox->findText(currentProfile));

    mLauncherConfig->endGroup();

    readConfig();
}

void DataFilesPage::masterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    // TODO: Use mMasterWidget->selectedItems
    if (mMastersWidget->selectionModel()->hasSelection()) {
        const QModelIndexList selectedIndexes = mMastersWidget->selectionModel()->selectedIndexes();

        QStringList masters;
        QString masterstr;

        // Create a QStringList containing all the masters
        foreach (const QModelIndex &index, selectedIndexes) {
            masters.append(index.data().toString());
        }

        masters.sort();
        masterstr = masters.join(","); // Make a comma-separated QString

        qDebug() << "Masters" << masterstr;

        // Iterate over all masters in the datafilesmodel to see if they are selected
        for (int r=0; r<mDataFilesModel->rowCount(); ++r) {
            QModelIndex currentIndex = mDataFilesModel->index(r, 0);
            QString master = currentIndex.data().toString();

            if (currentIndex.isValid()) {
                // See if the current master is in the string with selected masters
                if (masterstr.contains(master))
                {
                    // Append the plugins from the current master to pluginsmodel
                    addPlugins(currentIndex);
                    mPluginsTable->resizeRowsToContents();
                }
            }
        }
    }

   // See what plugins to remove
   QModelIndexList deselectedIndexes = deselected.indexes();

   if (!deselectedIndexes.isEmpty()) {
        foreach (const QModelIndex &currentIndex, deselectedIndexes) {

            QString master = currentIndex.data().toString();
            master.prepend("*");
            master.append("*");
            QList<QStandardItem *> itemList = mDataFilesModel->findItems(master, Qt::MatchWildcard);

            if (itemList.isEmpty())
                qDebug() << "Empty as shit";

            foreach (const QStandardItem *currentItem, itemList) {

                QModelIndex index = currentItem->index();
                qDebug() << "Master to remove plugins of:" << index.data().toString();

                removePlugins(index);
            }
        }
   }

}

void DataFilesPage::addPlugins(const QModelIndex &index)
{
    // Find the plugins in the datafilesmodel and append them to the pluginsmodel
    if (!index.isValid())
        return;

    for (int r=0; r<mDataFilesModel->rowCount(index); ++r ) {
        QModelIndex childIndex = index.child(r, 0);

        if (childIndex.isValid()) {
            // Now we see if the pluginsmodel already contains this plugin
            const QString childIndexData = QVariant(mDataFilesModel->data(childIndex)).toString();

            QList<QStandardItem *> itemList = mPluginsModel->findItems(childIndexData);

            if (itemList.isEmpty())
            {
                // Plugin not yet in the pluginsmodel, add it
                QStandardItem *item = new QStandardItem(childIndexData);
                item->setFlags(item->flags() & ~(Qt::ItemIsDropEnabled));
                item->setCheckable(true);

                mPluginsModel->appendRow(item);
            }
        }

    }

}

void DataFilesPage::removePlugins(const QModelIndex &index)
{

    if (!index.isValid())
        return;

    for (int r=0; r<mDataFilesModel->rowCount(index); ++r) {
        QModelIndex childIndex = index.child(r, 0);

        QList<QStandardItem *> itemList = mPluginsModel->findItems(QVariant(childIndex.data()).toString());

        if (!itemList.isEmpty()) {
            foreach (const QStandardItem *currentItem, itemList) {
                qDebug() << "Remove plugin:" << currentItem->data(Qt::DisplayRole).toString();

                mPluginsModel->removeRow(currentItem->row());
            }
        }
    }

}

void DataFilesPage::setCheckstate(QModelIndex index)
{
    if (!index.isValid())
        return;

    if (mPluginsModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
        // Selected row is checked, uncheck it
        mPluginsModel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
    } else {
        mPluginsModel->setData(index, Qt::Checked, Qt::CheckStateRole);
    }
}

const QStringList DataFilesPage::checkedPlugins()
{
    QStringList checkedItems;

    for (int r=0; r<mPluginsModel->rowCount(); ++r ) {
        QModelIndex index = mPluginsModel->index(r, 0);

        if (index.isValid()) {
            // See if the current item is checked
            if (mPluginsModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
                checkedItems.append(index.data().toString());
            }
        }
    }
    return checkedItems;
}

void DataFilesPage::uncheckPlugins()
{
    for (int r=0; r<mPluginsModel->rowCount(); ++r ) {
        QModelIndex index = mPluginsModel->index(r, 0);

        if (index.isValid()) {
            // See if the current item is checked
            if (mPluginsModel->data(index, Qt::CheckStateRole) == Qt::Checked) {
                qDebug() << "Uncheck!";
                mPluginsModel->setData(index, Qt::Unchecked, Qt::CheckStateRole);
            }
        }
    }
}

void DataFilesPage::resizeRows()
{
    // Contents changed
    mPluginsTable->resizeRowsToContents();
}

void DataFilesPage::profileChanged(const QString &previous, const QString &current)
{
    qDebug() << "Profile changed " << current << previous;

    // Just to be sure
    if (!previous.isEmpty()) {
        writeConfig(previous);
        mLauncherConfig->sync();
    }

    uncheckPlugins();

    readConfig();

}

void DataFilesPage::readConfig()
{
    QString profile = mProfilesComboBox->currentText();
    qDebug() << "read from: " << profile;

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->beginGroup(profile);

    QStringList childKeys = mLauncherConfig->childKeys();
    qDebug() << childKeys << "SJILDKIEJS";

    foreach (const QString &key, childKeys) {
        const QString keyValue = mLauncherConfig->value(key).toString();

        if (key.startsWith("Plugin")) {
            QList<QStandardItem *> pluginList = mPluginsModel->findItems(keyValue);

            if (!pluginList.isEmpty())
            {
                foreach (const QStandardItem *currentPlugin, pluginList) {
                    mPluginsModel->setData(currentPlugin->index(), Qt::Checked, Qt::CheckStateRole);
                }
            }
        }

        if (key.startsWith("Master")) {
            qDebug() << "Read master: " << keyValue;
            QList<QTableWidgetItem*> masterList = mMastersWidget->findItems(keyValue, Qt::MatchExactly);

            if (!masterList.isEmpty()) {
                foreach (QTableWidgetItem *currentMaster, masterList) {
                    mMastersWidget->selectionModel()->select(mMastersWidget->model()->index(currentMaster->row(), 0), QItemSelectionModel::Select);
                }
            }
        }
    }
}

void DataFilesPage::writeConfig(QString profile)
{
    // TODO: Testing the config here
    if (profile.isEmpty()) {
        profile = mProfilesComboBox->currentText();
    }

    qDebug() << "Writing: " << profile;

    // Make sure we have no groups open
    while (!mLauncherConfig->group().isEmpty()) {
        mLauncherConfig->endGroup();
    }

    mLauncherConfig->beginGroup("Profiles");
    mLauncherConfig->setValue("CurrentProfile", profile);

    // Open the profile-name subgroup
    mLauncherConfig->beginGroup(profile);
    mLauncherConfig->remove("");

    // First write the masters to the config
    QList<QTableWidgetItem *> selectedMasters = mMastersWidget->selectedItems();

    for (int i = 0; i < selectedMasters.size(); ++i) {
        const QTableWidgetItem *item = selectedMasters.at(i);
        mLauncherConfig->setValue(QString("Master%0").arg(i), item->data(Qt::DisplayRole).toString());
    }

    // Now write all checked plugins
    const QStringList plugins = checkedPlugins();

    for (int i = 0; i < plugins.size(); ++i)
    {
        mLauncherConfig->setValue(QString("Plugin%1").arg(i), plugins.at(i));
    }

    qDebug() <<  mLauncherConfig->childKeys();
    mLauncherConfig->endGroup();
    qDebug() <<  mLauncherConfig->childKeys();
    mLauncherConfig->endGroup();

}