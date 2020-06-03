import { isUndefined, isObject } from 'util';
import { ErrorCode } from '../base';

const defaultConfig = {
    sessionManager: {
        singletonSession: {
            connectTimeout: 10 * 1000, 
            tcpSession: {
                connectTimeout: 1 * 1000
            }, 
            tunnelSession: {
                allowHalfOpen: false,
                msl: 1 * 1000,
                connectTimeout: 5 * 1000, 
                handshakeDelay: 100, 
                ackDelay: 100
            }
        }   
    },

    tunnelManager: {
        tunnelContainer: {
            connectTimeout: 10 * 1000, 
            reconnectInterval: 2 * 1000, 
            udpTunnel: {
                firstPkgResendInterval: 500, 
                pingInterval: 2 * 60 * 1000, 
                connectTimeout: 5 * 1000
            }, 
            tcpTunnel: {
                connectTimeout: 1 * 1000
            }
        }
    }, 
    
    snClient: {
        onlineSNLimit: 3, // SN上线数量限制
        snRefreshIntervalMS: 600 * 1000, // SN列表更新间隔
        pingIntervalInitMS: 500, // 初始上线期间PING间隔
        pingIntervalMS: 25 * 1000, // 正常运行期间PING间隔
        offlineIntervalMS: 300 * 1000, // SN离线判定间隔

        callRefreshSNIntervalMS: 5 * 1000, // CALL操作更新SN间隔
        callSNLimit: 3, // 查询CALL的SN数量限制
        callIntervalMS: 200 * 1000, // CALL时间间隔
        tcpDelayMS: 500, // TCP通道启动延迟时间
        callTimeoutMS: 30 * 100, // CALL超时时间
    },

    snServer: {
        tcpFreeInterval: 5 * 1000, 
        peerManager: {
            timeout: 60809, 
            capacity: 160809
        }, 
        resendQueue: {
            defaultInterval: 489, 
            maxTimes: 5
        }
    },

    peerHistoryManagerConfig: {
        keyExpire: 24 * 3600000,
        peerExpire: 24 * 3600000,
    }
};

function deepMerge(left: any, right: any): any {
    if (isUndefined(left)) {
        return right;
    }
    if (!isObject(left)) {
        return left;
    }
    let merged = Object.create(null);
    for (const [k, v] of Object.entries(right)) {
        merged[k] = deepMerge(left[k], v);
    }
    return merged;
}

export function mergeDefaultConfig(config: any): {err: ErrorCode, merged: any} {
    const merged = deepMerge(config, defaultConfig);
    return {err: ErrorCode.success, merged};
}