#include "Include_i.h"

namespace Canary {

Peer::Peer(const Canary::PConfig pConfig) : pConfig(pConfig), pAwsCredentialProvider(nullptr)
{
}

STATUS Peer::init()
{
    STATUS retStatus = STATUS_SUCCESS;

    CHK_STATUS(createStaticCredentialProvider((PCHAR) pConfig->pAccessKey, 0, (PCHAR) pConfig->pSecretKey, 0, (PCHAR) pConfig->pSessionToken, 0,
                                              MAX_UINT64, &pAwsCredentialProvider));

CleanUp:

    if (STATUS_FAILED(retStatus)) {
        deinit();
    }

    return retStatus;
}

VOID Peer::deinit()
{
    freeStaticCredentialProvider(&pAwsCredentialProvider);
}

STATUS Peer::connect(UINT64 duration)
{
    STATUS retStatus = STATUS_SUCCESS;
    UNUSED_PARAM(duration);

    // CleanUp:

    return retStatus;
}

STATUS Peer::connectSignaling()
{
    STATUS retStatus = STATUS_SUCCESS;

    SignalingClientInfo clientInfo;
    ChannelInfo channelInfo;
    SignalingClientCallbacks clientCallbacks;

    MEMSET(&clientInfo, 0, SIZEOF(clientInfo));
    MEMSET(&channelInfo, 0, SIZEOF(channelInfo));
    MEMSET(&clientCallbacks, 0, SIZEOF(clientCallbacks));

    clientInfo.version = SIGNALING_CLIENT_INFO_CURRENT_VERSION;
    clientInfo.loggingLevel = pConfig->logLevel;

    channelInfo.version = CHANNEL_INFO_CURRENT_VERSION;
    channelInfo.pChannelName = (PCHAR) pConfig->pChannelName;
    channelInfo.pKmsKeyId = NULL;
    channelInfo.tagCount = 0;
    channelInfo.pTags = NULL;
    channelInfo.channelType = SIGNALING_CHANNEL_TYPE_SINGLE_MASTER;
    channelInfo.channelRoleType = pConfig->isMaster ? SIGNALING_CHANNEL_ROLE_TYPE_MASTER : SIGNALING_CHANNEL_ROLE_TYPE_VIEWER;
    channelInfo.cachingPolicy = SIGNALING_API_CALL_CACHE_TYPE_FILE;
    channelInfo.cachingPeriod = SIGNALING_API_CALL_CACHE_TTL_SENTINEL_VALUE;
    channelInfo.asyncIceServerConfig = TRUE;
    channelInfo.retry = TRUE;
    channelInfo.reconnect = TRUE;
    channelInfo.pCertPath = (PCHAR) pConfig->pCertPath;
    channelInfo.messageTtl = 0; // Default is 60 seconds

    clientCallbacks.customData = (UINT64) this;
    clientCallbacks.errorReportFn = [](UINT64 customData, STATUS status, PCHAR msg, UINT32 msgLen) -> STATUS {
        PPeer pPeer = (PPeer) customData;
        DLOGW("Signaling client generated an error 0x%08x - '%.*s'", status, msgLen, msg);

        UNUSED_PARAM(pPeer);
        // TODO: handle recreate signaling

        return STATUS_SUCCESS;
    };
    clientCallbacks.messageReceivedFn = [](UINT64 customData, PReceivedSignalingMessage pMsg) -> STATUS {
        STATUS retStatus = STATUS_SUCCESS;
        PPeer pPeer = (PPeer) customData;
        std::shared_ptr<Connection> pConnection;
        std::string msgClientId(pMsg->signalingMessage.peerClientId);

        auto it = std::find_if(pPeer->connections.begin(), pPeer->connections.end(),
                               [&](const std::shared_ptr<Connection>& c) { return c->id == msgClientId; });

        if (it == pPeer->connections.end()) {
            pPeer->connections.push_back(std::make_shared<Connection>(pPeer, msgClientId));
            pConnection = pPeer->connections[pPeer->connections.size() - 1];
        } else {
            pConnection = *it;
        }

        CHK_STATUS(pConnection->handleSignalingMsg(pMsg));

    CleanUp:

        return retStatus;
    };

    CHK_STATUS(createSignalingClientSync(&clientInfo, &channelInfo, &clientCallbacks, pAwsCredentialProvider, &pSignalingClientHandle));

CleanUp:

    return retStatus;
}

STATUS Peer::shutdown()
{
    STATUS retStatus = STATUS_SUCCESS;

    // TODO

    // CleanUp:

    return retStatus;
}

} // namespace Canary
