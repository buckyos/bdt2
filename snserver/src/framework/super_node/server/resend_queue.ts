import * as assert from 'assert';
import { processUptime, LoggerInstance, ErrorCode, Peerid } from '../../../base';
import { Peer } from './peer';

/**
 * 重传队列
 */

type PackageInfo = {
    pkg: {cmdType: number, sequence?: number}&any,
    peer: Peer,
    interval: number,
    times: number,
    lastTime: number,
    logKey: string,
};

export type ResendQueueConfig = {
    defaultInterval: number, 
    maxTimes: number
};

export class ResendQueue {
    constructor(options: {
        logger: LoggerInstance,
        config: ResendQueueConfig
    }) {
        this.m_logger = options.logger;
        this.m_config = options.config;
        this.m_sendTimeout = this.m_config.defaultInterval * this.m_config.maxTimes;
    }

    async send(peer: Peer, pkg: {cmdType: number, sequence?: number}&any, logKey: string): Promise<ErrorCode> {
        const seq = pkg.sequence;
        const now = processUptime();

        let pkgInfo: PackageInfo | undefined = this.m_packages.get(seq);
        if (!pkgInfo) {
            pkgInfo = {
                pkg,
                peer,
                interval: this.m_config.defaultInterval,
                times: 0,
                lastTime: now,
                logKey,
            };
            this.m_packages.set(seq, pkgInfo);
        } else {
            pkgInfo.pkg = pkg;
            pkgInfo.times >>= 1;
            pkgInfo.interval >>= 1;
        }

        let err: ErrorCode = ErrorCode.pending;
        const lastSendTime = this.m_pendings.get(seq);
        if (!lastSendTime || now - lastSendTime > this.m_sendTimeout) {
            this.m_pendings.set(seq, now);
            if (pkgInfo) {
                pkgInfo.lastTime = now;
                pkgInfo.times++;
            }
            err = await peer.transport.send(pkg, logKey);
            this.m_pendings.delete(seq);
            if (!err && peer.transport.reliable) {
                this.m_packages.delete(seq);
            }
        }
        return err;
    }

    confirm(pkg: {cmdType: number, sequence?: number}&any) {
        this.m_packages.delete(pkg.sequence);
    }

    onTimer() {
        const now = processUptime();

        let willRemove: number[] = [];

        const doSend = async (pkgInfo: PackageInfo, seq: number) => {
            const lastSendTime = this.m_pendings.get(seq);
            if (!lastSendTime || now - lastSendTime > this.m_sendTimeout) {
                this.m_pendings.set(seq, now);
                pkgInfo.lastTime = now;
                pkgInfo.times++;
                const err = await pkgInfo.peer.transport.send(pkgInfo.pkg, pkgInfo.logKey);
                this.m_pendings.delete(seq);
                    
                if (!err && pkgInfo.peer.transport.reliable) {
                    this.m_packages.delete(seq);
                }
            }
        };

        for (let [seq, pkgInfo] of this.m_packages) {
            if (now - pkgInfo.lastTime >= pkgInfo.interval) {
                pkgInfo.interval <<= 1;
                doSend(pkgInfo, seq);
            }
            if (pkgInfo.times >= this.m_config.maxTimes) {
                willRemove.push(seq);
            }
        }

        willRemove.forEach((seq) => this.m_packages.delete(seq));
    }

    clear() {
        this.m_packages.clear();
        this.m_pendings.clear();
    }

    private m_logger: LoggerInstance;
    private m_config: ResendQueueConfig;
    private m_sendTimeout: number;
    private m_packages: Map< number, PackageInfo > = new Map();
    private m_pendings: Map< number, number > = new Map();
}
