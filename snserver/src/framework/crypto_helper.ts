const assert = require('assert');
import { ErrorCode, Peerid, LoggerInstance, stringifyErrorCode, PeerConstInfo } from '../base';
import { PackageEncryptOptions, PackageDecoder } from '../protocol'; 
import { TCPNetworkInterface, UDPNetworkInterface } from './interface';
import { PeerHistoryManager } from './peer_history';
import { LocalPeer } from './local_peer';

export abstract class CryptoSession {
    constructor(options: {logger: LoggerInstance}) { 
        this.m_logger = options.logger;
    }

    abstract clone(): {err: ErrorCode, session?: CryptoSession};
    abstract preSendPackage(): {err: ErrorCode, encrypt?: PackageEncryptOptions};
    abstract onRecvPackage(pkg: {cmeType: number} & any): ErrorCode;

    setInterface(_interface: UDPNetworkInterface | TCPNetworkInterface): ErrorCode {
        if (this.m_interface) {
            assert(false, `crypto session should not attach to multi interface`);
            this.m_logger.error(`crypto session should not attach to multi interface`);
            return ErrorCode.invalidState;
        }
        this.m_interface = _interface;
        this._onSetInterface();
        return ErrorCode.success;
    }

    abstract get encryptOptions(): PackageEncryptOptions|undefined;

    protected abstract _onSetInterface(): void;

    abstract unint(): void;

    protected m_logger: LoggerInstance;
    protected m_interface?: UDPNetworkInterface | TCPNetworkInterface;
}

export class NoneCryptoSession extends CryptoSession {

    clone(): {err: ErrorCode, session?: CryptoSession} {
        if (this.m_interface) {
            assert(false, `crypto session should not cloned after setInterface`);
            this.m_logger.error(`crypto session should not cloned after setInterface`);
            return {err: ErrorCode.invalidState};
        }
        const session = new NoneCryptoSession({
            logger: this.m_logger
        });
        return {err: ErrorCode.success, session};
    }

    get encryptOptions(): PackageEncryptOptions|undefined {
        return undefined;
    }

    protected _onSetInterface(): void {

    }

    unint(): void {

    }

    preSendPackage(): {err: ErrorCode, encrypt?: PackageEncryptOptions} {
        return {err: ErrorCode.success};
    }

    onRecvPackage(pkg: {cmeType: number} & any): ErrorCode {
        if (pkg.__encrypt) {
            this.m_logger.debug(`got encrypted package on none crypto session`);
            return ErrorCode.invalidPkg;
        }
        return ErrorCode.success;
    }
}

enum SingleKeyCryptoSessionState {
    none = 0, 
    keyCreated = 1, 
    keyUntrust = 2, 
    keyExchanged = 3
}

// TODO: expire !!
const keyExpire = 100000;

export class SingleKeyCryptoSession extends CryptoSession {
    constructor(options: {
        logger: LoggerInstance, 
        historyManager: PeerHistoryManager, 
        localPeer: LocalPeer, 
        remote: {
            peerid: Peerid, 
            publicKey: Buffer
        }
    }) {
        super(options);
        this.m_historyManager = options.historyManager;
        this.m_localPeer = options.localPeer;
        this.m_remote = options.remote;
    }

    clone(): {err: ErrorCode, session?: CryptoSession} {
        if (this.m_interface) {
            assert(false, `crypto session should not cloned after setInterface`);
            this.m_logger.error(`crypto session should not cloned after setInterface`);
            return {err: ErrorCode.invalidState};
        }
        let session = new SingleKeyCryptoSession({
            logger: this.m_logger, 
            historyManager: this.m_historyManager, 
            localPeer: this.m_localPeer, 
            remote: this.m_remote
        });
        session.m_state = this.m_state;
        session.m_key = this.m_key;
        session.m_keyHash = this.m_keyHash;
        return {err: ErrorCode.success, session};
    }

    protected _onSetInterface(): void {
        if (this.m_state !== SingleKeyCryptoSessionState.none) {
            this.m_interface!.addKey({keyHash: this.m_keyHash!, key: this.m_key!, peerid: this.m_remote.peerid});
        } 
    }

    unint(): void {
        if (this.m_interface instanceof UDPNetworkInterface) {
            if (this.m_state !== SingleKeyCryptoSessionState.none) {
                this.m_interface.removeKey(this.m_keyHash!);
            }
        }
    }

    async initKey(): Promise<ErrorCode> {
        this.m_logger.debug(`single key crypto session init key on remote ${this.m_remote.peerid.toString()}`);
        if (this.m_state !== SingleKeyCryptoSessionState.none) {
            assert(false, `single key crypto session init key on invalid state`);
            this.m_logger.error(`single key crypto session init key on invalid state`);
            return ErrorCode.invalidState;
        }
        const ckr = await this.m_historyManager.createKey({peerid: this.m_remote.peerid});
        if (ckr.err) {
            this.m_logger.error(`single key crypto session init key failed for history manager createKey returned ${stringifyErrorCode(ckr.err)}`);
            return ckr.err;
        }
        this.m_logger.debug(`single key crypto session init keyHash ${ckr.keyHash!.toString('hex')} new ${!!ckr.newKey}`);
        this.m_key = ckr.key!;
        this.m_keyHash = ckr.keyHash!;
        if (!!ckr.newKey) {
            this.m_logger.debug(`single key crypto session enter state keyCreated`);
            this.m_state = SingleKeyCryptoSessionState.keyCreated; 
        } else {
            this.m_logger.debug(`single key crypto session enter state keyExchanged`);
            this.m_state = SingleKeyCryptoSessionState.keyExchanged;
        }
        return ErrorCode.success;
    }

    get encryptOptions(): PackageEncryptOptions {
        let encrypt: PackageEncryptOptions = Object.create(null);
        encrypt.key = this.m_key!;
        encrypt.keyHash = this.m_keyHash!;
        if (this.m_state === SingleKeyCryptoSessionState.keyCreated) {
            encrypt.newKey = {
                remotePublic: this.m_remote.publicKey, 
            };
            encrypt.sign = {
                secret: this.m_localPeer.secret
            };
        }
        return encrypt;
    }

    preSendPackage(): {err: ErrorCode, encrypt?: PackageEncryptOptions} {
        if (this.m_state === SingleKeyCryptoSessionState.none) {
            assert(false, `send package without create or exchange key`);
            this.m_logger.error(`send package without create or exchange key`);
            return {err: ErrorCode.invalidState};
        }
        let encrypt: PackageEncryptOptions = Object.create(null);
        encrypt.key = this.m_key!;
        encrypt.keyHash = this.m_keyHash!;
        if (this.m_state === SingleKeyCryptoSessionState.keyCreated) {
            encrypt.newKey = {
                remotePublic: this.m_remote.publicKey, 
            };
            encrypt.sign = {
                secret: this.m_localPeer.secret
            };
            this.m_state = SingleKeyCryptoSessionState.keyExchanged;
            this.m_logger.debug(`single key crypto session enter state keyExchanged`);
        }
        return {
            err: ErrorCode.success, 
            encrypt
        };
    }

    setUntrustKey(options: {keyHash: Buffer, key: Buffer}) {
        if (this.m_state === SingleKeyCryptoSessionState.none) {
            this.m_logger.debug(`got untrust key on none state, set session key to ${options.keyHash.toString()}`);
            this.m_key = PackageDecoder.decryptKey(options.key, this.m_localPeer.secret);
            this.m_keyHash = options.keyHash;
            this.m_logger.debug(`single key crypto session enter state keyUntrust`);
            this.m_state = SingleKeyCryptoSessionState.keyUntrust;
        }
    }

    onRecvPackage(pkg: {cmeType: number} & any): ErrorCode {
        if (!pkg.__encrypt) {
            this.m_logger.debug(`got nonencrypted pkg on crypto session`);
            return ErrorCode.invalidPkg;
        }
        if (this.m_state < SingleKeyCryptoSessionState.keyExchanged) {
            this.m_logger.debug(`got encrypted package and prepare to set session key to ${pkg.__encrypt.keyHash.toString()}`);
            if (this.m_state === SingleKeyCryptoSessionState.keyUntrust) {
                if (!this.m_keyHash!.equals(pkg.__encrypt.keyHash)) {
                    this.m_logger.debug(`exchanged key ${pkg.__encrypt.keyHash.toString()} donot match untrust key ${this.m_keyHash!.toString()}`);
                }
            }
            this.m_key = pkg.__encrypt.key;
            this.m_keyHash = pkg.__encrypt.keyHash;
            if (pkg.__encrypt.newKey) {
                this.m_logger.debug(`add session key to history manager`);
                // not async, dont care the result
                this.m_historyManager.addKey({
                    peerid: this.m_remote.peerid, 
                    key: this.m_key!, 
                    keyHash: this.m_keyHash!, 
                });
            } else {
                if (!this.m_remote.peerid.equal(pkg.__encrypt.peerid)) {
                    this.m_logger.debug(`first package's key isnot exchanged with remote peer`);
                    return ErrorCode.invalidPkg;
                }
            }
            if (this.m_interface) {
                this.m_interface.addKey(pkg.__encrypt);
            }
            this.m_logger.debug(`single key crypto session enter state keyExchanged`);
            this.m_state = SingleKeyCryptoSessionState.keyExchanged;
        } else {
            if (!this.m_keyHash!.equals(pkg.__encrypt.keyHash)) {
                this.m_logger.debug(`got encrypted pkg with another key ${pkg.__encrypt.keyHash.toString()} on single key crypto session`);
                return ErrorCode.invalidPkg;
            }
        }
        return ErrorCode.success;
    }

    private m_state: SingleKeyCryptoSessionState = SingleKeyCryptoSessionState.none;
    private m_remote: {peerid: Peerid, publicKey: Buffer};
    private m_localPeer: LocalPeer;
    private m_historyManager: PeerHistoryManager;
    private m_key?: Buffer;
    private m_keyHash?: Buffer;
}