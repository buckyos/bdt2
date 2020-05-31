export * from './local_peer';
export * from './interface'; 
export * from './super_node';
export * from './config';
export { PeerHistoryManager } from './peer_history';

const assert = require('assert');
import { ErrorCode, stringifyErrorCode, EndpointList, LoggerInstance, PeerConstInfo, PeerMutableInfo, Peer } from '../base';
import { NetworkManager } from './interface';
import { SuperNodeServer, SuperNodeServerConfig } from './super_node';
import { LocalPeer } from './local_peer';
import { mergeDefaultConfig } from './config';
import { PeerHistoryManager, PeerHistoryManagerImpl, PeerHistoryManagerConfig } from './peer_history';

function initLocalPeer(logger: LoggerInstance, options: {
    constInfo: PeerConstInfo, 
    secret: Buffer, 
    mutableInfo: PeerMutableInfo
}): {err: ErrorCode, localPeer?: LocalPeer} {
    let ins = new LocalPeer({
        logger
    });
    const err = ins.init(options);
    if (err) {
        return {err};
    } 
    return {err: ErrorCode.success, localPeer: ins};
}

async function initNetworkManager(logger: LoggerInstance, options: {
    localPeer: LocalPeer, 
    historyManager: PeerHistoryManager
}): Promise<{err: ErrorCode, networkManager?: NetworkManager}> {
    let ins = new NetworkManager({logger});
    const { err, failedEndpoints } = await ins.init(options);
    // <TODO> 部分地址初始化成功也可以跑，支持这种情况吗？
    if (err) {
        logger.error(`[initNetworkManager] bind fail err:${stringifyErrorCode(err)}, fail ep: ${failedEndpoints!.toString()}`);
        return {err};
    }
    return {err: ErrorCode.success, networkManager: ins};
}
