#pragma once

#include "CBConfig.h"

#include <cbl++/Replicator.hh>

class ReplicatorManager
{
public:
    static std::string activityLevelToString(CBLReplicatorActivityLevel activityLevel);

    ReplicatorManager(CBLProxySettings* proxySettings);
    ~ReplicatorManager() = default;

    ReplicatorManager& setServerHostname(std::string hostname);

    ReplicatorManager& initReplicatorConfigurator(std::shared_ptr<cbl::Database> database);

    ReplicatorManager& configPublicReplicator();

    CBLReplicatorActivityLevel replicatorStatus() const;

    ReplicatorManager& registerBreadcrumbFunction(std::function<void(std::string)> f);

    ReplicatorManager& registerReportFunction(std::function<void(std::string)> f);

    void sync();

    void stop();

private:
    std::string serverAddress_;

    CBLProxySettings* proxySettings_;

    // Replicator
    std::shared_ptr<cbl::ReplicatorConfiguration> replicatorConfig_;

    std::shared_ptr<cbl::Replicator> pullReplicator_;

    CBLReplicatorActivityLevel                                                        cachedReplicatorActivity_;
    std::shared_ptr<cbl::ListenerToken<cbl::Replicator, const CBLReplicatorStatus&> > replicatorListener_;

    void syncPull();

    void stopPull();
};
