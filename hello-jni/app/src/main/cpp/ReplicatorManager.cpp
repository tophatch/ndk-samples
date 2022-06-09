#include "ReplicatorManager.h"

#include <thread>

extern "C"
{
void cpp_android_log_print(const char *tag, const char *format, ...) {

    va_list args;
    va_start(args, format);
    char buffer[512] = {};
    vsnprintf(buffer, 512, format, args);
    __android_log_print(ANDROID_LOG_DEBUG, tag, format, args);
    va_end(args);
}
}

std::string ReplicatorManager::activityLevelToString(CBLReplicatorActivityLevel activityLevel)
{
    switch (activityLevel)
    {
    case kCBLReplicatorStopped:
        return "stopped";
    case kCBLReplicatorOffline:
        return "offline";
    case kCBLReplicatorConnecting:
        return "connecting";
    case kCBLReplicatorIdle:
        return "idle";
    case kCBLReplicatorBusy:
        return "busy";
    }
    return "";
}

ReplicatorManager::ReplicatorManager(CBLProxySettings* proxySettings)
    : proxySettings_(proxySettings)
    , pullReplicator_(nullptr)
    , cachedReplicatorActivity_(CBLReplicatorActivityLevel::kCBLReplicatorStopped)
{
    // This reference should never be null
    if (!proxySettings_)
    {
         throw AccountsException::NullException_ProxySettingsIsNull;
    }
}

ReplicatorManager& ReplicatorManager::setServerHostname(std::string hostname)
{
    serverAddress_ = hostname;

    return *this;
}

ReplicatorManager& ReplicatorManager::initReplicatorConfigurator(std::shared_ptr<cbl::Database> database)
{
    std::string wrappedServerAddress = "wss://";

    wrappedServerAddress = wrappedServerAddress + serverAddress_ + ":443/db";

    if (replicatorConfig_)
    {
        replicatorConfig_.reset();
        replicatorConfig_ = nullptr;
    }

    replicatorConfig_ = std::make_shared<cbl::ReplicatorConfiguration>(*database);

    configPublicReplicator();

    return *this;
}

ReplicatorManager& ReplicatorManager::configPublicReplicator()
{
    thAccountsLogDebug("[ReplicatorManager]", "Public replicator configuration.");

    if (!replicatorConfig_)
    {
        throw AccountsException::NullException_ReplicatorConfigurationIsNull;
    }

    std::string wrappedServerAddress = "wss://";

    wrappedServerAddress = wrappedServerAddress + serverAddress_ + ":443/db";

    replicatorConfig_->replicatorType = kDefaultReplicatorConfiguration.replicatorType;
    replicatorConfig_->continuous     = kDefaultReplicatorConfiguration.continuous;

    replicatorConfig_->endpoint.setURL(fleece::slice(wrappedServerAddress.c_str()));

    if (proxySettings_->port > 0)
    {
        replicatorConfig_->proxy = proxySettings_;
    }
    else
    {
        replicatorConfig_->proxy = nullptr;
    }

    replicatorConfig_->headers = kDefaultReplicatorConfiguration.headers;

    replicatorConfig_->pushFilter = kDefaultReplicatorConfiguration.pushFilter;
    replicatorConfig_->pullFilter = kDefaultReplicatorConfiguration.pullFilter;

    replicatorConfig_->trustedRootCertificates = TRUSTED_SSL_PUBLIC_CERTIFICATE;

    thAccountsLogDebug("[ReplicatorManager]", "Using replicator address: " + wrappedServerAddress);

    return *this;
}

CBLReplicatorActivityLevel ReplicatorManager::replicatorStatus() const
{
    if (!pullReplicator_)
    {
        thAccountsLogDebug("[ReplicatorManager]", "Replicator Status: Stopped.");
        return CBLReplicatorActivityLevel::kCBLReplicatorStopped;
    }

    thAccountsLogDebug("[ReplicatorManager]", "Replicator Status: " + std::to_string((int)pullReplicator_->status().activity));
    return pullReplicator_->status().activity;
}

void ReplicatorManager::sync()
{
    replicatorConfig_->replicatorType = kCBLReplicatorTypePull;

    replicatorConfig_->authenticator.setBasic(kDefaultReplicatorConfiguration.username,
                                              kDefaultReplicatorConfiguration.password);
    syncPull();

}

void ReplicatorManager::syncPull()
{
    // Instantiate the replicator
    if (!pullReplicator_)
    {
        pullReplicator_ = std::make_shared<cbl::Replicator>(*replicatorConfig_);
    }

    // Attach change listener to the replicator
    replicatorListener_ = std::make_shared<cbl::ListenerToken<cbl::Replicator, const CBLReplicatorStatus&>>(
        pullReplicator_->addChangeListener([this](cbl::Replicator r, const CBLReplicatorStatus& status) {
            CBLReplicatorActivityLevel newStatus = status.activity;
            if (cachedReplicatorActivity_ != newStatus || status.error.code)
            {
                thAccountsLogDebug("[ReplicatorManager]", ("Replicator status updated: " + activityLevelToString(cachedReplicatorActivity_) + " -> " +
                                         activityLevelToString(newStatus)).c_str());

                cachedReplicatorActivity_ = newStatus;
            }
        }));

    auto startTime = std::chrono::system_clock::now();
    // Start the replicator
    pullReplicator_->start( true);

    thAccountsLogDebug("[ReplicatorManager]", "Replicator started.(Pull)");
    while (pullReplicator_->status().activity != kCBLReplicatorIdle)
    {
        if (pullReplicator_->status().activity == kCBLReplicatorOffline)
        {
            break;
        }
        if (pullReplicator_->status().activity == kCBLReplicatorStopped)
        {
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::chrono::duration<double> diff = std::chrono::system_clock::now() - startTime;

    thAccountsLogDebug("[ReplicatorManager]", ("Finished database replication in " + std::to_string(diff.count()) + " seconds.(Pull)").c_str());

}

void ReplicatorManager::stop()
{
    stopPull();
}

void ReplicatorManager::stopPull()
{
    thAccountsLogDebug("[ReplicatorManager]", "Stopping the replicator.(Pull)");


     // Reset the replicator
    if (pullReplicator_)
    {
        if (pullReplicator_->status().activity != kCBLReplicatorStopped)
        {
            pullReplicator_->stop();
        }
        // Block until the connection close
        while (pullReplicator_->status().activity != kCBLReplicatorStopped)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    if (replicatorListener_)
    {
        replicatorListener_->remove();

        replicatorListener_.reset();
        replicatorListener_ = nullptr;
    }


    if (pullReplicator_)
    {
        pullReplicator_.reset();
        pullReplicator_ = nullptr;
    }
}
