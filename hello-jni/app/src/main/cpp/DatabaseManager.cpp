#include "DatabaseManager.h"

#include <cbl/CBLPlatform.h>


DatabaseManager::DatabaseManager()
    : databasePath_(nullptr)
    , tmpDatabasePath_(nullptr)
    , localDatabase_(nullptr)
{
}

DatabaseManager& DatabaseManager::setDatabasePath(std::string databasePath)
{

    if (databasePath.empty())
    {
        thAccountsLogDebug("[DatabaseManager]", "%s", "EMPTY");
    }
    else
    {
        thAccountsLogDebug("[DatabaseManager]", databasePath.c_str());
    }

    if (databasePath.empty())
    {
        throw AccountsException::NullException_DatabasePathEmpty;
    }

    databasePath_ = fleece::alloc_slice(databasePath.c_str());

    const char* databaseFolderName = kDatabaseFilesFolderName;

    tmpDatabasePath_ = databasePath.replace(databasePath.find(databaseFolderName),
                                            databasePath.find(databaseFolderName) + strlen(databaseFolderName), kDatabaseCacheFolderName);

    CBLError error;

    auto           filesDir = databasePath_.asString();
    auto           tempDir  = tmpDatabasePath_.asString();
    CBLInitContext context  = {filesDir.c_str(), tempDir.c_str()};
    CBL_Init(context, &error);

    if (error.code == kCBLErrorNotFound)
    {
        thAccountsLogDebug("[DatabaseManager]", "Setting up database path failed.");
    }

    thAccountsLogDebug("[DatabaseManager]", "Database path: " + databasePath_.asString());
    thAccountsLogDebug("[DatabaseManager]", "Temporary database path " + tmpDatabasePath_.asString());

    return *this;
}

DatabaseManager& DatabaseManager::setServerHostname(std::string hostname)
{
    databaseConfig_ = {fleece::alloc_slice("db"), fleece::alloc_slice("wss://" + hostname + "/")};

    return *this;
}

void DatabaseManager::registerQueryListener(QueryKey queryKey)
{
    thAccountsLogDebug("[Database Manager]", "Add listener for the key " + std::to_string((int)queryKey));
    queryListeners_[queryKey] = std::make_shared<cbl::Query::ChangeListener>(
        queries_[queryKey].addChangeListener([&, queryKey{queryKey}](cbl::Query::Change change) {
            thAccountsLogDebug("[Database Manager]", "Query changed callback called " + std::to_string(queryKey));
            cbl::ResultSet rs = change.results();
            if (!rs.valid())
            {
                thAccountsLogDebug("[Database Manager]", ("The result set for the key " + std::to_string(queryKey) + "is not valid.").c_str());
                return;
            }
            cbl::ResultSetIterator           it = rs.begin();
            std::vector<fleece::MutableDict> results;

            while (it != rs.end())
            {
                cbl::Result ri = *it;
                results.push_back(ri[0].asDict().mutableCopy());
                ++it;
            }

            thAccountsLogDebug("[Database Manager]", ("Query changed callback, query ID: " + std::to_string(queryKey)).c_str());
            if (queryCallbacks_[queryKey])
            {
                queryCallbacks_[queryKey](results);
            }
            else
            {
                thAccountsLogDebug("[Database Manager]", ("The callback for " + std::to_string(queryKey) + " is empty").c_str());
            }
        }));
}

void DatabaseManager::registerQueryCallback(QueryKey queryKey, std::function<void(std::vector<fleece::MutableDict>)> f)
{
    queryCallbacks_[queryKey] = f;
}

void DatabaseManager::loadDatabaseCreateIfNeeded()
{
    if (!databasePath_)
    {
        throw AccountsException::NullException_DatabasePathEmpty;
    }

    try
    {
        CBLDatabaseConfiguration config = {databasePath_};

        localDatabase_ = std::make_shared<cbl::Database>(databaseConfig_.dbName, config);

        thAccountsLogDebug("[Database Manager]", "Local database created.");

        createAllQueries();

        thAccountsLogDebug("[Database Manager]", "Database successfully created.");

    }
    catch (CBLError err)
    {
        if (err.code == kCBLErrorDatabaseTooNew || err.code == kCBLErrorDatabaseTooOld)
        {
            cbl::Database::deleteDatabase(databaseConfig_.dbName, databasePath_);
        }
    }
}

void DatabaseManager::closeDatabase(bool removeDatabase)
{
    std::string params = removeDatabase ? "TRUE" : "FALSE";

    thAccountsLogDebug("[DatabaseManager]", ("Stop and remove the database." + params).c_str());

    // Remove queries
    queries_.clear();

    // Close / remove database
    if (localDatabase_)
    {
        thAccountsLogDebug("[Database Manager]", "Local database closing.");
        localDatabase_->close();

        if (removeDatabase)
        {
            localDatabase_->deleteDatabase();
            localDatabase_.reset();
            localDatabase_ = nullptr;
        }
    }
    else
    {
        if (removeDatabase)
        {
            cbl::Database::deleteDatabase(databaseConfig_.dbName, databasePath_);
        }
    }
}

void DatabaseManager::createQuery(QueryKey key, fleece::alloc_slice request, bool addListener)
{
    if (!localDatabase_)
    {
        throw AccountsException::NullException_Database;
    }

    if (queries_.find(key) != queries_.end())
    {
        throw AccountsException::NullException_QuerySetShouldBeEmpty;
    }

    try
    {
        queries_.insert({key, cbl::Query(*localDatabase_, kCBLN1QLLanguage, request)});

        thAccountsLogDebug("[Database Manager]", "Created [" + request.asString() + "] query.");

        if (addListener)
        {
            registerQueryListener(key);
        }
    }
    catch (CBLError err)
    {
        thAccountsLogDebug("[Database Manager]", "N10Q Query could not be created " + request.asString());
        thAccountsLogDebug("[Database Manager]",  ("Error " + std::to_string((int)err.domain) + "/" + std::to_string(err.code)).c_str());
    }
}

template <>
fleece::MutableDict DatabaseManager::convertQueryResultToType(cbl::Result r)
{
    return r[0].asDict().mutableCopy();
}

template <>
std::string DatabaseManager::convertQueryResultToType(cbl::Result r)
{
    return r[0].asstring();
}

void DatabaseManager::createAllQueries()
{
    thAccountsLogDebug("[Database Manager]", "Create all queries.");

    for (auto& queryKey: allQueryKeys_)
    {
        bool isRegularKey = std::find(allMetaKeys_.begin(), allMetaKeys_.end(), queryKey) == allMetaKeys_.end();

        createQuery(queryKey, allQueryCommands_.at(queryKey), isRegularKey);

    }
}

std::shared_ptr<cbl::Database> DatabaseManager::getDatabase() const
{
    return localDatabase_;
}

bool DatabaseManager::isDatabaseConnected() const
{
    if (!localDatabase_)
        return false;

    return localDatabase_->valid();
}
