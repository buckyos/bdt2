import * as assert from 'assert';
import { processUptime, LoggerInstance, Peerid } from '../../../base';
import { SuperNodeTransport } from '../transport';

/**
 * 上一个请求还没来得及响应时，放弃响应，防止服务器负荷过大时，客户端还高频请求
 */

export class ResponseSender {
    constructor(options: {
        logger: LoggerInstance,
        timeout?: number,
    }) {
        this.m_logger = options.logger;
        this.m_timeout = (options.timeout || 30809) / 1000;
    }

    async send(peer: {id: Peerid, transport: SuperNodeTransport}&any, pkg: {cmdType: number, sequence?: number}&any, logKey: string, ignoreSequence?: boolean) {
        let key = `${peer.id.toString()}|${pkg.cmdType}`;
        if (!ignoreSequence) {
            key += `|${pkg.sequence}`;
        }

        const now = processUptime();
        const pendingTime = this.m_pendings.get(key);
        if (!pendingTime || now - pendingTime > this.m_timeout) {
            this.m_pendings.set(key, now);
            await peer.transport.send(pkg, logKey);
            this.m_pendings.delete(key);
        }
    }

    destroy() {
        this.m_pendings.clear();
    }

    private m_logger: LoggerInstance;
    private m_timeout: number;
    private m_pendings: Map< string, number > = new Map();
}
