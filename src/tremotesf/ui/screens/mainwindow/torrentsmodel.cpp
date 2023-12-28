// SPDX-FileCopyrightText: 2015-2023 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "torrentsmodel.h"

#include <limits>

#include <QCoreApplication>
#include <QIcon>
#include <QMetaEnum>

#include "libtremotesf/stdutils.h"
#include "libtremotesf/torrent.h"
#include "tremotesf/rpc/trpc.h"
#include "tremotesf/desktoputils.h"
#include "tremotesf/utils.h"

namespace tremotesf {
    using libtremotesf::Torrent;
    using libtremotesf::TorrentData;

    TorrentsModel::TorrentsModel(Rpc* rpc, QObject* parent) : QAbstractTableModel(parent), mRpc(nullptr) {
        setRpc(rpc);
    }

    int TorrentsModel::columnCount(const QModelIndex&) const { return QMetaEnum::fromType<Column>().keyCount(); }

    QVariant TorrentsModel::data(const QModelIndex& index, int role) const {
        if (!index.isValid()) {
            return {};
        }
        Torrent* torrent = mRpc->torrents().at(static_cast<size_t>(index.row())).get();
        switch (role) {
        case Qt::DecorationRole:
            if (static_cast<Column>(index.column()) == Column::Name) {
                using namespace desktoputils;
                if (torrent->data().error != TorrentData::Error::None) {
                    return QIcon(statusIconPath(ErroredIcon));
                }
                switch (torrent->data().status) {
                case TorrentData::Status::Paused:
                    return QIcon(statusIconPath(PausedIcon));
                case TorrentData::Status::Seeding:
                    if (torrent->data().isSeedingStalled()) {
                        return QIcon(statusIconPath(StalledSeedingIcon));
                    }
                    return QIcon(statusIconPath(SeedingIcon));
                case TorrentData::Status::Downloading:
                    if (torrent->data().isDownloadingStalled()) {
                        return QIcon(statusIconPath(StalledDownloadingIcon));
                    }
                    return QIcon(statusIconPath(DownloadingIcon));
                case TorrentData::Status::QueuedForDownloading:
                case TorrentData::Status::QueuedForSeeding:
                    return QIcon(statusIconPath(QueuedIcon));
                case TorrentData::Status::Checking:
                case TorrentData::Status::QueuedForChecking:
                    return QIcon(statusIconPath(CheckingIcon));
                }
            }
            break;
        case Qt::DisplayRole:
            switch (static_cast<Column>(index.column())) {
            case Column::Name:
                return torrent->data().name;
            case Column::SizeWhenDone:
                {
                    QString sizeWhenDone = Utils::formatByteSize(torrent->data().sizeWhenDone);
                    return sizeWhenDone == "0 B" ? "" : sizeWhenDone;
                }
            case Column::TotalSize:
                {
                    QString totalSize = Utils::formatByteSize(torrent->data().totalSize);
                    return totalSize == "0 B" ? "" : totalSize;
                }
            case Column::Progress:
                if (torrent->data().status == TorrentData::Status::Checking) {
                    return Utils::formatProgress(torrent->data().recheckProgress);
                }
                return Utils::formatProgress(torrent->data().percentDone);
            case Column::Status: {
                switch (torrent->data().status) {
                case TorrentData::Status::Paused:
                    if (torrent->data().hasError()) {
                        //: Torrent status while torrent also has an error. %1 is error string
                        return qApp->translate("tremotesf", "Paused (%1)").arg(torrent->data().errorString);
                    }
                    //: Torrent status
                    return qApp->translate("tremotesf", "Paused");
                case TorrentData::Status::Downloading:
                    if (torrent->data().hasError()) {
                        //: Torrent status while torrent also has an error. %1 is error string
                        return qApp->translate("tremotesf", "Downloading (%1)").arg(torrent->data().errorString);
                    }
                    //: Torrent status
                    return qApp->translate("tremotesf", "Downloading", "Torrent status");
                case TorrentData::Status::Seeding:
                    if (torrent->data().hasError()) {
                        //: Torrent status while torrent also has an error. %1 is error string
                        return qApp->translate("tremotesf", "Seeding (%1)").arg(torrent->data().errorString);
                    }
                    //: Torrent status
                    return qApp->translate("tremotesf", "Seeding", "Torrent status");
                case TorrentData::Status::QueuedForDownloading:
                case TorrentData::Status::QueuedForSeeding:
                    if (torrent->data().hasError()) {
                        //: Torrent status while torrent also has an error. %1 is error string
                        return qApp->translate("tremotesf", "Queued (%1)").arg(torrent->data().errorString);
                    }
                    //: Torrent status
                    return qApp->translate("tremotesf", "Queued");
                case TorrentData::Status::Checking:
                    if (torrent->data().hasError()) {
                        //: Torrent status while torrent also has an error. %1 is error string
                        return qApp->translate("tremotesf", "Checking (%1)").arg(torrent->data().errorString);
                    }
                    //: Torrent status
                    return qApp->translate("tremotesf", "Checking");
                case TorrentData::Status::QueuedForChecking:
                    if (torrent->data().hasError()) {
                        //: Torrent status while torrent also has an error. %1 is error string
                        return qApp->translate("tremotesf", "Queued for checking (%1)")
                            .arg(torrent->data().errorString);
                    }
                    //: Torrent status
                    return qApp->translate("tremotesf", "Queued for checking");
                }
                break;
            }
            case Column::Priority:
                switch (torrent->data().bandwidthPriority) {
                case TorrentData::Priority::High:
                    //: Torrent's loading priority
                    return qApp->translate("tremotesf", "High");
                case TorrentData::Priority::Normal:
                    //: Torrent's loading priority
                    return qApp->translate("tremotesf", "Normal");
                case TorrentData::Priority::Low:
                    //: Torrent's loading priority
                    return qApp->translate("tremotesf", "Low");
                }
                break;
            case Column::QueuePosition:
                return torrent->data().queuePosition;
            case Column::Seeders:
                {
                    int totalSeedersFromTrackersCount = torrent->data().totalSeedersFromTrackersCount;
                    return totalSeedersFromTrackersCount == 0 ? "" : QString::number(totalSeedersFromTrackersCount);
                }
            case Column::Leechers:
                {
                    int totalLeechersFromTrackersCount = torrent->data().totalLeechersFromTrackersCount;
                    return totalLeechersFromTrackersCount == 0 ? "" : QString::number(totalLeechersFromTrackersCount);
                }
            case Column::PeersSendingToUs:
                {
                    int peersSendingToUsCount = torrent->data().peersSendingToUsCount;
                    return peersSendingToUsCount == 0 ? "" : QString::number(peersSendingToUsCount);
                }
            case Column::PeersGettingFromUs:
                {
                    int peersGettingFromUsCount = torrent->data().peersGettingFromUsCount;
                    return peersGettingFromUsCount == 0 ? "" : QString::number(peersGettingFromUsCount);
                }
            case Column::DownloadSpeed:
                {
                    QString downloadSpeed = Utils::formatByteSpeed(torrent->data().downloadSpeed);
                    return downloadSpeed == "0 B/s" ? "" : downloadSpeed;
                }
            case Column::UploadSpeed:
                {
                    QString uploadSpeed = Utils::formatByteSpeed(torrent->data().uploadSpeed);
                    return uploadSpeed == "0 B/s" ? "" : uploadSpeed;
                }
            case Column::Eta:
                return Utils::formatEta(torrent->data().eta);
            case Column::Ratio:
                return Utils::formatRatio(torrent->data().ratio);
            case Column::AddedDate:
                return torrent->data().addedDate.toLocalTime();
            case Column::DoneDate:
                return torrent->data().doneDate.toLocalTime();
            case Column::DownloadSpeedLimit:
                if (torrent->data().downloadSpeedLimited) {
                    return Utils::formatSpeedLimit(torrent->data().downloadSpeedLimit);
                }
                break;
            case Column::UploadSpeedLimit:
                if (torrent->data().uploadSpeedLimited) {
                    return Utils::formatSpeedLimit(torrent->data().uploadSpeedLimit);
                }
                break;
            case Column::TotalDownloaded:
                {
                    QString totalDownloaded = Utils::formatByteSize(torrent->data().totalDownloaded);
                    return totalDownloaded == "0 B" ? "" : totalDownloaded;
                }
            case Column::TotalUploaded:
                {
                    QString totalUploaded = Utils::formatByteSize(torrent->data().totalUploaded);
                    return totalUploaded == "0 B" ? "" : totalUploaded;
                }
            case Column::LeftUntilDone:
                {
                    QString leftUntilDone = Utils::formatByteSize(torrent->data().leftUntilDone);
                    return leftUntilDone == "0 B" ? "" : leftUntilDone;
                }
            case Column::DownloadDirectory:
                return torrent->data().downloadDirectory;
            case Column::CompletedSize:
                {
                    QString completedSize = Utils::formatByteSize(torrent->data().completedSize);
                    return completedSize == "0 B" ? "" : completedSize;
                }
            case Column::ActivityDate:
                return torrent->data().activityDate.toLocalTime();
            default:
                break;
            }
            break;
        case Qt::ToolTipRole:
            switch (static_cast<Column>(index.column())) {
            case Column::Name:
            case Column::Status:
            case Column::AddedDate:
            case Column::DoneDate:
            case Column::DownloadDirectory:
            case Column::ActivityDate:
                return data(index, Qt::DisplayRole);
            default:
                break;
            }
            break;
        case Qt::TextAlignmentRole:
            switch (static_cast<Column>(index.column())) {
            case Column::SizeWhenDone:
            case Column::Eta:
            case Column::TotalSize:
            case Column::Progress:
            case Column::QueuePosition:
            case Column::Seeders:
            case Column::Leechers:
            case Column::PeersSendingToUs:
            case Column::PeersGettingFromUs:
            case Column::DownloadSpeed:
            case Column::UploadSpeed:
            case Column::Ratio:
            case Column::DownloadSpeedLimit:
            case Column::UploadSpeedLimit:
            case Column::TotalDownloaded:
            case Column::TotalUploaded:
            case Column::LeftUntilDone:
            case Column::CompletedSize:
                return static_cast<Qt::Alignment::Int>(Qt::AlignRight | Qt::AlignVCenter);
            }
            break;
        case static_cast<int>(Role::Sort):
            switch (static_cast<Column>(index.column())) {
            case Column::SizeWhenDone:
                return torrent->data().sizeWhenDone;
            case Column::TotalSize:
                return torrent->data().totalSize;
            case Column::ProgressBar:
            case Column::Progress:
                if (torrent->data().status == TorrentData::Status::Checking) {
                    return torrent->data().recheckProgress;
                }
                return torrent->data().percentDone;
            case Column::Priority:
                return static_cast<int>(torrent->data().bandwidthPriority);
            case Column::Status:
                return static_cast<int>(torrent->data().status);
            case Column::DownloadSpeed:
                return torrent->data().downloadSpeed;
            case Column::UploadSpeed:
                return torrent->data().uploadSpeed;
            case Column::Eta: {
                const auto eta = torrent->data().eta;
                if (eta < 0) {
                    return std::numeric_limits<decltype(eta)>::max();
                }
                return eta;
            }
            case Column::Ratio:
                return torrent->data().ratio;
            case Column::AddedDate:
                return torrent->data().addedDate;
            case Column::DoneDate:
                return torrent->data().doneDate;
            case Column::DownloadSpeedLimit:
                if (torrent->data().downloadSpeedLimited) {
                    return torrent->data().downloadSpeedLimit;
                }
                return -1;
            case Column::UploadSpeedLimit:
                if (torrent->data().uploadSpeedLimited) {
                    return torrent->data().uploadSpeedLimit;
                }
                return -1;
            case Column::TotalDownloaded:
                return torrent->data().totalDownloaded;
            case Column::TotalUploaded:
                return torrent->data().totalUploaded;
            case Column::LeftUntilDone:
                return torrent->data().leftUntilDone;
            case Column::CompletedSize:
                return torrent->data().completedSize;
            case Column::ActivityDate:
                return torrent->data().activityDate;
            default:
                return data(index, Qt::DisplayRole);
            }
        case static_cast<int>(Role::TextElideMode):
            if (static_cast<Column>(index.column()) == Column::DownloadDirectory) {
                return Qt::ElideMiddle;
            }
            return Qt::ElideRight;
        }
        return {};
    }

    QVariant TorrentsModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
            return {};
        }
        switch (static_cast<Column>(section)) {
        case Column::Name:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Name");
        case Column::SizeWhenDone:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Size");
        case Column::TotalSize:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Total Size");
        case Column::ProgressBar:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Progress Bar");
        case Column::Progress:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Progress");
        case Column::Priority:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Priority");
        case Column::QueuePosition:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Queue Position");
        case Column::Status:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Status");
        case Column::Seeders:
            //: Torrents list column name, number of seeders reported by trackers
            return qApp->translate("tremotesf", "Seeders");
        case Column::Leechers:
            //: Torrents list column name, number of leechers reported by trackers
            return qApp->translate("tremotesf", "Leechers");
        case Column::PeersSendingToUs:
            //: Torrents list column name, number of peers that we are downloading from
            return qApp->translate("tremotesf", "Downloading to peers");
        case Column::PeersGettingFromUs:
            //: Torrents list column name, number of peers that we are uploading to
            return qApp->translate("tremotesf", "Uploading to peers");
        case Column::DownloadSpeed:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Down Speed");
        case Column::UploadSpeed:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Up Speed");
        case Column::Eta:
            //: Torrents list column name
            return qApp->translate("tremotesf", "ETA");
        case Column::Ratio:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Ratio");
        case Column::AddedDate:
            //: Torrents list column name, date/time when torrent was added
            return qApp->translate("tremotesf", "Added on");
        case Column::DoneDate:
            //: Torrents list column name, date/time when torrent was completed
            return qApp->translate("tremotesf", "Completed on");
        case Column::DownloadSpeedLimit:
            //: Torrents list column name, download speed limit
            return qApp->translate("tremotesf", "Down Limit");
        case Column::UploadSpeedLimit:
            //: Torrents list column name, upload speed limit
            return qApp->translate("tremotesf", "Up Limit");
        case Column::TotalDownloaded:
            //: Torrents list column name, downloaded byte size
            return qApp->translate("tremotesf", "Downloaded");
        case Column::TotalUploaded:
            //: Torrents list column name, uploaded byte size
            return qApp->translate("tremotesf", "Uploaded");
        case Column::LeftUntilDone:
            //: Torrents list column name, remaining byte size
            return qApp->translate("tremotesf", "Remaining");
        case Column::DownloadDirectory:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Download Directory");
        case Column::CompletedSize:
            //: Torrents list column name, completed byte size
            return qApp->translate("tremotesf", "Completed");
        case Column::ActivityDate:
            //: Torrents list column name
            return qApp->translate("tremotesf", "Last Activity");
        default:
            return {};
        }
    }

    int TorrentsModel::rowCount(const QModelIndex&) const { return static_cast<int>(mRpc->torrentsCount()); }

    Rpc* TorrentsModel::rpc() const { return mRpc; }

    void TorrentsModel::setRpc(Rpc* rpc) {
        if (rpc != mRpc) {
            if (mRpc) {
                QObject::disconnect(mRpc, nullptr, this, nullptr);
            }
            mRpc = rpc;
            if (rpc) {
                QObject::connect(rpc, &Rpc::onAboutToAddTorrents, this, [=, this](size_t count) {
                    const auto first = mRpc->torrentsCount();
                    beginInsertRows({}, first, first + static_cast<int>(count) - 1);
                });

                QObject::connect(rpc, &Rpc::onAddedTorrents, this, [=, this] { endInsertRows(); });

                QObject::connect(rpc, &Rpc::onAboutToRemoveTorrents, this, [=, this](size_t first, size_t last) {
                    beginRemoveRows({}, static_cast<int>(first), static_cast<int>(last - 1));
                });

                QObject::connect(rpc, &Rpc::onRemovedTorrents, this, [=, this] { endRemoveRows(); });

                QObject::connect(rpc, &Rpc::onChangedTorrents, this, [=, this](size_t first, size_t last) {
                    emit dataChanged(
                        index(static_cast<int>(first), 0),
                        index(static_cast<int>(last), columnCount() - 1)
                    );
                });

                const auto count = rpc->torrentsCount();
                if (count != 0) {
                    beginInsertRows({}, 0, count - 1);
                    endInsertRows();
                }
            }
        }
    }

    Torrent* TorrentsModel::torrentAtIndex(const QModelIndex& index) const { return torrentAtRow(index.row()); }

    Torrent* TorrentsModel::torrentAtRow(int row) const { return mRpc->torrents()[static_cast<size_t>(row)].get(); }

    std::vector<int> TorrentsModel::idsFromIndexes(const QModelIndexList& indexes) const {
        return createTransforming<std::vector<int>>(indexes, [this](const QModelIndex& index) {
            return torrentAtIndex(index)->data().id;
        });
    }
}
