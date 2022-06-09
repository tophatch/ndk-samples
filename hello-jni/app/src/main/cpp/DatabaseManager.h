#pragma once

#include "CBConfig.h"

#include <map>

#include <cbl++/Query.hh>

typedef std::string CloudMessage;

class DatabaseManager
{
public:
    enum QueryKey
    {
        PrivateProfile = 0,
        PrivateProfileMetaId,
        UserLicense,
        UserLicenseMetaId,
        UserSettings,
        UserSettingsMetaId,
        UserProfile,
        UserProfileMetaId
    };

    DatabaseManager();
    ~DatabaseManager() = default;

    DatabaseManager& setDatabasePath(std::string databasePath);

    DatabaseManager& setServerHostname(std::string hostname);

    void loadDatabaseCreateIfNeeded();

    void closeDatabase(bool removeDatabase);

    bool isDatabaseConnected() const;

    void registerQueryCallback(QueryKey queryKey, std::function<void(std::vector<fleece::MutableDict>)> f);

    std::shared_ptr<cbl::Database> getDatabase() const;

private:
    inline static const char* kDatabaseCacheFolderName = "cache";
    inline static const char* kDatabaseFilesFolderName = "files";

    fleece::alloc_slice            databasePath_;
    fleece::alloc_slice            tmpDatabasePath_;
    DatabaseConfig                 databaseConfig_;
    std::shared_ptr<cbl::Database> localDatabase_;

    std::map<QueryKey, std::function<void(std::vector<fleece::MutableDict>)>> queryCallbacks_;
    std::map<QueryKey, cbl::Query>                                            queries_;
    std::map<QueryKey, std::shared_ptr<cbl::Query::ChangeListener>>           queryListeners_;

    const std::map<QueryKey, fleece::alloc_slice> allQueryCommands_ = {
        {QueryKey::PrivateProfile, fleece::alloc_slice("SELECT * FROM _ WHERE type='userprofile_private'")},
        {QueryKey::PrivateProfileMetaId, fleece::alloc_slice("SELECT meta().id FROM _ WHERE type='userprofile_private'")},
        {QueryKey::UserLicense, fleece::alloc_slice("SELECT * FROM _ WHERE type='user_license'")},
        {QueryKey::UserLicenseMetaId, fleece::alloc_slice("SELECT meta().id FROM _ WHERE type='user_license'")},
        {QueryKey::UserSettings, fleece::alloc_slice("SELECT * FROM _ WHERE type='usersettings'")},
        {QueryKey::UserSettingsMetaId, fleece::alloc_slice("SELECT meta().id FROM _ WHERE type='usersettings'")},
        {QueryKey::UserProfile, fleece::alloc_slice("SELECT * FROM _ WHERE type='userprofile'")},
        {QueryKey::UserProfileMetaId, fleece::alloc_slice("SELECT meta().id FROM _ WHERE type='userprofile'")}};

    const std::vector<QueryKey> allQueryKeys_ = {
        QueryKey::PrivateProfile,
        QueryKey::PrivateProfileMetaId,
        QueryKey::UserLicense,
        QueryKey::UserLicenseMetaId,
        QueryKey::UserSettings,
        QueryKey::UserSettingsMetaId,
        QueryKey::UserProfile,
        QueryKey::UserProfileMetaId};

    const std::vector<QueryKey> allMetaKeys_ = {
        QueryKey::PrivateProfileMetaId,
        QueryKey::UserLicenseMetaId,
        QueryKey::UserSettingsMetaId,
        QueryKey::UserProfileMetaId};

    template <typename T>
    static T convertQueryResultToType(cbl::Result r);

    void createQuery(QueryKey key, fleece::alloc_slice request, bool addListener);

    void registerQueryListener(QueryKey queryKey);

    void createAllQueries();
};
