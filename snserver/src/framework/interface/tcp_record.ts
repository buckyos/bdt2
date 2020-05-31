import { EventEmitter } from 'events';
import * as crypto from 'crypto';
import { ErrorCode, aes, BufferWriter, BufferReader } from '../../base';

export abstract class RecordEncoder {
    constructor(options: {
        recordSize: number, 
        callback: (record: Buffer) => number,
    }) {
        this.m_recordSize = options.recordSize;
        this.m_callback = options.callback;
    }
    abstract push(data: Buffer, options?: {zeroCopy?: boolean}): {err: ErrorCode, size?: number};
    abstract close(): ErrorCode;
    abstract uninit(): void;
    abstract continue(): boolean;
    protected m_recordSize: number;
    protected m_callback: (record: Buffer) => number;
}

export abstract class RecordDecoder {
    abstract push(record: Buffer, options?: {zeroCopy?: boolean}): {err: ErrorCode, data?: Buffer[]};
}

export class NoneRecordEncoder extends RecordEncoder {
    private m_closed: boolean = false;
    private m_remain?: Buffer;
    push(data: Buffer, options?: {zeroCopy?: boolean}): {err: ErrorCode, size?: number} {
        if (this.m_remain) {
            return {err: ErrorCode.success, size: 0};
        }
        const zeroCopy = options && options.zeroCopy;
        let offset = 0;
        do {
            let record = data.slice(offset, offset + this.m_recordSize);
            const done = this.m_callback(record);
            this.m_remain = record.slice(done, record.length);
            if (done < record.length) {
                if (!zeroCopy) {
                    let newRemain = Buffer.alloc(this.m_remain.length);
                    this.m_remain.copy(newRemain);
                    this.m_remain = newRemain;
                } 
                return {err: ErrorCode.success, size: offset + record.length};
            } else {
                delete this.m_remain;
            }
            offset += record.length;
            if (offset >= data.length) {
                break;
            }
        } while (true);
        return {err: ErrorCode.success, size: data.length};
    }

    continue(): boolean {
        if (this.m_remain) {
            let record = this.m_remain;
            delete this.m_remain;
            const done = this.m_callback(record);
            if (done < record.length) {
                this.m_remain = record.slice(done, record.length);
                return false;
            } 
            if (this.m_closed) {
                this.m_callback(Buffer.alloc(0));
                return false;
            }
        }
        return true;
    }

    close(): ErrorCode {
        if (this.m_closed) {
            return ErrorCode.success;
        }
        this.m_closed = true;
        if (!this.m_remain) {
            this.m_callback(Buffer.alloc(0));
        }
        return ErrorCode.success;
    }

    uninit(): void {

    }
}

export class NoneRecordDecoder extends RecordDecoder {
    push(record: Buffer, options?: {zeroCopy?: boolean}): {err: ErrorCode, data?: Buffer[]} {
        return {err: ErrorCode.success, data: [record]};
    }
}

export class EncryptRecordEncoder extends RecordEncoder {
    constructor(options: {
        keyHash: Buffer, 
        key: Buffer, 
        recordSize: number, 
        naglingTimeout: number, 
        callback: (record: Buffer) => number
    }) {
        super(options);
        this.m_naglingTimout = options.naglingTimeout;
        this.m_keyHash = options.keyHash;
        this.m_key = options.key;
    }

    push(data: Buffer, options?: {zeroCopy?: false}): {err: ErrorCode, size?: number} {
        if (this.m_remain) {
            return {err: ErrorCode.success, size: 0};
        }
        let remain = data;
        let encoded = 0;
        let zeroCopy = options && options.zeroCopy;
        do {
            if (this.m_pending.size + remain.length < this.m_recordSize) {
                encoded += remain.length;
                this.m_pending.size = this.m_pending.size + remain.length;
                if (zeroCopy) {
                    this.m_pending.buffers.push(remain);
                } else {
                    this.m_pending.buffers.push(remain.slice());
                }
                if (!this.m_naglingTimer) {
                    this.m_naglingTimer = setTimeout(() => {
                        delete this.m_naglingTimer;
                        let buffers = this.m_pending.buffers;
                        this.m_pending.buffers = [];
                        this.m_pending.size = 0;
                        const record = this._encodeRecord(buffers);
                        const done = this.m_callback(record);
                        if (done < record.length) {
                            this.m_remain = record.slice(done, record.length);
                        } 
                    }, this.m_naglingTimout);
                }
                return {err: ErrorCode.success, size: encoded};
            } else {
                let lastSize = this.m_recordSize - this.m_pending.size;
                encoded += lastSize;
                let last = remain.slice(0, lastSize);
                let buffers = this.m_pending.buffers;
                this.m_pending.buffers = [];
                this.m_pending.size = 0;
                buffers.push(last);
                const record = this._encodeRecord(buffers);
                const done = this.m_callback(record);
                if (done < record.length) {
                    this.m_remain = record.slice(done, record.length);
                    return {err: ErrorCode.success, size: encoded};
                } else {
                    remain = data.slice(encoded - data.byteOffset);
                    if (!remain.length) {
                        return {err: ErrorCode.success, size: encoded};
                    }
                }
            }
        } while (true);
    }

    private _encodeRecord(data: Buffer[]): Buffer {
        let writer = new BufferWriter();
        writer.writeU16(this.m_recordSize);
        let cipher = crypto.createCipheriv('aes-256-ecb', this.m_key, '');
        cipher.setAutoPadding(true);
        for (const d of data) {
            writer.writeBytes(cipher.update(d));
        }
        writer.writeBytes(cipher.final());
        return writer.render();
    }

    continue(): boolean {
        if (this.m_remain) {
            let record = this.m_remain;
            delete this.m_remain;
            const done = this.m_callback(record);
            if (done < record.length) {
                this.m_remain = record.slice(done, record.length);
                return false;
            } 
            if (this.m_closed) {
                this.m_callback(Buffer.alloc(0));
                return false;
            }
        }
        return true;
    }

    close(): ErrorCode {
        if (this.m_closed) {
            return ErrorCode.success;
        }
        this.m_closed = true;
        if (!this.m_remain) {
            this.m_callback(Buffer.alloc(0));
        } else if (this.m_pending.size) {
            let buffers = this.m_pending.buffers;
            this.m_pending.buffers = [];
            this.m_pending.size = 0;
            const record = this._encodeRecord(buffers);
            const done = this.m_callback(record);
            if (done < record.length) {
                this.m_remain = record.slice(done, record.length);
            }
        }
        return ErrorCode.success;
    }

    uninit(): void {
        if (this.m_naglingTimer) {
            clearTimeout(this.m_naglingTimer);
            delete this.m_naglingTimer;
        }
    }

    private m_keyHash: Buffer;
    private m_key: Buffer;
    private m_closed: boolean = false;
    private m_naglingTimer?: NodeJS.Timer;
    protected m_naglingTimout: number;
    private m_remain?: Buffer;
    private m_pending: {
        size: number, 
        buffers: Buffer[], 
    } = {
        size: 0, 
        buffers: []
    };
}

enum EncryptRecordDecodeState {
    wait = 0, 
    header = 1,
    record = 2
}

export class EncryptRecordDecoder extends RecordDecoder {
    constructor(options: {
        keyHash: Buffer, 
        key: Buffer, 
    }) {
        super();
        this.m_key = options.key;
        this.m_keyHash = options.keyHash;
    }

    push(record: Buffer, options?: {zeroCopy?: boolean}): {err: ErrorCode, data?: Buffer[]} {
        let zeroCopy = options && options.zeroCopy;
        const headerSize = 2;
        let remain = record;
        let data = [];
        do {
            if (this.m_state === EncryptRecordDecodeState.wait) {
                this.m_state = EncryptRecordDecodeState.header;
                this.m_pending = {
                    buffers: [Buffer.alloc(0)], 
                    size: 0
                };
            } else if (this.m_state === EncryptRecordDecodeState.header) {
                let headerBuffer = this.m_pending!.buffers[0];
                if ((remain.length + headerBuffer.length) < headerSize) {
                    this.m_pending!.buffers[0] = Buffer.concat([headerBuffer, remain], remain.length + headerBuffer.length);
                    break;
                } else {
                    remain = remain.slice(headerSize - headerBuffer.length);
                    headerBuffer = Buffer.concat([headerBuffer, remain.slice(0, headerSize - headerBuffer.length)]);
                    const reader = new BufferReader(headerBuffer);
                    const recordSize = reader.readU16();
                    if (remain.length > recordSize) {
                        const sub = this._decodeRecord([remain.slice(0, recordSize)]);
                        data.push(...sub);
                        remain = remain.slice(recordSize);
                        this.m_state = EncryptRecordDecodeState.wait;
                        delete this.m_pending;
                    } else {
                        this.m_state = EncryptRecordDecodeState.record;
                        this.m_pending!.recordSize = recordSize;
                        this.m_pending!.size += remain.length;
                        if (zeroCopy) {
                            this.m_pending!.buffers.push(remain);
                        } else {
                            const newRemain = Buffer.alloc(remain.length);
                            remain.copy(newRemain);
                            this.m_pending!.buffers.push(newRemain);
                        }
                        break;
                    }
                }
            } else if (this.m_state === EncryptRecordDecodeState.record) {
                if (remain.length + this.m_pending!.size > this.m_pending!.recordSize!) {
                    let subRecord = this.m_pending!.buffers;
                    subRecord.push(remain.slice(0, this.m_pending!.recordSize! - this.m_pending!.size));
                    const sub = this._decodeRecord(subRecord);
                    data.push(...sub);
                    remain = remain.slice(this.m_pending!.recordSize! - this.m_pending!.size);
                    this.m_state = EncryptRecordDecodeState.wait;
                    delete this.m_pending;
                } else {
                    this.m_pending!.size += remain.length;
                    if (zeroCopy) {
                        this.m_pending!.buffers.push(remain);
                    } else {
                        const newRemain = Buffer.alloc(remain.length);
                        remain.copy(newRemain);
                        this.m_pending!.buffers.push(newRemain);
                    }
                    break;
                }
            }
        } while (true);

        return {err: ErrorCode.success, data};
    }

    private _decodeRecord(record: Buffer[]): Buffer[] {
        let cipher = crypto.createDecipheriv('aes-256-ecb', this.m_key, '');
        cipher.setAutoPadding(true);
        let data = [];
        for (const r of record) {
            data.push(cipher.update(r));
        }
        data.push(cipher.final());
        return data;
    }

    private m_key: Buffer;
    private m_keyHash: Buffer;
    private m_state: EncryptRecordDecodeState = EncryptRecordDecodeState.wait;
    private m_pending?: {
        recordSize?: number, 
        buffers: Buffer[], 
        size: number,
    };
}
